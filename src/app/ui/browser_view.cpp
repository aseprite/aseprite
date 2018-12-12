// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2016-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/app_menus.h"
#include "app/resource_finder.h"
#include "app/ui/browser_view.h"
#include "app/ui/main_window.h"
#include "app/ui/separator_in_view.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "base/split_string.h"
#include "os/font.h"
#include "ui/alert.h"
#include "ui/link_label.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/resize_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/textbox.h"

#include "cmark.h"

#include <array>
#include <cstring>
#include <string>
#include <vector>

namespace app {

using namespace ui;
using namespace app::skin;

namespace {

RegisterMessage kLoadFileMessage;

class LoadFileMessage : public Message {
public:
  LoadFileMessage(const std::string& file)
    : Message(kLoadFileMessage)
    , m_file(file) {
  }

  const std::string& file() const { return m_file; }

private:
  std::string m_file;
};

} // annonymous namespace

// TODO This is not the best implementation, but it's "good enough"
//      for a first version.
class BrowserView::CMarkBox : public Widget {
  class Break : public Widget {
  public:
    Break() {
      setMinSize(gfx::Size(0, font()->height()));
    }
  };
  class OpenList : public Widget { };
  class CloseList : public Widget { };
  class Item : public Label {
  public:
    Item(const std::string& text) : Label(text) { }
  };

public:
  obs::signal<void()> FileChange;

  CMarkBox() {
    initTheme();
  }

  const std::string& file() {
    return m_file;
  }

  void loadFile(const std::string& inputFile) {
    std::string file = inputFile;
    {
      ResourceFinder rf;
      rf.includeDataDir(file.c_str());
      if (rf.findFirst())
        file = rf.filename();
    }
    m_file = file;

    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    FILE* fp = base::open_file_raw(file, "rb");
    if (fp) {
      std::array<char, 4096> buffer;
      size_t bytes;
      bool isTxt = (base::get_file_extension(file) == "txt");

      if (isTxt)
        cmark_parser_feed(parser, "```\n", 4);

      while ((bytes = std::fread(&buffer[0], 1, buffer.size(), fp)) > 0) {
        cmark_parser_feed(parser, &buffer[0], bytes);
        if (bytes < buffer.size()) {
          break;
        }
      }

      if (isTxt)
        cmark_parser_feed(parser, "\n```\n", 5);

      cmark_node* root = cmark_parser_finish(parser);

      if (root) {
        processNode(root);
        cmark_node_free(root);
      }
      fclose(fp);
    }
    else {
      clear();
      addText("File not found:");
      addBreak();
      addCodeBlock(file);
    }
    cmark_parser_free(parser);

    relayout();
    FileChange();
  }

private:
  void layoutElements(int width,
                      std::function<void(const gfx::Rect& bounds,
                                         Widget* child)> callback) {
    const WidgetsList& children = this->children();
    const gfx::Rect cpos = childrenBounds();

    gfx::Point p = cpos.origin();
    int maxH = 0;
    int itemLevel = 0;
    //Widget* prevChild = nullptr;

    for (auto child : children) {
      gfx::Size sz = child->sizeHint(gfx::Size(width, 0));

      bool isBreak = (dynamic_cast<Break*>(child) ? true: false);
      bool isOpenList = (dynamic_cast<OpenList*>(child) ? true: false);
      bool isCloseList = (dynamic_cast<CloseList*>(child) ? true: false);
      bool isItem = (dynamic_cast<Item*>(child) ? true: false);

      if (isOpenList) {
        ++itemLevel;
      }
      else if (isCloseList) {
        --itemLevel;
      }
      else if (isItem) {
        p.x -= sz.w;
      }

      if (child->isExpansive() ||
          p.x+sz.w > cpos.x+width ||
          isBreak || isOpenList || isCloseList) {
        p.x = cpos.x + itemLevel*font()->textLength(" - ");
        p.y += maxH;
        maxH = 0;
        //prevChild = nullptr;
      }

      if (child->isExpansive())
        sz.w = std::max(sz.w, width);

      callback(gfx::Rect(p, sz), child);

      //if (!isItem) prevChild = child;
      //if (isBreak) prevChild = nullptr;

      maxH = std::max(maxH, sz.h);
      p.x += sz.w;
    }
  }

  void onSizeHint(SizeHintEvent& ev) override {
    gfx::Size sz;

    layoutElements(
      View::getView(this)->viewportBounds().w - border().width(),
      [&](const gfx::Rect& rc, Widget* child) {
        sz.w = std::max(sz.w, rc.x+rc.w-this->bounds().x);
        sz.h = std::max(sz.h, rc.y+rc.h-this->bounds().y);
      });
    sz.w += border().right();
    sz.h += border().bottom();

    ev.setSizeHint(sz);
  }

