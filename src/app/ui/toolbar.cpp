// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/toolbar.h"

#include "app/app.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/i18n/strings.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool_box.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "fmt/format.h"
#include "gfx/size.h"
#include "obs/signal.h"
#include "os/surface.h"
#include "ui/ui.h"

#include <string>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace tools;

// Class to show a group of tools (horizontally)
// This widget is inside the ToolBar::m_popupWindow
class ToolBar::ToolStrip : public Widget {
public:
  ToolStrip(ToolGroup* group, ToolBar* toolbar);
  ~ToolStrip();

  ToolGroup* toolGroup() { return m_group; }

  obs::signal<void(Tool*)> ToolSelected;

protected:
  bool onProcessMessage(Message* msg) override;
  void onSizeHint(SizeHintEvent& ev) override;
  void onPaint(PaintEvent& ev) override;

private:
  Rect getToolBounds(int index);

  ToolGroup* m_group;
  Tool* m_hotTool;
  ToolBar* m_toolbar;
};

static Size getToolIconSize(Widget* widget)
{
  SkinTheme* theme = static_cast<SkinTheme*>(widget->theme());
  os::Surface* icon = theme->getToolIcon("configuration");
  if (icon)
    return Size(icon->width(), icon->height());
  else
    return Size(16, 16) * guiscale();
}

//////////////////////////////////////////////////////////////////////
// ToolBar

ToolBar* ToolBar::m_instance = NULL;

ToolBar::ToolBar()
  : Widget(kGenericWidget)
  , m_openedRecently(false)
  , m_tipTimer(300, this)
{
  m_instance = this;

  setBorder(gfx::Border(1*guiscale(), 0, 1*guiscale(), 0));

  m_hotTool = NULL;
  m_hotIndex = NoneIndex;
  m_openOnHot = false;
  m_popupWindow = NULL;
  m_currentStrip = NULL;
  m_tipWindow = NULL;
  m_tipOpened = false;

  ToolBox* toolbox = App::instance()->toolBox();
  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (m_selectedInGroup.find(tool->getGroup()) == m_selectedInGroup.end())
      m_selectedInGroup[tool->getGroup()] = tool;
  }

  App::instance()->activeToolManager()->add_observer(this);
}

ToolBar::~ToolBar()
{
  App::instance()->activeToolManager()->remove_observer(this);

  delete m_popupWindow;
  delete m_tipWindow;
}

bool ToolBar::isToolVisible(Tool* tool)
{
  return (m_selectedInGroup[tool->getGroup()] == tool);
}

