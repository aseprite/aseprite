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

#ifndef APP_UI_WORKSPACE_H_INCLUDED
#define APP_UI_WORKSPACE_H_INCLUDED

#include "app/ui/workspace_views.h"
#include "base/signal.h"
#include "ui/box.h"
#include <vector>

namespace app {
  class WorkspacePart;

  class Workspace : public ui::Box {
  public:
    typedef WorkspaceViews::iterator iterator;

    Workspace();
    ~Workspace();

    iterator begin() { return m_views.begin(); }
    iterator end() { return m_views.end(); }

    void addView(WorkspaceView* view);
    void removeView(WorkspaceView* view);

    WorkspaceView* getActiveView();
    void setActiveView(WorkspaceView* view);

    void splitView(WorkspaceView* view, int orientation);
    void makeUnique(WorkspaceView* view);

    Signal0<void> ActiveViewChanged;

  private:
    typedef std::vector<WorkspacePart*> WorkspaceParts;

    WorkspacePart* destroyPart(WorkspacePart* part);
    WorkspacePart* getPartByView(WorkspaceView* view);
    void enumAllParts(WorkspaceParts& parts);

    // All views of all parts.
    WorkspaceViews m_views;
    WorkspacePart* m_activePart;
  };

} // namespace app

#endif
