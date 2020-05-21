// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/news_listbox.h"

#include "app/app.h"
#include "app/pref/preferences.h"
#include "app/res/http_loader.h"
#include "app/ui/skin/skin_theme.h"
#include "app/xml_document.h"
#include "base/fs.h"
#include "base/string.h"
#include "base/time.h"
#include "ui/link_label.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/view.h"
#include "ver/info.h"

#include "tinyxml.h"

#include <cctype>
#include <sstream>

namespace app {

using namespace ui;
using namespace app::skin;

namespace {

std::string convert_html_entity(const std::string& e)
{
  if (e.size() >= 3 && e[0] == '#' && std::isdigit(e[1])) {
    long unicodeChar;
    if (e[2] == 'x')
      unicodeChar = std::strtol(e.c_str()+1, nullptr, 16);
    else
      unicodeChar = std::strtol(e.c_str()+1, nullptr, 10);

    if (unicodeChar == 0x2018) return "\x60";
    if (unicodeChar == 0x2019) return "'";
    else {
      std::wstring wstr(1, (wchar_t)unicodeChar);
      return base::to_utf8(wstr);
    }
  }
  else if (e == "lt") return "<";
  else if (e == "gt") return ">";
  else if (e == "amp") return "&";
  return "";
}

std::string parse_html(const std::string& str)
{
  bool paraOpen = true;
  std::string result;
  size_t i = 0;
  while (i < str.size()) {
    // Ignore content between <...> symbols
    if (str[i] == '<') {
      size_t j = ++i;
      while (i < str.size() && str[i] != '>')
        ++i;

      if (i < str.size()) {
        ASSERT(str[i] == '>');

        std::string tag = str.substr(j, i - j);
        if (tag == "li") {
          if (!paraOpen)
            result.push_back('\n');
          result.push_back((char)0xc2);
          result.push_back((char)0xb7); // middle dot
          result.push_back(' ');
          paraOpen = false;
        }
        else if (tag == "p" || tag == "ul") {
          if (!paraOpen)
            result.push_back('\n');
          paraOpen = true;
        }

        ++i;
      }
    }
    else if (str[i] == '&') {
      size_t j = ++i;
      while (i < str.size() && str[i] != ';')
        ++i;

      if (i < str.size()) {
        ASSERT(str[i] == ';');
        std::string entity = str.substr(j, i - j);
        result += convert_html_entity(entity);
        ++i;
      }

      paraOpen = false;
    }
    else {
      result.push_back(str[i++]);
      paraOpen = false;
    }
  }
  return result;
}

}

class NewsItem : public LinkLabel {
public:
  NewsItem(const std::string& link,
           const std::string& title,
           const std::string& desc)
    : LinkLabel(link, title)
    , m_title(title)
    , m_desc(desc) {
  }

protected:
  void onSizeHint(SizeHintEvent& ev) override {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    ui::Style* style = theme->styles.newsItem();

    setTextQuiet(m_title);
    gfx::Size sz = theme->calcSizeHint(this, style);

    if (!m_desc.empty())
      sz.h *= 5;

    ev.setSizeHint(gfx::Size(0, sz.h));
  }

  void onPaint(PaintEvent& ev) override {
    SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
    Graphics* g = ev.graphics();
    gfx::Rect bounds = clientBounds();
    ui::Style* style = theme->styles.newsItem();
    ui::Style* styleDetail = theme->styles.newsItemDetail();

    setTextQuiet(m_title);
    gfx::Size textSize = theme->calcSizeHint(this, style);
    gfx::Rect textBounds(bounds.x, bounds.y, bounds.w, textSize.h);
    gfx::Rect detailsBounds(
      bounds.x, bounds.y+textSize.h,
      bounds.w, bounds.h-textSize.h);

    theme->paintWidget(g, this, style, textBounds);

    setTextQuiet(m_desc);
    theme->paintWidget(g, this, styleDetail, detailsBounds);
  }

private:
  std::string m_title;
  std::string m_desc;
};

class ProblemsItem : public NewsItem {
public:
  ProblemsItem() : NewsItem("", "Problems loading news. Retry.", "") {
  }

protected:
  void onClick() override {
    static_cast<NewsListBox*>(parent())->reload();
  }
};

NewsListBox::NewsListBox()
  : m_timer(250, this)
  , m_loader(nullptr)
{
  m_timer.Tick.connect(&NewsListBox::onTick, this);

  std::string cache = Preferences::instance().news.cacheFile();
  if (!cache.empty() && base::is_file(cache) && validCache(cache))
    parseFile(cache);
  else
    reload();
}

NewsListBox::~NewsListBox()
{
  if (m_timer.isRunning())
    m_timer.stop();

  delete m_loader;
  m_loader = nullptr;
}

void NewsListBox::reload()
{
  if (m_loader || m_timer.isRunning())
    return;

  while (lastChild())
    removeChild(lastChild());

  View* view = View::getView(this);
  if (view)
    view->updateView();

  m_loader = new HttpLoader(get_app_news_rss_url());
  m_timer.start();
}

bool NewsListBox::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kCloseMessage:
      if (m_loader)
        m_loader->abort();
      break;
  }

  return ListBox::onProcessMessage(msg);
}

void NewsListBox::onTick()
{
  if (!m_loader || !m_loader->isDone())
    return;

  std::string fn = m_loader->filename();

  delete m_loader;
  m_loader = nullptr;
  m_timer.stop();

  if (fn.empty()) {
    addChild(new ProblemsItem());
    View::getView(this)->updateView();
    return;
  }

  parseFile(fn);
}

void NewsListBox::parseFile(const std::string& filename)
{
  View* view = View::getView(this);

  XmlDocumentRef doc;
  try {
    doc = open_xml(filename);
  }
  catch (...) {
    addChild(new ProblemsItem());
    if (view)
      view->updateView();
    return;
  }

  TiXmlHandle handle(doc.get());
  TiXmlElement* itemXml = handle
    .FirstChild("rss")
    .FirstChild("channel")
    .FirstChild("item").ToElement();

  int count = 0;

  while (itemXml) {
    TiXmlElement* titleXml = itemXml->FirstChildElement("title");
    TiXmlElement* descXml = itemXml->FirstChildElement("description");
    TiXmlElement* linkXml = itemXml->FirstChildElement("link");
    if (titleXml && titleXml->GetText() &&
        descXml && descXml->GetText() &&
        linkXml && linkXml->GetText()) {
      std::string link = linkXml->GetText();
      std::string title = titleXml->GetText();
      std::string desc = parse_html(descXml->GetText());
      // Limit the description text to 4 lines
      std::string::size_type i = 0;
      int j = 0;
      while (true) {
        i = desc.find('\n', i);
        if (i == std::string::npos)
          break;
        i++;
        j++;
        if (j == 5)
          desc = desc.substr(0, i);
      }

      addChild(new NewsItem(link, title, desc));
      if (++count == 4)
        break;
    }
    itemXml = itemXml->NextSiblingElement();
  }

  TiXmlElement* linkXml = handle
    .FirstChild("rss")
    .FirstChild("channel")
    .FirstChild("link").ToElement();
  if (linkXml && linkXml->GetText())
    addChild(new NewsItem(linkXml->GetText(), "More...", ""));

  if (view)
    view->updateView();

  // Save as cached news
  Preferences::instance().news.cacheFile(filename);
}

bool NewsListBox::validCache(const std::string& filename)
{
  base::Time
    now = base::current_time(),
    time = base::get_modification_time(filename);

  now.dateOnly();
  time.dateOnly();

  return (now == time);
}

} // namespace app
