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

#ifndef APP_UI_START_VIEW_H_INCLUDED
#define APP_UI_START_VIEW_H_INCLUDED
#pragma once

#include "app/ui/tabs.h"
#include "app/ui/workspace_view.h"
#include "ui/box.h"

namespace ui {
  class View;
}

namespace app {
  class StartView : public ui::Box
                  , public TabView
                  , public WorkspaceView {
  public:
    StartView();
    ~StartView();

    // TabView implementation
    std::string getTabText() override;

    // WorkspaceView implementation
    ui::Widget* getContentWidget() override { return this; }
    WorkspaceView* cloneWorkspaceView() override;
    void onClonedFrom(WorkspaceView* from) override;
  };

} // namespace app

#endif
