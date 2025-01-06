// Aseprite
// Copyright (C) 2018-2025  Igara Studio S.A.
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
#include "app/modules/gfx.h"
#include "app/tools/active_tool.h"
#include "app/tools/tool_box.h"
#include "app/tools/tool_group.h"
#include "app/ui/keyboard_shortcuts.h"
#include "app/ui/main_window.h"
#include "app/ui/preview_editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "fmt/format.h"
#include "gfx/size.h"
#include "obs/signal.h"
#include "os/surface.h"
#include "ui/ui.h"

#include <algorithm>
#include <string>
#include <vector>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;
using namespace tools;

// Class to show a group of tools (horizontally)
// This widget is inside the ToolBar::m_popupWindow
class ToolBar::ToolStrip : public Widget {
public:
  using Tools = std::vector<Tool*>;

  ToolStrip(ToolBar* toolbar, const Tools& tools, ToolGroup* group);
  ~ToolStrip();

  ToolGroup* toolGroup() { return m_group; }
  const Tools& tools() const { return m_tools; }

  obs::signal<void(Tool*)> ToolSelected;

protected:
  bool onProcessMessage(Message* msg) override;
  void onSizeHint(SizeHintEvent& ev) override;
  void onPaint(PaintEvent& ev) override;

private:
  Rect getToolBounds(int index);

  ToolBar* m_toolbar;
  Tools m_tools;
  ToolGroup* m_group;
  Tool* m_hotTool;
};

static Size getToolIconSize(const Widget* widget)
{
  auto theme = SkinTheme::get(widget);
  os::Surface* icon = theme->getToolIcon("configuration");
  if (icon)
    return Size(icon->width(), icon->height());
  else
    return Size(16, 16) * guiscale();
}

//////////////////////////////////////////////////////////////////////
// ToolBar

ToolBar* ToolBar::m_instance = NULL;

