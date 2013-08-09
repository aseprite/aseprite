/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_UI_TOOLBAR_H_INCLUDED
#define APP_UI_TOOLBAR_H_INCLUDED

#include "base/compiler_specific.h"
#include "gfx/point.h"
#include "ui/timer.h"
#include "ui/widget.h"

#include <map>

namespace ui {
  class PopupWindow;
  class TipWindow;
}

namespace app {
  namespace tools {
    class Tool;
    class ToolGroup;
  }

  // Class to show selected tools for each tool (vertically)
  class ToolBar : public ui::Widget {
    static ToolBar* m_instance;
  public:
    static ToolBar* instance() { return m_instance; }

    static const int NoneIndex = -1;
    static const int ConfigureToolIndex = -2;
    static const int MiniEditorVisibilityIndex = -3;

    ToolBar();
    ~ToolBar();

    bool isToolVisible(tools::Tool* tool);
    void selectTool(tools::Tool* tool);

    void openTipWindow(tools::ToolGroup* tool_group, tools::Tool* tool);
    void closeTipWindow();

  protected:
    bool onProcessMessage(ui::Message* msg) OVERRIDE;
    void onPreferredSize(ui::PreferredSizeEvent& ev) OVERRIDE;

  private:
    int getToolGroupIndex(tools::ToolGroup* group);
    void openPopupWindow(int group_index, tools::ToolGroup* group);
    gfx::Rect getToolGroupBounds(int group_index);
    gfx::Point getToolPositionInGroup(int group_index, tools::Tool* tool);
    void openTipWindow(int group_index, tools::Tool* tool);
    void onClosePopup();

    // What tool is selected for each tool-group
    std::map<const tools::ToolGroup*, tools::Tool*> m_selected_in_group;

    // Index of the tool group or special button highlighted.
    int m_hot_index;

    // What tool has the mouse above
    tools::Tool* m_hot_tool;

    // True if the popup-window must be opened when a tool-button is hot
    bool m_open_on_hot;

    // Window displayed to show a tool-group
    ui::PopupWindow* m_popupWindow;

    // Tool-tip window
    ui::TipWindow* m_tipWindow;

    ui::Timer m_tipTimer;
    bool m_tipOpened;
  };

} // namespace app

#endif
