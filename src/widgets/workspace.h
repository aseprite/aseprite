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

#ifndef WIDGETS_WORKSPACE_H_INCLUDED
#define WIDGETS_WORKSPACE_H_INCLUDED

#include "ui/box.h"
#include <vector>

namespace widgets {

  class WorkspaceView;
  typedef std::vector<WorkspaceView*> WorkspaceViews;

  class Workspace : public ui::Box
  {
  public:
    Workspace();
    ~Workspace();

    void addView(WorkspaceView* view);
    void removeView(WorkspaceView* view);

    WorkspaceView* getActiveView() { return m_activeView; }
    void setActiveView(WorkspaceView* view);

    void splitView(WorkspaceView* view, int orientation);
    void closeView(WorkspaceView* view);
    void makeUnique(WorkspaceView* view);

  private:
    WorkspaceView* m_activeView;
    WorkspaceViews m_views;
  };

} // namespace widgets

#endif
