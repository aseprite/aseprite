// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/shift_masked_cel.h"
#include "app/commands/cmd_move_mask.h"
#include "app/commands/command.h"
#include "app/commands/params.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/pref/preferences.h"
#include "app/transaction.h"
#include "app/ui/editor/editor.h"
#include "base/convert_to.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/view.h"

namespace app {

MoveMaskCommand::MoveMaskCommand()
  : Command("MoveMask",
            "Move Mask",
            CmdRecordableFlag)
{
}

void MoveMaskCommand::onLoadParams(const Params& params)
{
  std::string target = params.get("target");
  if (target == "boundaries") m_target = Boundaries;
  else if (target == "content") m_target = Content;

  std::string direction = params.get("direction");
  if (direction == "left") m_direction = Left;
  else if (direction == "right") m_direction = Right;
  else if (direction == "up") m_direction = Up;
  else if (direction == "down") m_direction = Down;

  std::string units = params.get("units");
  if (units == "pixel") m_units = Pixel;
  else if (units == "tile-width") m_units = TileWidth;
  else if (units == "tile-height") m_units = TileHeight;
  else if (units == "zoomed-pixel") m_units = ZoomedPixel;
  else if (units == "zoomed-tile-width") m_units = ZoomedTileWidth;
  else if (units == "zoomed-tile-height") m_units = ZoomedTileHeight;
  else if (units == "viewport-width") m_units = ViewportWidth;
  else if (units == "viewport-height") m_units = ViewportHeight;

  int quantity = params.get_as<int>("quantity");
  m_quantity = std::max<int>(1, quantity);

  if (params.has_param("wrap"))
    m_wrap = params.get_as<bool>("wrap");
  else
    m_wrap = false;
}

bool MoveMaskCommand::onEnabled(Context* context)
{
  switch (m_target) {

    case Boundaries:
      return context->checkFlags(ContextFlags::HasActiveDocument |
                                 ContextFlags::HasVisibleMask);

    case Content:
      if (m_wrap)
        return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                                   ContextFlags::HasVisibleMask |
                                   ContextFlags::HasActiveImage);
      else
        return (current_editor ? true: false);

  }

  return false;
}

void MoveMaskCommand::onExecute(Context* context)
{
  DocumentPreferences& docPref = Preferences::instance().document(context->activeDocument());
  ui::View* view = ui::View::getView(current_editor);
  gfx::Rect vp = view->viewportBounds();
  gfx::Rect gridBounds = docPref.grid.bounds();
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
      pixels = current_editor->zoom().apply(1);
      break;
    case ZoomedTileWidth:
      pixels = current_editor->zoom().apply(gridBounds.w);
      break;
    case ZoomedTileHeight:
      pixels = current_editor->zoom().apply(gridBounds.h);
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
        Transaction transaction(writer.context(), "Move Selection", DoesntModifyDocument);
        gfx::Point pt = document->mask()->bounds().origin();
        document->getApi(transaction).setMaskPosition(pt.x+dx, pt.y+dy);
        transaction.commit();
      }

      document->generateMaskBoundaries();
      update_screen_for_document(document);
      break;
    }

    case Content:
      if (m_wrap) {
        ContextWriter writer(context);
        if (writer.cel()) {
          // Rotate content
          Transaction transaction(writer.context(), "Shift Pixels");
          transaction.execute(new cmd::ShiftMaskedCel(writer.cel(), dx, dy));
          transaction.commit();
        }
        update_screen_for_document(writer.document());
      }
      else {
        current_editor->startSelectionTransformation(gfx::Point(dx, dy), 0.0);
      }
      break;

  }
}

std::string MoveMaskCommand::onGetFriendlyName() const
{
  std::string text = "Move";

  switch (m_target) {

    case Boundaries: {
      text += " Selection Boundaries";
      break;
    }

    case Content: {
      text += " Selection Content";
      break;
    }

  }

  text += " " + base::convert_to<std::string>(m_quantity);

  switch (m_units) {
    case Pixel:
      text += " pixel";
      break;
    case TileWidth:
      text += " horizontal tile";
      break;
    case TileHeight:
      text += " vertical tile";
      break;
    case ZoomedPixel:
      text += " zoomed pixel";
      break;
    case ZoomedTileWidth:
      text += " zoomed horizontal tile";
      break;
    case ZoomedTileHeight:
      text += " zoomed vertical tile";
      break;
    case ViewportWidth:
      text += " viewport width";
      break;
    case ViewportHeight:
      text += " viewport height";
      break;
  }
  if (m_quantity != 1)
    text += "s";

  switch (m_direction) {
    case Left:  text += " left"; break;
    case Right: text += " right"; break;
    case Up:    text += " up"; break;
    case Down:  text += " down"; break;
  }

  return text;
}

Command* CommandFactory::createMoveMaskCommand()
{
  return new MoveMaskCommand;
}

} // namespace app
