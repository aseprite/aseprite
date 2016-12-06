// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app_menus.h"
#include "app/resource_finder.h"
#include "app/ui/browser_view.h"
#include "app/ui/skin/skin_style_property.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/workspace.h"
#include "base/file_handle.h"
#include "base/fs.h"
#include "ui/box.h"
#include "ui/link_label.h"
#include "ui/menu.h"
#include "ui/message.h"
#include "ui/system.h"
#include "ui/textbox.h"

#include "cmark.h"

#include <array>

namespace app {

using namespace ui;
using namespace app::skin;

class BrowserView::CMarkBox : public ui::VBox {
public:
  CMarkBox() {
    setBgColor(SkinTheme::instance()->colors.textboxFace());
    setBorder(gfx::Border(4*guiscale()));
  }

  void loadFile(const std::string& inputFile) {
    std::string file = inputFile;
    if (file.size() >= 5 && file.substr(0, 5) == "data/") {
      ResourceFinder rf;
      rf.includeDataDir(file.substr(5).c_str());
      if (rf.findFirst())
        file = rf.filename();
    }

    cmark_parser* parser = cmark_parser_new(CMARK_OPT_DEFAULT);
    FILE* fp = base::open_file_raw(file, "rb");
    if (fp) {
      std::array<char, 4096> buffer;
      size_t bytes;
      while ((bytes = std::fread(&buffer[0], 1, buffer.size(), fp)) > 0) {
        cmark_parser_feed(parser, &buffer[0], bytes);
        if (bytes < buffer.size()) {
          break;
        }
      }
      cmark_node* root = cmark_parser_finish(parser);

      if (root) {
        processNode(root);
        cmark_node_free(root);
      }
      fclose(fp);
    }
    else {
      clear();
      addTextBox("File not found: " + file);
    }
    cmark_parser_free(parser);

    relayout();
  }

private:
  bool onProcessMessage(Message* msg) override {
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

    return VBox::onProcessMessage(msg);
  }

  void clear() {
    // Delete all children
    while (firstChild())
      delete firstChild();
  }

  void processNode(cmark_node* root) {
    clear();

    std::string content;
    bool inHeading = false;
    bool inImage = false;
    bool extraSeparation = true;
    const char* inLink = nullptr;

    cmark_iter* iter = cmark_iter_new(root);
    cmark_event_type ev_type;
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
      cmark_node* cur = cmark_iter_get_node(iter);

      switch (cmark_node_get_type(cur)) {

        case CMARK_NODE_CODE_BLOCK:
        case CMARK_NODE_TEXT: {
          const char* text = cmark_node_get_literal(cur);
          if (!inImage && text) {
            if (inLink && inHeading) {
              closeContent(content);
              addLink(inLink, text);
              extraSeparation = false;
            }
            else {
              if (inLink && !content.empty() &&
                  content[content.size()-1] != ' ') {
                content += " ";
              }
              content += text;
            }
          }
          break;
        }

        case CMARK_NODE_LINEBREAK: {
          content += "\n";
          break;
        }

        case CMARK_NODE_ITEM: {
          if (ev_type == CMARK_EVENT_ENTER)
            content += " - ";
          break;
        }

        case CMARK_NODE_THEMATIC_BREAK: {
          if (ev_type == CMARK_EVENT_ENTER) {
            closeContent(content);
            addSeparator();
          }
          else if (ev_type == CMARK_EVENT_EXIT) {
            content += "\n\n";
          }
          break;
        }

        case CMARK_NODE_HEADING: {
          if (ev_type == CMARK_EVENT_ENTER) {
            inHeading = true;

            closeContent(content);
            addSeparator();
          }
          else if (ev_type == CMARK_EVENT_EXIT) {
            inHeading = false;

            content += "\n";
            if (extraSeparation)
              content += "\n";
            else
              extraSeparation = true;
          }
          break;
        }

        case CMARK_NODE_LIST:
        case CMARK_NODE_PARAGRAPH: {
          if (ev_type == CMARK_EVENT_EXIT)
            content += "\n";
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
          }
          else if (ev_type == CMARK_EVENT_EXIT) {
            inLink = nullptr;
          }
        }

      }
    }
    cmark_iter_free(iter);

    closeContent(content);
  }

  void closeContent(std::string& content) {
    if (!content.empty()) {
      addTextBox(content);
      content.clear();
    }
  }

  void addSeparator() {
    auto sep = new Separator("", HORIZONTAL);
    sep->setBgColor(SkinTheme::instance()->colors.textboxFace());
    addChild(sep);
  }

  void addTextBox(const std::string& content) {
    addChild(new TextBox(content, LEFT | WORDWRAP));
  }

  void addLink(const std::string& url, const std::string& text) {
    addChild(new LinkLabel(url, text));
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

};

BrowserView::BrowserView()
  : m_textBox(new CMarkBox)
{
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());

  addChild(&m_view);

  m_view.attachToView(m_textBox);
  m_view.setExpansive(true);
  m_view.setProperty(SkinStylePropertyPtr(
      new SkinStyleProperty(theme->styles.workspaceView())));
}

BrowserView::~BrowserView()
{
  delete m_textBox;
}

void BrowserView::loadFile(const std::string& file)
{
  m_title = base::get_file_title(file);
  m_filename = file;

  m_textBox->loadFile(file);
}

std::string BrowserView::getTabText()
{
  return m_title;
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