bool ToolBar::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      ToolBox* toolbox = App::instance()->toolBox();
      int groups = toolbox->getGroupsCount();
      Rect toolrc;

      ToolGroupList::iterator it = toolbox->begin_group();
      for (int c=0; c<groups; ++c, ++it) {
        ToolGroup* tool_group = *it;
        Tool* tool = m_selectedInGroup[tool_group];

        toolrc = getToolGroupBounds(c);
        if (mouseMsg->position().y >= toolrc.y &&
            mouseMsg->position().y < toolrc.y+toolrc.h) {
          selectTool(tool);

          openPopupWindow(c, tool_group);

          // We capture the mouse so the user can continue navigating
          // the ToolBar to open other groups while he is pressing the
          // mouse button.
          captureMouse();
        }
      }

      toolrc = getToolGroupBounds(PreviewVisibilityIndex);
      if (mouseMsg->position().y >= toolrc.y &&
          mouseMsg->position().y < toolrc.y+toolrc.h) {
        // Toggle preview visibility
        PreviewEditorWindow* preview =
          App::instance()->mainWindow()->getPreviewEditor();
        bool state = preview->isPreviewEnabled();
        preview->setPreviewEnabled(!state);
      }
      break;
    }

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      ToolBox* toolbox = App::instance()->toolBox();
      int groups = toolbox->getGroupsCount();
      Tool* new_hot_tool = NULL;
      int new_hot_index = NoneIndex;
      Rect toolrc;

      ToolGroupList::iterator it = toolbox->begin_group();

      for (int c=0; c<groups; ++c, ++it) {
        ToolGroup* tool_group = *it;
        Tool* tool = m_selectedInGroup[tool_group];

        toolrc = getToolGroupBounds(c);
        if (mouseMsg->position().y >= toolrc.y &&
            mouseMsg->position().y < toolrc.y+toolrc.h) {
          new_hot_tool = tool;
          new_hot_index = c;

          if ((m_openOnHot) && (m_hotTool != new_hot_tool) && hasCapture()) {
            openPopupWindow(c, tool_group);
          }
          break;
        }
      }

      toolrc = getToolGroupBounds(PreviewVisibilityIndex);
      if (mouseMsg->position().y >= toolrc.y &&
          mouseMsg->position().y < toolrc.y+toolrc.h) {
        new_hot_index = PreviewVisibilityIndex;
      }

      // hot button changed
      if (new_hot_tool != m_hotTool ||
          new_hot_index != m_hotIndex) {

        m_hotTool = new_hot_tool;
        m_hotIndex = new_hot_index;
        invalidate();

        if (!m_currentStrip) {
          if (m_hotIndex != NoneIndex && !hasCapture())
            openTipWindow(m_hotIndex, m_hotTool);
          else
            closeTipWindow();
        }

        if (m_hotTool) {
          if (hasCapture())
            selectTool(m_hotTool);
          else
            StatusBar::instance()->showTool(0, m_hotTool);
        }
      }

      // We can change the current tool if the user is dragging the
      // mouse over the ToolBar.
      if (hasCapture()) {
        MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
        Widget* pick = manager()->pick(mouseMsg->position());
        if (ToolStrip* strip = dynamic_cast<ToolStrip*>(pick)) {
          releaseMouse();

          MouseMessage* mouseMsg2 = new MouseMessage(
            kMouseDownMessage,
            mouseMsg->pointerType(),
            mouseMsg->button(),
            mouseMsg->modifiers(),
            mouseMsg->position());
          mouseMsg2->setRecipient(strip);
          manager()->enqueueMessage(mouseMsg2);
        }
      }
      break;
    }

    case kMouseUpMessage:
      if (!hasCapture())
        break;

      if (!m_openedRecently) {
        if (m_popupWindow && m_popupWindow->isVisible())
          m_popupWindow->closeWindow(this);
      }
      m_openedRecently = false;

      releaseMouse();
      // fallthrough

    case kMouseLeaveMessage:
      if (hasCapture())
        break;

      closeTipWindow();

      if (!m_popupWindow || !m_popupWindow->isVisible()) {
        m_tipOpened = false;

        m_hotTool = NULL;
        m_hotIndex = NoneIndex;
        invalidate();
      }

      StatusBar::instance()->showDefaultText();
      break;

    case kTimerMessage:
      if (static_cast<TimerMessage*>(msg)->timer() == &m_tipTimer) {
        if (m_tipWindow)
          m_tipWindow->openWindow();

        m_tipTimer.stop();
        m_tipOpened = true;
      }
      break;

  }

  return Widget::onProcessMessage(msg);
}

void ToolBar::onSizeHint(SizeHintEvent& ev)
{
  Size iconsize = getToolIconSize(this);
  iconsize.w += border().width();
  iconsize.h += border().height();
  ev.setSizeHint(iconsize);
}

