/* ASEPRITE
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

#ifndef APP_UI_WORKSPACE_PART_H_INCLUDED
#define APP_UI_WORKSPACE_PART_H_INCLUDED

#include "app/ui/workspace_views.h"
#include "ui/box.h"
#include <vector>

namespace app {

  class WorkspacePart : public ui::Box {
  public:
    WorkspacePart();
    ~WorkspacePart();

    size_t getViewCount() { return m_views.size(); }

    void addView(WorkspaceView* view);
    void removeView(WorkspaceView* view);

    WorkspaceView* getActiveView() { return m_activeView; }
    void setActiveView(WorkspaceView* view);

  private:
    WorkspaceView* m_activeView;
    WorkspaceViews m_views;
  };

} // namespace app

#endif
