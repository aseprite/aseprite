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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/cmd_move_mask.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/settings/document_settings.h"
#include "app/settings/settings.h"
#include "app/ui/editor/editor.h"
#include "app/undo_transaction.h"
#include "app/undoers/set_mask_position.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "ui/view.h"

namespace app {

MoveMaskCommand::MoveMaskCommand()
  : Command("MoveMask",
            "Move Mask",
            CmdRecordableFlag)
{
}

void MoveMaskCommand::onLoadParams(Params* params)
{
  std::string target = params->get("target");
  if (target == "boundaries") m_target = Boundaries;
  else if (target == "content") m_target = Content;

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

bool MoveMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::HasActiveDocument |
                             ContextFlags::HasVisibleMask);
}

void MoveMaskCommand::onExecute(Context* context)
{
  IDocumentSettings* docSettings = context->settings()->getDocumentSettings(context->activeDocument());
  ui::View* view = ui::View::getView(current_editor);
  gfx::Rect vp = view->getViewportBounds();
  gfx::Rect gridBounds = docSettings->getGridBounds();
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
      pixels = 1 << current_editor->zoom();
      break;
    case ZoomedTileWidth:
      pixels = gridBounds.w << current_editor->zoom();
      break;
    case ZoomedTileHeight:
      pixels = gridBounds.h << current_editor->zoom();
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

  switch (m_target) {

    case Boundaries: {
      ContextWriter writer(context);
      Document* document(writer.document());
      {
        UndoTransaction undo(writer.context(), "Move Selection", undo::DoesntModifyDocument);
        gfx::Point pt = document->mask()->bounds().getOrigin();
        document->getApi().setMaskPosition(pt.x+dx, pt.y+dy);
        undo.commit();
      }

      document->generateMaskBoundaries();
      update_screen_for_document(document);
      break;
    }

    case Content: {
      current_editor->startSelectionTransformation(gfx::Point(dx, dy));
      break;
    }

  }
}

Command* CommandFactory::createMoveMaskCommand()
{
  return new MoveMaskCommand;
}

} // namespace app