  void onResize(ResizeEvent& ev) override {
    setBoundsQuietly(ev.bounds());

    layoutElements(
      View::getView(this)->viewportBounds().w - border().width(),
      [](const gfx::Rect& rc, Widget* child) {
        child->setBounds(rc);
      });
  }

  void onPaint(PaintEvent& ev) override {
    Graphics* g = ev.graphics();
    gfx::Rect rc = clientBounds();
    auto skin = (SkinTheme*)theme();

    g->fillRect(skin->colors.textboxFace(), rc);
  }

  bool onProcessMessage(Message* msg) override {
    if (msg->type() == kLoadFileMessage) {
      loadFile(static_cast<LoadFileMessage*>(msg)->file());
      return true;
    }

    switch (msg->type()) {

      case kMouseWheelMessage: {
        View* view = View::getView(this);
        if (view) {
          auto mouseMsg = static_cast<MouseMessage*>(msg);
          gfx::Point scroll = view->viewScroll();

          if (mouseMsg->preciseWheel())
            scroll += mouseMsg->wheelDelta();
          else
            scroll += mouseMsg->wheelDelta() * textHeight()*3;

          view->setViewScroll(scroll);
        }
        break;
      }
    }

    return Widget::onProcessMessage(msg);
  }

  void onInitTheme(InitThemeEvent& ev) override {
    Widget::onInitTheme(ev);
    setBgColor(SkinTheme::instance()->colors.textboxFace());
    setBorder(gfx::Border(4*guiscale()));
  }

  void clear() {
    // Delete all children
    while (firstChild())
      delete firstChild();
  }

  void processNode(cmark_node* root) {
    clear();

    m_content.clear();

    bool inHeading = false;
    bool inImage = false;
    const char* inLink = nullptr;

    cmark_iter* iter = cmark_iter_new(root);
    cmark_event_type ev_type;
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
      cmark_node* cur = cmark_iter_get_node(iter);

      switch (cmark_node_get_type(cur)) {

        case CMARK_NODE_TEXT: {
          const char* text = cmark_node_get_literal(cur);
          if (!inImage && text) {
            if (inLink) {
              if (!m_content.empty() &&
                  m_content[m_content.size()-1] != ' ') {
                m_content += " ";
              }
              m_content += text;
            }
            else {
              m_content += text;
              if (inHeading)
                closeContent();
            }
          }
          break;
        }

        case CMARK_NODE_INLINE_HTML: {
          const char* text = cmark_node_get_literal(cur);
          if (text && std::strncmp(text, "<br />", 6) == 0) {
            closeContent();
            addBreak();
          }
          break;
        }

        case CMARK_NODE_CODE: {
          const char* text = cmark_node_get_literal(cur);
          if (text) {
            closeContent();
            addCodeInline(text);
          }
          break;
        }

        case CMARK_NODE_CODE_BLOCK: {
          const char* text = cmark_node_get_literal(cur);
          if (text) {
            closeContent();
            addCodeBlock(text);
          }
          break;
        }

        case CMARK_NODE_SOFTBREAK: {
          m_content += " ";
          break;
        }

        case CMARK_NODE_LINEBREAK: {
          closeContent();
          addBreak();
          break;
        }

        case CMARK_NODE_LIST: {
          if (ev_type == CMARK_EVENT_ENTER) {
            closeContent();
            addChild(new OpenList);
          }
          else if (ev_type == CMARK_EVENT_EXIT) {
            closeContent();
            addChild(new CloseList);
          }
          break;
        }

        case CMARK_NODE_ITEM: {
          if (ev_type == CMARK_EVENT_ENTER) {
            closeContent();
            addChild(new Item(" - "));
          }
          break;
        }

        case CMARK_NODE_THEMATIC_BREAK: {
          if (ev_type == CMARK_EVENT_ENTER) {
            closeContent();
            addSeparator();
          }
          else if (ev_type == CMARK_EVENT_EXIT) {
            closeContent();
            addBreak();
            addBreak();
          }
          break;
        }

        case CMARK_NODE_HEADING: {
          if (ev_type == CMARK_EVENT_ENTER) {
            inHeading = true;

            closeContent();
            addSeparator();
          }
          else if (ev_type == CMARK_EVENT_EXIT) {
            inHeading = false;

            closeContent();
            addBreak();
            addBreak();
          }
          break;
        }

        case CMARK_NODE_PARAGRAPH: {
          if (ev_type == CMARK_EVENT_EXIT) {
            closeContent();
            addBreak();
          }
          break;
        }

        case CMARK_NODE_IMAGE: {
          if (ev_type == CMARK_EVENT_ENTER)
            inImage = true;
          else if (ev_type == CMARK_EVENT_EXIT)
            inImage = false;
        }

        case CMARK_NODE_LINK: {
          if (ev_type == CMARK_EVENT_ENTER) {
            inLink = cmark_node_get_url(cur);
            if (inLink)
              closeContent();
          }
          else if (ev_type == CMARK_EVENT_EXIT) {
            if (inLink) {
              if (!m_content.empty()) {
                addLink(inLink, m_content);
                m_content.clear();
              }
            }
            inLink = nullptr;
          }
        }

      }
    }
    cmark_iter_free(iter);

