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

#include "commands/command.h"
#include "commands/params.h"
#include "document_wrappers.h"
#include "modules/editors.h"
#include "settings/settings.h"
#include "ui/view.h"
#include "widgets/editor/editor.h"

//////////////////////////////////////////////////////////////////////
// scroll

class ScrollCommand : public Command
{
public:
  enum Direction { Left, Up, Right, Down, };
  enum Units {
    Pixel,
    TileWidth,
    TileHeight,
    ZoomedPixel,
    ZoomedTileWidth,
    ZoomedTileHeight,
    ViewportWidth,
    ViewportHeight
  };

  ScrollCommand();
  Command* clone() { return new ScrollCommand(*this); }

protected:
  void onLoadParams(Params* params);
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
  Direction m_direction;
  Units m_units;
  int m_quantity;
};

ScrollCommand::ScrollCommand()
  : Command("Scroll",
            "Scroll",
            CmdUIOnlyFlag)
{
}

void ScrollCommand::onLoadParams(Params* params)
{
  std::string direction = params->get("direction");
  if (direction == "left") m_direction = Left;
  else if (direction == "right") m_direction = Right;
  else if (direction == "up") m_direction = Up;
  else if (direction == "down") m_direction = Down;

  std::string units = params->get("units");
  if (units == "pixel") m_units = Pixel;
  else if (units == "tile-width") m_units = TileWidth;
  else if (units == "tile-height") m_units = TileHeight;
  else if (units == "zoomed-pixel") m_units = ZoomedPixel;
  else if (units == "zoomed-tile-width") m_units = ZoomedTileWidth;
  else if (units == "zoomed-tile-height") m_units = ZoomedTileHeight;
  else if (units == "viewport-width") m_units = ViewportWidth;
  else if (units == "viewport-height") m_units = ViewportHeight;

  int quantity = params->get_as<int>("quantity");
  m_quantity = std::max<int>(1, quantity);
}

bool ScrollCommand::onEnabled(Context* context)
{
  ActiveDocumentWriter document(context);
  return document != NULL;
}

void ScrollCommand::onExecute(Context* context)
{
  ui::View* view = ui::View::getView(current_editor);
  gfx::Rect vp = view->getViewportBounds();
  gfx::Point scroll = view->getViewScroll();
  gfx::Rect gridBounds = context->getSettings()->getGridBounds();
  int dx = 0;
  int dy = 0;
  int pixels = 0;

  switch (m_units) {
    case Pixel:
      pixels = 1;
      break;
    case TileWidth:
      pixels = gridBounds.w;
      break;
    case TileHeight:
      pixels = gridBounds.h;
      break;
    case ZoomedPixel:
      pixels = 1 << current_editor->getZoom();
      break;
    case ZoomedTileWidth:
      pixels = gridBounds.w << current_editor->getZoom();
      break;
    case ZoomedTileHeight:
      pixels = gridBounds.h << current_editor->getZoom();
      break;
    case ViewportWidth:
      pixels = vp.h;
      break;
    case ViewportHeight:
      pixels = vp.w;
      break;
  }

  switch (m_direction) {
    case Left:  dx = -m_quantity * pixels; break;
    case Right: dx = +m_quantity * pixels; break;
    case Up:    dy = -m_quantity * pixels; break;
    case Down:  dy = +m_quantity * pixels; break;
  }

  current_editor->setEditorScroll(scroll.x+dx, scroll.y+dy, true);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createScrollCommand()
{
  return new ScrollCommand;
}
