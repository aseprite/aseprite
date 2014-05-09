/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_UI_DEVCONSOLE_VIEW_H_INCLUDED
#define APP_UI_DEVCONSOLE_VIEW_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "base/compiler_specific.h"
#include "ui/box.h"
#include "ui/label.h"
#include "ui/textbox.h"
#include "ui/view.h"

namespace app {
  class DevConsoleView : public ui::Box
                       , public TabView
                       , public WorkspaceView {
  public:
    DevConsoleView();
    ~DevConsoleView();

    // TabView implementation
    std::string getTabText() OVERRIDE;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() OVERRIDE { return this; }
    WorkspaceView* cloneWorkspaceView() OVERRIDE;
    void onWorkspaceViewSelected() OVERRIDE;
    void onClonedFrom(WorkspaceView* from) OVERRIDE;

  protected:
    bool onProcessMessage(ui::Message* msg) OVERRIDE;
    void onExecuteCommand(const std::string& cmd);

  private:
    class CommmandEntry;

    ui::View m_view;
    ui::TextBox m_textBox;
    ui::HBox m_bottomBox;
    ui::Label m_label;
    CommmandEntry* m_entry;
  };

} // namespace app

#endif