void ToolBar::onPaint(ui::PaintEvent& ev)
{
  gfx::Rect bounds = clientBounds();
  Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  ToolBox* toolbox = App::instance()->toolBox();
  Tool* activeTool = App::instance()->activeTool();
  ToolGroupList::iterator it = toolbox->begin_group();
  int groups = toolbox->getGroupsCount();
  Rect toolrc;

  g->fillRect(theme->colors.tabActiveFace(), bounds);

  for (int c=0; c<groups; ++c, ++it) {
    ToolGroup* tool_group = *it;
    Tool* tool = m_selectedInGroup[tool_group];
    SkinPartPtr nw;

    if (activeTool == tool || m_hotIndex == c) {
      nw = theme->parts.toolbuttonHot();
    }
    else {
      nw = c >= 0 && c < groups-1 ? theme->parts.toolbuttonNormal():
                                    theme->parts.toolbuttonLast();
    }

    toolrc = getToolGroupBounds(c);
    toolrc.offset(-origin());
    theme->drawRect(g, toolrc, nw.get());

    // Draw the tool icon
    os::Surface* icon = theme->getToolIcon(tool->getId().c_str());
    if (icon) {
      g->drawRgbaSurface(icon,
        toolrc.x+toolrc.w/2-icon->width()/2,
        toolrc.y+toolrc.h/2-icon->height()/2);
    }
  }

  // Draw button to show/hide preview
  toolrc = getToolGroupBounds(PreviewVisibilityIndex);
  toolrc.offset(-origin());
  bool isHot = (m_hotIndex == PreviewVisibilityIndex ||
    App::instance()->mainWindow()->getPreviewEditor()->isPreviewEnabled());
  theme->drawRect(
    g,
    toolrc,
    (isHot ? theme->parts.toolbuttonHot().get():
             theme->parts.toolbuttonLast().get()));

  os::Surface* icon = theme->getToolIcon("minieditor");
  if (icon) {
    g->drawRgbaSurface(icon,
      toolrc.x+toolrc.w/2-icon->width()/2,
      toolrc.y+toolrc.h/2-icon->height()/2);
  }
}

void ToolBar::onVisible(bool visible)
{
  Widget::onVisible(visible);
  if (!visible) {
    if (m_popupWindow) {
      closePopupWindow();
      closeTipWindow();
    }
  }
}

int ToolBar::getToolGroupIndex(ToolGroup* group)
{
  ToolBox* toolbox = App::instance()->toolBox();
  ToolGroupList::iterator it = toolbox->begin_group();
  int groups = toolbox->getGroupsCount();

  for (int c=0; c<groups; ++c, ++it) {
    if (group == *it)
      return c;
  }

  return -1;
}

void ToolBar::openPopupWindow(int group_index, ToolGroup* tool_group)
{
  if (m_popupWindow) {
    // If we've already open the given group, do nothing.
    if (m_currentStrip && m_currentStrip->toolGroup() == tool_group)
      return;

    if (m_closeConn)
      m_closeConn.disconnect();

    onClosePopup();
    closePopupWindow();
  }

  // Close tip window
  closeTipWindow();

  // If this group contains only one tool, do not show the popup
  ToolBox* toolbox = App::instance()->toolBox();
  int count = 0;
  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (tool->getGroup() == tool_group)
      ++count;
  }
  m_openOnHot = true;
  if (count <= 1)
    return;

  // In case this tool contains more than just one tool, show the popup window
  m_popupWindow = new TransparentPopupWindow(PopupWindow::ClickBehavior::CloseOnClickOutsideHotRegion);
  m_closeConn = m_popupWindow->Close.connect(base::Bind<void, ToolBar, ToolBar>(&ToolBar::onClosePopup, this));
  m_openedRecently = true;

  ToolStrip* toolstrip = new ToolStrip(tool_group, this);
  m_currentStrip = toolstrip;
  m_popupWindow->addChild(toolstrip);

  Rect rc = getToolGroupBounds(group_index);
  int w = 0;

  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (tool->getGroup() == tool_group)
      w += bounds().w-border().width()-1;
  }

  rc.x -= w;
  rc.w = w;

  // Set hotregion of popup window
  Region rgn(gfx::Rect(rc).enlarge(16*guiscale()));
  rgn.createUnion(rgn, Region(bounds()));
  m_popupWindow->setHotRegion(rgn);
  m_popupWindow->setAutoRemap(false);
  m_popupWindow->setBounds(rc);
  toolstrip->setBounds(rc);
  m_popupWindow->openWindow();

  toolstrip->setBounds(rc);
}

void ToolBar::closePopupWindow()
{
  if (m_popupWindow) {
    m_popupWindow->closeWindow(nullptr);
    delete m_popupWindow;
    m_popupWindow = nullptr;
  }
}

