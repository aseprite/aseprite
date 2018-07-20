// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TOOLBAR_H_INCLUDED
#define APP_UI_TOOLBAR_H_INCLUDED
#pragma once

#include "app/tools/active_tool_observer.h"
#include "gfx/point.h"
#include "obs/connection.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <map>

namespace ui {
  class CloseEvent;
  class PopupWindow;
  class TipWindow;
}

namespace app {
  namespace tools {
    class Tool;
    class ToolGroup;
  }

  // Class to show selected tools for each tool (vertically)
  class ToolBar : public ui::Widget
                , public tools::ActiveToolObserver {
    static ToolBar* m_instance;
  public:
    static ToolBar* instance() { return m_instance; }

    static const int NoneIndex = -1;
    static const int PreviewVisibilityIndex = -2;

    ToolBar();
    ~ToolBar();

    bool isToolVisible(tools::Tool* tool);
    void selectTool(tools::Tool* tool);
    void selectToolGroup(tools::ToolGroup* toolGroup);

    void openTipWindow(tools::ToolGroup* toolGroup, tools::Tool* tool);
    void closeTipWindow();

  protected:
    bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;
    void onVisible(bool visible) override;

  private:
    int getToolGroupIndex(tools::ToolGroup* group);
    void openPopupWindow(int group_index, tools::ToolGroup* group);
    void closePopupWindow();
    gfx::Rect getToolGroupBounds(int group_index);
    gfx::Point getToolPositionInGroup(int group_index, tools::Tool* tool);
    void openTipWindow(int group_index, tools::Tool* tool);
    void onClosePopup();

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
    ui::PopupWindow* m_popupWindow;
    class ToolStrip;
    ToolStrip* m_currentStrip;

    // Tool-tip window
    ui::TipWindow* m_tipWindow;

    ui::Timer m_tipTimer;
    bool m_tipOpened;

    obs::connection m_closeConn;
  };

} // namespace app

#endif