    closeContent();
  }

  void closeContent() {
    if (!m_content.empty()) {
      addText(m_content);
      m_content.clear();
    }
  }

  void addSeparator() {
    auto sep = new SeparatorInView(std::string(), HORIZONTAL);
    sep->setBorder(gfx::Border(0, font()->height(), 0, font()->height()));
    sep->setExpansive(true);
    addChild(sep);
  }

  void addBreak() {
    addChild(new Break);
  }

  void addText(const std::string& content) {
    std::vector<std::string> words;
    base::split_string(content, words, " ");
    for (const auto& word : words)
      if (!word.empty()) {
        Label* label;

        if (word.size() > 4 &&
            std::strncmp(word.c_str(), "http", 4) == 0) {
          label = new LinkLabel(word);
          label->setStyle(SkinTheme::instance()->styles.browserLink());
        }
        else
          label = new Label(word);

        // Uncomment this line to debug labels
        //label->setBgColor(gfx::rgba((rand()%128)+128, 128, 128));

        addChild(label);
      }
  }

  void addCodeInline(const std::string& content) {
    auto label = new Label(content);
    label->setBgColor(SkinTheme::instance()->colors.textboxCodeFace());
    addChild(label);
  }

  void addCodeBlock(const std::string& content) {
    auto textBox = new TextBox(content, LEFT);
    textBox->InitTheme.connect(
      [textBox]{
        textBox->setBgColor(SkinTheme::instance()->colors.textboxCodeFace());
        textBox->setBorder(gfx::Border(4*guiscale()));
      });
    textBox->initTheme();
    addChild(textBox);
  }

  void addLink(const std::string& url, const std::string& text) {
    auto label = new LinkLabel(url, text);
    label->InitTheme.connect(
      [label]{
        label->setStyle(SkinTheme::instance()->styles.browserLink());
      });
    label->initTheme();

    if (url.find(':') == std::string::npos) {
      label->setUrl("");
      label->Click.connect(
        [this, url]{
          Message* msg = new LoadFileMessage(url);
          msg->setRecipient(this);
          Manager::getDefault()->enqueueMessage(msg);
        });
    }

    // Uncomment this line to debug labels
    //label->setBgColor(gfx::rgba((rand()%128)+128, 128, 128));

    addChild(label);
  }

  void relayout() {
    layout();
    auto view = View::getView(this);
    if (view) {
      view->updateView();
      view->setViewScroll(gfx::Point(0, 0));
    }
    invalidate();
  }

  std::string m_file;
  std::string m_content;
};

BrowserView::BrowserView()
  : m_textBox(new CMarkBox)
{
  addChild(&m_view);

  m_view.attachToView(m_textBox);
  m_view.setExpansive(true);
  m_view.InitTheme.connect(
    [this]{
      m_view.setStyle(SkinTheme::instance()->styles.workspaceView());
    });
  m_view.initTheme();

  m_textBox->FileChange.connect(
    []{
      App::instance()->workspace()->updateTabs();
    });
}

BrowserView::~BrowserView()
{
  delete m_textBox;
}

void BrowserView::loadFile(const std::string& file)
{
  m_textBox->loadFile(file);
}

std::string BrowserView::getTabText()
{
  return base::get_file_title(m_textBox->file());
}

TabIcon BrowserView::getTabIcon()
{
  return TabIcon::NONE;
}

WorkspaceView* BrowserView::cloneWorkspaceView()
{
  return new BrowserView();
}

void BrowserView::onWorkspaceViewSelected()
{
}

bool BrowserView::onCloseView(Workspace* workspace, bool quitting)
{
  workspace->removeView(this);
  return true;
}

void BrowserView::onTabPopup(Workspace* workspace)
{
  Menu* menu = AppMenus::instance()->getTabPopupMenu();
  if (!menu)
    return;

  menu->showPopup(ui::get_mouse_position());
}

} // namespace app