Rect ToolBar::getToolGroupBounds(int group_index)
{
  ToolBox* toolbox = App::instance()->toolBox();
  int groups = toolbox->getGroupsCount();
  Size iconsize = getToolIconSize(this);
  Rect rc(bounds());
  rc.shrink(border());

  switch (group_index) {

    case PreviewVisibilityIndex:
      rc.y += rc.h - iconsize.h - 2*guiscale();
      rc.h = iconsize.h+2*guiscale();
      break;

    default:
      rc.y += group_index*(iconsize.h-1*guiscale());
      rc.h = group_index < groups-1 ? iconsize.h+1*guiscale():
                                      iconsize.h+2*guiscale();
      break;
  }

  return rc;
}

Point ToolBar::getToolPositionInGroup(int group_index, Tool* tool)
{
  ToolBox* toolbox = App::instance()->toolBox();
  Size iconsize = getToolIconSize(this);
  int nth = 0;

  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    if (tool == *it)
      break;

    if ((*it)->getGroup() == tool->getGroup()) {
      ++nth;
    }
  }

  return Point(iconsize.w/2+iconsize.w*nth, iconsize.h);
}

void ToolBar::openTipWindow(ToolGroup* tool_group, Tool* tool)
{
  openTipWindow(getToolGroupIndex(tool_group), tool);
}

void ToolBar::openTipWindow(int group_index, Tool* tool)
{
  if (m_tipWindow)
    closeTipWindow();

  std::string tooltip;
  if (tool && group_index >= 0) {
    tooltip = tool->getText();
    if (tool->getTips().size() > 0) {
      tooltip += ":\n";
      tooltip += tool->getTips();
    }

    // Tool shortcut
    KeyPtr key = KeyboardShortcuts::instance()->tool(tool);
    if (key && !key->accels().empty()) {
      tooltip += "\n\n";
      tooltip += fmt::format(Strings::tools_shortcut(),
                             key->accels().front().toString());
    }
  }
  else if (group_index == PreviewVisibilityIndex) {
    if (App::instance()->mainWindow()->getPreviewEditor()->isPreviewEnabled())
      tooltip = Strings::tools_preview_hide();
    else
      tooltip = Strings::tools_preview_show();
  }
  else
    return;

  m_tipWindow = new TipWindow(tooltip);
  m_tipWindow->remapWindow();

  Rect toolrc = getToolGroupBounds(group_index);
  Point arrow = (tool ? getToolPositionInGroup(group_index, tool): Point(0, 0));
  if (tool && m_popupWindow && m_popupWindow->isVisible())
    toolrc.x += arrow.x - m_popupWindow->bounds().w;

  m_tipWindow->pointAt(TOP | RIGHT, toolrc);

  if (m_tipOpened)
    m_tipWindow->openWindow();
  else
    m_tipTimer.start();
}

void ToolBar::closeTipWindow()
{
  m_tipTimer.stop();

  if (m_tipWindow) {
    m_tipWindow->closeWindow(NULL);
    delete m_tipWindow;
    m_tipWindow = NULL;
  }
}

void ToolBar::selectTool(Tool* tool)
{
  ASSERT(tool);

  m_selectedInGroup[tool->getGroup()] = tool;

  // Inform to the active tool manager about this tool change.
  App::instance()->activeToolManager()->setSelectedTool(tool);

  if (m_currentStrip)
    m_currentStrip->invalidate();

  invalidate();
}

void ToolBar::selectToolGroup(tools::ToolGroup* toolGroup)
{
  ASSERT(toolGroup);
  ASSERT(m_selectedInGroup[toolGroup]);
  if (m_selectedInGroup[toolGroup])
    selectTool(m_selectedInGroup[toolGroup]);
}

void ToolBar::onClosePopup()
{
  closeTipWindow();

  if (!hasMouse())
    m_tipOpened = false;

  m_openOnHot = false;
  m_hotTool = NULL;
  m_hotIndex = NoneIndex;
  m_currentStrip = NULL;

  invalidate();
}

//////////////////////////////////////////////////////////////////////
// ToolStrip
//////////////////////////////////////////////////////////////////////

ToolBar::ToolStrip::ToolStrip(ToolGroup* group, ToolBar* toolbar)
  : Widget(kGenericWidget)
{
  m_group = group;
  m_hotTool = NULL;
  m_toolbar = toolbar;

  setDoubleBuffered(true);
  setTransparent(true);
}

