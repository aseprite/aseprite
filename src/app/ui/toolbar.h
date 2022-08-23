// Aseprite
// Copyright (C) 2021-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TOOLBAR_H_INCLUDED
#define APP_UI_TOOLBAR_H_INCLUDED
#pragma once

#include "app/tools/active_tool_observer.h"
#include "app/ui/dockable.h"
#include "app/ui/skin/skin_part.h"
#include "gfx/point.h"
#include "obs/connection.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <map>
#include <memory>

namespace ui {
class CloseEvent;
class PopupWindow;
class TipWindow;
} // namespace ui

namespace app {
namespace tools {
class Tool;
class ToolGroup;
} // namespace tools

// Class to show selected tools for each tool (vertically)
class ToolBar : public ui::Widget,
                public Dockable,
                public tools::ActiveToolObserver {
  static ToolBar* m_instance;

public:
  static ToolBar* instance() { return m_instance; }

  static const int NoneIndex = -1;
  static const int PreviewVisibilityIndex = -2;
  static const int TimelineVisibilityIndex = -3;

  ToolBar();
  ~ToolBar();

  bool isToolVisible(tools::Tool* tool);
  void selectTool(tools::Tool* tool);
  void selectToolGroup(tools::ToolGroup* toolGroup);

  void openTipWindow(tools::ToolGroup* toolGroup, tools::Tool* tool);
  void closeTipWindow();

  // Dockable impl
  int dockableAt() const override
  {
    // TODO add future support to dock the tool bar at the
    // top/bottom sides
    return ui::LEFT | ui::RIGHT;
  }

protected:
  bool onProcessMessage(ui::Message* msg) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onResize(ui::ResizeEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;
  void onVisible(bool visible) override;

private:
  enum class GroupType { Regular, Overflow };

  bool isDockedAtLeftSide() const;
  int getToolGroupIndex(tools::ToolGroup* group);
  void openPopupWindow(GroupType group_type,
                       int group_index = 0,
                       tools::ToolGroup* tool_group = nullptr);
  void closePopupWindow();
  gfx::Rect getToolGroupBounds(int group_index);
  gfx::Point getToolPositionInGroup(const tools::Tool* tool) const;
  void openTipWindow(int group_index, tools::Tool* tool);
  void onClosePopup();
  void drawToolIcon(ui::Graphics* g, int group_index, skin::SkinPartPtr skin, os::Surface* icon);
  int getHiddenGroups() const;

  // ActiveToolObserver impl
  void onActiveToolChange(tools::Tool* tool) override;
  void onSelectedToolChange(tools::Tool* tool) override;

  // What tool is selected for each tool-group
  std::map<const tools::ToolGroup*, tools::Tool*> m_selectedInGroup;

  // Index of the tool group or special button highlighted.
  int m_hotIndex;

  // What tool has the mouse above
  tools::Tool* m_hotTool;

  // True if the popup-window must be opened when a tool-button is hot
  bool m_openOnHot;

  // True if the last MouseDown opened the popup. This is used to
  // close the popup with a second MouseUp event.
  bool m_openedRecently;

  // Window displayed to show a tool-group
  std::unique_ptr<ui::PopupWindow> m_popupWindow;
  class ToolStrip;
  ToolStrip* m_currentStrip = nullptr;

  // Tool-tip window
  std::unique_ptr<ui::TipWindow> m_tipWindow;

  ui::Timer m_tipTimer;
  bool m_tipOpened;

  // Minimum height for all the top toolbar buttons to be visible.
  int m_minHeight;

  obs::connection m_closeConn;
};

} // namespace app

#endif
