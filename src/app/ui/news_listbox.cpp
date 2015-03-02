// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/news_listbox.h"

#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "base/string.h"
#include "net/http_request.h"
#include "net/http_response.h"
#include "ui/link_label.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"
#include "ui/view.h"

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
    , m_desc(desc) {
  }

protected:
  void onPreferredSize(PreferredSizeEvent& ev) override {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
    Style* style = theme->styles.newsItem();
    Style* styleDetail = theme->styles.newsItemDetail();
    Style::State state;
    gfx::Size sz1 = style->preferredSize(getText().c_str(), state);
    gfx::Size sz2, sz2fourlines;

    if (!m_desc.empty()) {
      View* view = View::getView(getParent());
      sz2 = styleDetail->preferredSize(m_desc.c_str(), state,
        (view ? view->getViewportBounds().w: 0));
      sz2fourlines = styleDetail->preferredSize("\n\n\n", state);
    }

    ev.setPreferredSize(gfx::Size(0, MIN(sz1.h+sz2fourlines.h, sz1.h+sz2.h)));
  }

  void onPaint(PaintEvent& ev) override {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
    Graphics* g = ev.getGraphics();
    gfx::Rect bounds = getClientBounds();
    Style* style = theme->styles.newsItem();
    Style* styleDetail = theme->styles.newsItemDetail();

    Style::State state;
    if (hasMouse() && !getManager()->getCapture()) state += Style::hover();
    if (isSelected()) state += Style::active();
    if (getParent()->hasCapture()) state += Style::clicked();

    gfx::Size textSize = style->preferredSize(getText().c_str(), state);
    gfx::Rect textBounds(bounds.x, bounds.y, bounds.w, textSize.h);
    gfx::Rect detailsBounds(
      bounds.x, bounds.y+textSize.h,
      bounds.w, bounds.h-textSize.h);

    style->paint(g, textBounds, getText().c_str(), state);
    styleDetail->paint(g, detailsBounds, m_desc.c_str(), state);
  }

private:
  std::string m_desc;
};

NewsListBox::NewsListBox()
{
  std::stringstream output;
  net::HttpRequest http(WEBSITE_NEWS_RSS);
  net::HttpResponse response(&output);
  http.send(response);

  TiXmlDocument doc;
  doc.Parse(output.str().c_str(), 0, TIXML_DEFAULT_ENCODING);
  TiXmlHandle handle(&doc);
  TiXmlElement* itemXml = handle
    .FirstChild("rss")
    .FirstChild("channel")
    .FirstChild("item").ToElement();

  int count = 0;

  while (itemXml) {
    TiXmlElement* titleXml = itemXml->FirstChildElement("title");
    TiXmlElement* descXml = itemXml->FirstChildElement("description");
    TiXmlElement* linkXml = itemXml->FirstChildElement("link");
    TiXmlElement* guidXml = itemXml->FirstChildElement("guid");
    TiXmlElement* pubDateXml = itemXml->FirstChildElement("pubDate");

    std::string link = linkXml->GetText();
    std::string title = titleXml->GetText();
    std::string desc = parse_html(descXml->GetText());

    addChild(new NewsItem(link, title, desc));

    itemXml = itemXml->NextSiblingElement();
    if (++count == 4)
      break;
  }

  TiXmlElement* linkXml = handle
    .FirstChild("rss")
    .FirstChild("channel")
    .FirstChild("link").ToElement();
  if (linkXml)
    addChild(new NewsItem(linkXml->GetText(), "More...", ""));
}

} // namespace app