ToolBar::ToolStrip::~ToolStrip()
{
}

bool ToolBar::ToolStrip::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseDownMessage:
      captureMouse();
      // fallthrough

    case kMouseMoveMessage: {
      MouseMessage* mouseMsg = static_cast<MouseMessage*>(msg);
      gfx::Point mousePos = mouseMsg->position();
      ToolBox* toolbox = App::instance()->toolBox();
      Tool* hot_tool = NULL;
      Rect toolrc;
      int index = 0;

      for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
        Tool* tool = *it;
        if (tool->getGroup() == m_group) {
          toolrc = getToolBounds(index++);
          if (toolrc.contains(Point(mousePos.x, mousePos.y))) {
            hot_tool = tool;
            break;
          }
        }
      }

      // Hot button changed
      if (m_hotTool != hot_tool) {
        m_hotTool = hot_tool;
        invalidate();

        // Show the tooltip for the hot tool
        if (m_hotTool && !hasCapture())
          m_toolbar->openTipWindow(m_group, m_hotTool);
        else
          m_toolbar->closeTipWindow();

        if (m_hotTool)
          StatusBar::instance()->showTool(0, m_hotTool);
      }

      if (hasCapture()) {
        if (m_hotTool)
          m_toolbar->selectTool(m_hotTool);

        Widget* pick = manager()->pick(mouseMsg->position());
        if (ToolBar* bar = dynamic_cast<ToolBar*>(pick)) {
          releaseMouse();

          MouseMessage* mouseMsg2 = new MouseMessage(
            kMouseDownMessage,
            mouseMsg->pointerType(),
            mouseMsg->button(),
            mouseMsg->modifiers(),
            mouseMsg->position());
          mouseMsg2->setRecipient(bar);
          manager()->enqueueMessage(mouseMsg2);
        }
      }
      break;
    }

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();
        closeWindow();
      }
      break;

  }
  return Widget::onProcessMessage(msg);
}

void ToolBar::ToolStrip::onSizeHint(SizeHintEvent& ev)
{
  ToolBox* toolbox = App::instance()->toolBox();
  int c = 0;

  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (tool->getGroup() == m_group) {
      ++c;
    }
  }

  Size iconsize = getToolIconSize(this);
  ev.setSizeHint(Size(iconsize.w * c, iconsize.h));
}

void ToolBar::ToolStrip::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  ToolBox* toolbox = App::instance()->toolBox();
  Tool* activeTool = App::instance()->activeTool();
  Rect toolrc;
  int index = 0;

  for (ToolIterator it = toolbox->begin(); it != toolbox->end(); ++it) {
    Tool* tool = *it;
    if (tool->getGroup() == m_group) {
      SkinPartPtr nw;

      if (activeTool == tool || m_hotTool == tool) {
        nw = theme->parts.toolbuttonHot();
      }
      else {
        nw = theme->parts.toolbuttonLast();
      }

      toolrc = getToolBounds(index++);
      toolrc.offset(-bounds().x, -bounds().y);
      theme->drawRect(g, toolrc, nw.get());

      // Draw the tool icon
      os::Surface* icon = theme->getToolIcon(tool->getId().c_str());
      if (icon) {
        g->drawRgbaSurface(
          icon,
          toolrc.x+toolrc.w/2-icon->width()/2,
          toolrc.y+toolrc.h/2-icon->height()/2);
      }
    }
  }
}

Rect ToolBar::ToolStrip::getToolBounds(int index)
{
  const Rect& bounds(this->bounds());
  Size iconsize = getToolIconSize(this);

  return Rect(bounds.x+index*(iconsize.w-1), bounds.y,
              iconsize.w, bounds.h);
}

void ToolBar::onActiveToolChange(tools::Tool* tool)
{
  invalidate();
}

void ToolBar::onSelectedToolChange(tools::Tool* tool)
{
  if (tool && m_selectedInGroup[tool->getGroup()] != tool)
    m_selectedInGroup[tool->getGroup()] = tool;

  invalidate();
}

} // namespace app
