/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include <allegro/unicode.h>

#include "gui/frame.h"

#include "commands/command.h"
#include "context.h"
#include "app.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "settings/settings.h"
#include "widgets/statebar.h"
#include "ui_context.h"

using namespace gfx;

//////////////////////////////////////////////////////////////////////
// show_grid

class ShowGridCommand : public Command
{
public:
  ShowGridCommand()
    : Command("ShowGrid",
              "Show Grid",
              CmdUIOnlyFlag)
  {
  }

  Command* clone() const { return new ShowGridCommand(*this); }

protected:
  bool onChecked(Context* context)
  {
    ISettings* settings = context->getSettings();

    return settings->getGridVisible();
  }

  void onExecute(Context* context)
  {
    ISettings* settings = context->getSettings();

    settings->setGridVisible(settings->getGridVisible() ? false: true);
    refresh_all_editors();
  }
};

//////////////////////////////////////////////////////////////////////
// snap_to_grid

class SnapToGridCommand : public Command
{
public:
  SnapToGridCommand()
    : Command("SnapToGrid",
              "Snap to Grid",
              CmdUIOnlyFlag)
  {
  }

  Command* clone() const { return new SnapToGridCommand(*this); }

protected:
  bool onChecked(Context* context)
  {
    ISettings* settings = context->getSettings();

    return settings->getSnapToGrid();
  }

  void onExecute(Context* context)
  {
    ISettings* settings = context->getSettings();
    char buf[512];

    settings->setSnapToGrid(settings->getSnapToGrid() ? false: true);
    refresh_all_editors();

    usprintf(buf, "Snap to grid: %s",
             (settings->getSnapToGrid() ? "On": "Off"));

    app_get_statusbar()->setStatusText(250, buf);
  }
};

//////////////////////////////////////////////////////////////////////
// grid_settings

class GridSettingsCommand : public Command
{
public:
  GridSettingsCommand();
  Command* clone() const { return new GridSettingsCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

GridSettingsCommand::GridSettingsCommand()
  : Command("GridSettings",
            "Grid Settings",
            CmdUIOnlyFlag)
{
}

bool GridSettingsCommand::onEnabled(Context* context)
{
  return true;
}

void GridSettingsCommand::onExecute(Context* context)
{
  JWidget grid_x, grid_y, grid_w, grid_h, button_ok;

  FramePtr window(load_widget("grid_settings.xml", "grid_settings"));
  get_widgets(window,
              "ok", &button_ok,
              "grid_x", &grid_x,
              "grid_y", &grid_y,
              "grid_w", &grid_w,
              "grid_h", &grid_h, NULL);

  Rect bounds = UIContext::instance()->getSettings()->getGridBounds();

  grid_x->setTextf("%d", bounds.x);
  grid_y->setTextf("%d", bounds.y);
  grid_w->setTextf("%d", bounds.w);
  grid_h->setTextf("%d", bounds.h);

  window->open_window_fg();

  if (window->get_killer() == button_ok) {
    bounds.x = grid_x->getTextInt();
    bounds.y = grid_y->getTextInt();
    bounds.w = grid_w->getTextInt();
    bounds.h = grid_h->getTextInt();
    bounds.w = MAX(bounds.w, 1);
    bounds.h = MAX(bounds.h, 1);

    UIContext::instance()->getSettings()->setGridBounds(bounds);

    if (UIContext::instance()->getSettings()->getGridVisible())
      refresh_all_editors();
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createShowGridCommand()
{
  return new ShowGridCommand;
}

Command* CommandFactory::createSnapToGridCommand()
{
  return new SnapToGridCommand;
}

Command* CommandFactory::createGridSettingsCommand()
{
  return new GridSettingsCommand;
}