ToolBar::ToolBar() : Widget(kGenericWidget), m_openedRecently(false), m_tipTimer(300, this)
{
  m_instance = this;

  setBorder(gfx::Border(1 * guiscale(), 0, 1 * guiscale(), 0));

  m_hotTool = NULL;
  m_hotIndex = NoneIndex;
  m_openOnHot = false;
  m_popupWindow = NULL;
  m_currentStrip = NULL;
  m_tipWindow = NULL;
  m_tipOpened = false;
  m_minHeight = 0;

  ToolBox* toolbox = App::instance()->toolBox();
  for (Tool* tool : *toolbox) {
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
      auto mouseMsg = static_cast<const MouseMessage*>(msg);
      const Point mousePos = mouseMsg->positionForDisplay(display());
      ToolBox* toolbox = App::instance()->toolBox();
      int hidden = getHiddenGroups();
      int groups = toolbox->getGroupsCount() - hidden;
      Rect toolrc;

      ToolGroupList::iterator it = toolbox->begin_group();
      for (int c = 0; c < groups; ++c, ++it) {
        ToolGroup* tool_group = *it;
        Tool* tool = m_selectedInGroup[tool_group];

        toolrc = getToolGroupBounds(c);
        if (mousePos.y >= toolrc.y && mousePos.y < toolrc.y + toolrc.h) {
          selectTool(tool);

          openPopupWindow(GroupType::Regular, c, tool_group);

          // We capture the mouse so the user can continue navigating
          // the ToolBar to open other groups while he is pressing the
          // mouse button.
          captureMouse();
        }
      }

      if (hidden > 0) {
        toolrc = getToolGroupBounds(groups);
        if (mousePos.y >= toolrc.y && mousePos.y < toolrc.y + toolrc.h) {
          // Show (or close if it's already visible) the
          // hidden/overflow group of tools.
          if (m_popupWindow) {
            closePopupWindow();
            closeTipWindow();
          }
          else {
            openPopupWindow(GroupType::Overflow, groups);
          }
        }
      }

      toolrc = getToolGroupBounds(PreviewVisibilityIndex);
      if (mousePos.y >= toolrc.y && mousePos.y < toolrc.y + toolrc.h) {
        // Toggle preview visibility
        PreviewEditorWindow* preview = App::instance()->mainWindow()->getPreviewEditor();
        bool state = preview->isPreviewEnabled();
        preview->setPreviewEnabled(!state);
      }

      toolrc = getToolGroupBounds(TimelineVisibilityIndex);
      if (mousePos.y >= toolrc.y && mousePos.y < toolrc.y + toolrc.h) {
        // Toggle timeline visibility
        bool state = App::instance()->mainWindow()->getTimelineVisibility();
        App::instance()->mainWindow()->setTimelineVisibility(!state);
      }
      break;
    }

    case kMouseMoveMessage: {
      auto mouseMsg = static_cast<const MouseMessage*>(msg);
      const Point mousePos = mouseMsg->positionForDisplay(display());
      ToolBox* toolbox = App::instance()->toolBox();
      int hidden = getHiddenGroups();
      int groups = toolbox->getGroupsCount() - hidden;
      Tool* new_hot_tool = NULL;
      int new_hot_index = NoneIndex;
      Rect toolrc;

      ToolGroupList::iterator it = toolbox->begin_group();

      for (int c = 0; c < groups; ++c, ++it) {
        ToolGroup* tool_group = *it;
        Tool* tool = m_selectedInGroup[tool_group];

        toolrc = getToolGroupBounds(c);
        if (mousePos.y >= toolrc.y && mousePos.y < toolrc.y + toolrc.h) {
          new_hot_tool = tool;
          new_hot_index = c;

          if ((m_openOnHot) && (m_hotTool != new_hot_tool) && hasCapture()) {
            openPopupWindow(GroupType::Regular, c, tool_group);
          }
          break;
        }
      }

      if (hidden > 0) {
        toolrc = getToolGroupBounds(groups);
        if (mousePos.y >= toolrc.y && mousePos.y < toolrc.y + toolrc.h) {
          // Mouse over the overflow button
          new_hot_index = groups;
        }
      }

      toolrc = getToolGroupBounds(PreviewVisibilityIndex);
      if (mousePos.y >= toolrc.y && mousePos.y < toolrc.y + toolrc.h) {
        new_hot_index = PreviewVisibilityIndex;
      }

      toolrc = getToolGroupBounds(TimelineVisibilityIndex);
      if (mousePos.y >= toolrc.y && mousePos.y < toolrc.y + toolrc.h) {
        new_hot_index = TimelineVisibilityIndex;
      }

      // hot button changed
      if (new_hot_tool != m_hotTool || new_hot_index != m_hotIndex) {
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
        Manager* mgr = manager();
        Widget* pick = mgr->pickFromScreenPos(mouseMsg->screenPosition());
        if (ToolStrip* strip = dynamic_cast<ToolStrip*>(pick)) {
          mgr->transferAsMouseDownMessage(this, strip, mouseMsg);
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
      [[fallthrough]];

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

  if (m_popupWindow) {
    closePopupWindow();
    closeTipWindow();
  }
}

void ToolBar::onResize(ui::ResizeEvent& ev)
{
  Widget::onResize(ev);

  auto* toolbox = App::instance()->toolBox();
  auto lastToolBounds = getToolGroupBounds(toolbox->getGroupsCount());
  m_minHeight = lastToolBounds.y2() -
                origin().y
                // Preview and timeline buttons
                + 2 * (getToolIconSize(this).h - 1) + 3 * guiscale();
}

void ToolBar::onPaint(ui::PaintEvent& ev)
{
  gfx::Rect bounds = clientBounds();
  Graphics* g = ev.graphics();
  auto theme = SkinTheme::get(this);
  ToolBox* toolbox = App::instance()->toolBox();
  Tool* activeTool = App::instance()->activeTool();
  ToolGroupList::iterator it = toolbox->begin_group();
  int groups = toolbox->getGroupsCount();
  Rect toolrc;
  SkinPartPtr nw;
  os::Surface* icon;

  g->fillRect(theme->colors.tabActiveFace(), bounds);

  int hiddenGroups = getHiddenGroups();
  int visibleGroupCount = groups - hiddenGroups;
  for (int c = 0; c < visibleGroupCount; ++c, ++it) {
    ToolGroup* tool_group = *it;
    Tool* tool = m_selectedInGroup[tool_group];

    if (activeTool == tool || m_hotIndex == c) {
      nw = theme->parts.toolbuttonHot();
    }
    else {
      nw = c >= 0 && c < groups - 1 ? theme->parts.toolbuttonNormal() :
                                      theme->parts.toolbuttonLast();
    }

    // Draw the tool icon
    icon = theme->getToolIcon(tool->getId().c_str());
    drawToolIcon(g, c, nw, icon);
  }

  if (hiddenGroups > 0) {
    nw = (m_hotIndex >= visibleGroupCount) ? theme->parts.toolbuttonHot() :
                                             theme->parts.toolbuttonLast();

    icon = theme->parts.listView()->bitmap(0);

    drawToolIcon(g, visibleGroupCount, nw, icon);
  }

  // Draw button to show/hide preview
  const auto mainWindow = App::instance()->mainWindow();
  bool isHot = (m_hotIndex == PreviewVisibilityIndex ||
                mainWindow->getPreviewEditor()->isPreviewEnabled());
  nw = isHot ? theme->parts.toolbuttonHot() : theme->parts.toolbuttonNormal();
  icon = theme->getToolIcon("minieditor");
  drawToolIcon(g, PreviewVisibilityIndex, nw, icon);

  // Draw button to show/hide timeline
  isHot = (m_hotIndex == TimelineVisibilityIndex || mainWindow->getTimelineVisibility());
  nw = isHot ? theme->parts.toolbuttonHot() : theme->parts.toolbuttonLast();
  icon = theme->getToolIcon("timeline");
  drawToolIcon(g, TimelineVisibilityIndex, nw, icon);
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

  for (int c = 0; c < groups; ++c, ++it) {
    if (group == *it)
      return c;
  }

  return -1;
}

void ToolBar::openPopupWindow(GroupType group_type, int group_index, tools::ToolGroup* tool_group)
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

  // Here we build the list of tools to show in the ToolStrip widget
  ToolBox* toolbox = App::instance()->toolBox();
  ToolStrip::Tools tools;
  switch (group_type) {
    case GroupType::Regular:
      for (Tool* tool : *toolbox) {
        if (tool->getGroup() == tool_group)
          tools.push_back(tool);
      }
      break;

    case GroupType::Overflow: {
      ToolGroupList::iterator it = toolbox->begin_group();
      for (int i = 0; i < toolbox->getGroupsCount(); ++i, ++it) {
        if (i < toolbox->getGroupsCount() - getHiddenGroups())
          continue;

        ToolGroup* it_group = *it;
        for (Tool* tool : *toolbox) {
          if (tool->getGroup() == it_group)
            tools.push_back(tool);
        }
      }
      break;
    }
  }

  // If this group contains only one tool, do not show the popup
  if (tools.size() <= 1)
    return;

  // In case this tool contains more than just one tool, show the popup window
  m_openOnHot = true;
  m_popupWindow = new TransparentPopupWindow(
    PopupWindow::ClickBehavior::CloseOnClickOutsideHotRegion);
  m_closeConn = m_popupWindow->Close.connect([this] { onClosePopup(); });
  m_openedRecently = true;

  ToolStrip* toolstrip = new ToolStrip(this, tools, tool_group);
  m_currentStrip = toolstrip;
  m_popupWindow->addChild(toolstrip);

  Rect rc = getToolGroupBounds(group_index);
  int w = 0;
  for (const auto* tool : tools) {
    (void)tool;
    w += bounds().w - border().width() - 1 * guiscale();
  }

  rc.x -= w;
  rc.w = w;

  // Set hotregion of popup window
  m_popupWindow->setAutoRemap(false);
  ui::fit_bounds(display(), m_popupWindow, rc);
  m_popupWindow->setBounds(rc);

  Region rgn(m_popupWindow->boundsOnScreen().enlarge(16 * guiscale()));
  rgn.createUnion(rgn, Region(boundsOnScreen()));
  m_popupWindow->setHotRegion(rgn);

  m_popupWindow->openWindow();
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
      rc.y += rc.h - 2 * iconsize.h - 2 * guiscale();
      rc.h = iconsize.h + 2 * guiscale();
      break;

    case TimelineVisibilityIndex:
      rc.y += rc.h - iconsize.h - 2 * guiscale();
      rc.h = iconsize.h + 2 * guiscale();
      break;

    default:
      rc.y += group_index * (iconsize.h - 1 * guiscale());
      rc.h = group_index < groups - 1 ? iconsize.h + 1 * guiscale() : iconsize.h + 2 * guiscale();
      break;
  }

  return rc;
}

Point ToolBar::getToolPositionInGroup(const Tool* tool) const
{
  if (!m_currentStrip)
    return Point(0, 0);

  const Size iconsize = getToolIconSize(this);
  const auto& tools = m_currentStrip->tools();
  const int nth = std::find(tools.begin(), tools.end(), tool) - tools.begin();

  return Point(iconsize.w / 2 + nth * (iconsize.w - 1 * guiscale()), iconsize.h);
}

void ToolBar::openTipWindow(ToolGroup* tool_group, Tool* tool)
{
  openTipWindow(getToolGroupIndex(tool_group), tool);
}

void ToolBar::openTipWindow(int group_index, Tool* tool)
{
  if (m_tipWindow)
    closeTipWindow();

  int hidden = getHiddenGroups();
  int groups = App::instance()->toolBox()->getGroupsCount() - hidden;

  std::string tooltip;
  if (!tool && hidden > 0 && group_index >= groups) {
    tooltip = Strings::general_show_more();
  }
  else if (tool && group_index >= 0) {
    tooltip = tool->getText();
    if (tool->getTips().size() > 0) {
      tooltip += ":\n";
      tooltip += tool->getTips();
    }

    // Tool shortcut
    KeyPtr key = KeyboardShortcuts::instance()->tool(tool);
    if (key && !key->shortcuts().empty()) {
      tooltip += "\n\n";
      tooltip += Strings::tools_shortcut(key->shortcuts().front().toString());
    }
  }
  else if (group_index == PreviewVisibilityIndex) {
    if (App::instance()->mainWindow()->getPreviewEditor()->isPreviewEnabled())
      tooltip = Strings::tools_preview_hide();
    else
      tooltip = Strings::tools_preview_show();
  }
  else if (group_index == TimelineVisibilityIndex) {
    if (App::instance()->mainWindow()->getTimelineVisibility())
      tooltip = Strings::tools_timeline_hide();
    else
      tooltip = Strings::tools_timeline_show();
  }
  else
    return;

  m_tipWindow = new TipWindow(tooltip);
  m_tipWindow->remapWindow();

  Rect toolrc = getToolGroupBounds(group_index);
  Point arrow = (tool ? getToolPositionInGroup(tool) : Point(0, 0));
  if (tool && m_popupWindow && m_popupWindow->isVisible())
    toolrc.x += arrow.x - m_popupWindow->bounds().w;

  m_tipWindow->pointAt(TOP | RIGHT, toolrc, ui::Manager::getDefault()->display());

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

void ToolBar::drawToolIcon(Graphics* g, int group_index, SkinPartPtr skin, os::Surface* icon)
{
  auto theme = SkinTheme::get(this);
  Rect toolrc = getToolGroupBounds(group_index);
  toolrc.offset(-origin());

  theme->drawRect(g, toolrc, skin.get());

  if (icon) {
    g->drawRgbaSurface(icon,
                       guiscaled_center(toolrc.x, toolrc.w, icon->width()),
                       guiscaled_center(toolrc.y, toolrc.h, icon->height()));
  }
}

int ToolBar::getHiddenGroups() const
{
  auto* toolbox = App::instance()->toolBox();
  const int height = size().h;
  if (height < m_minHeight) {
    int hidden = (m_minHeight - height) / (getToolIconSize(this).h - 1 * guiscale());
    if (hidden >= 1)
      return std::clamp(hidden + 1, 2, toolbox->getGroupsCount());
  }
  return 0;
}

//////////////////////////////////////////////////////////////////////
// ToolStrip
//////////////////////////////////////////////////////////////////////

ToolBar::ToolStrip::ToolStrip(ToolBar* toolbar, const Tools& tools, ToolGroup* group)
  : Widget(kGenericWidget)
  , m_toolbar(toolbar)
  , m_tools(tools)
  , m_group(group)
  , m_hotTool(nullptr)
{
  setDoubleBuffered(true);
  setTransparent(true);
}

ToolBar::ToolStrip::~ToolStrip()
{
}

bool ToolBar::ToolStrip::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kMouseDownMessage: captureMouse(); [[fallthrough]];

    case kMouseMoveMessage: {
      auto mouseMsg = static_cast<const MouseMessage*>(msg);
      const Point mousePos = mouseMsg->positionForDisplay(display());
      Tool* hot_tool = NULL;
      Rect toolrc;
      int index = 0;

      for (Tool* tool : m_tools) {
        toolrc = getToolBounds(index++);
        if (toolrc.contains(Point(mousePos.x, mousePos.y))) {
          hot_tool = tool;
          break;
        }
      }

      // Hot button changed
      if (m_hotTool != hot_tool) {
        m_hotTool = hot_tool;
        invalidate();

        // Show the tooltip for the hot tool
        if (m_hotTool && !hasCapture()) {
          if (m_group)
            m_toolbar->openTipWindow(m_group, m_hotTool);
          else {
            int groups = App::instance()->toolBox()->getGroupsCount() -
                         m_toolbar->getHiddenGroups();
            m_toolbar->openTipWindow(groups, m_hotTool);
          }
        }
        else
          m_toolbar->closeTipWindow();

        if (m_hotTool)
          StatusBar::instance()->showTool(0, m_hotTool);
      }

      if (hasCapture()) {
        if (m_hotTool)
          m_toolbar->selectTool(m_hotTool);

        Manager* mgr = manager();
        Widget* pick = mgr->pickFromScreenPos(mouseMsg->screenPosition());
        if (ToolBar* bar = dynamic_cast<ToolBar*>(pick)) {
          mgr->transferAsMouseDownMessage(this, bar, mouseMsg);
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
  Size iconsize = getToolIconSize(this);
  ev.setSizeHint(Size(iconsize.w * m_tools.size(), iconsize.h));
}

void ToolBar::ToolStrip::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  auto theme = SkinTheme::get(this);
  Tool* activeTool = App::instance()->activeTool();
  Rect toolrc;
  int index = 0;

  for (Tool* tool : m_tools) {
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
      g->drawRgbaSurface(icon,
                         guiscaled_center(toolrc.x, toolrc.w, icon->width()),
                         guiscaled_center(toolrc.y, toolrc.h, icon->height()));
    }
  }
}

Rect ToolBar::ToolStrip::getToolBounds(int index)
{
  const Rect& bounds(this->bounds());
  Size iconsize = getToolIconSize(this);

  return Rect(bounds.x + index * (iconsize.w - 1 * guiscale()), bounds.y, iconsize.w, bounds.h);
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
