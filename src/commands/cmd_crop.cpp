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

#include "app.h"
#include "app/color_utils.h"
#include "commands/command.h"
#include "document_wrappers.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "undo_transaction.h"
#include "util/autocrop.h"
#include "util/misc.h"
#include "widgets/color_bar.h"

//////////////////////////////////////////////////////////////////////
// crop_sprite

class CropSpriteCommand : public Command
{
public:
  CropSpriteCommand();
  Command* clone() const { return new CropSpriteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CropSpriteCommand::CropSpriteCommand()
  : Command("CropSprite",
            "Crop Sprite",
            CmdRecordableFlag)
{
}

bool CropSpriteCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasVisibleMask);
}

void CropSpriteCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  const Sprite* sprite(document->getSprite());
  const Mask* mask(document->getMask());
  {
    UndoTransaction undoTransaction(document, "Sprite Crop");
    int bgcolor = color_utils::color_for_image(app_get_colorbar()->getBgColor(), sprite->getPixelFormat());

    undoTransaction.cropSprite(mask->getBounds(), bgcolor);
    undoTransaction.commit();
  }
  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// autocrop_sprite

class AutocropSpriteCommand : public Command
{
public:
  AutocropSpriteCommand();
  Command* clone() const { return new AutocropSpriteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

AutocropSpriteCommand::AutocropSpriteCommand()
  : Command("AutocropSprite",
            "Trim Sprite",
            CmdRecordableFlag)
{
}

bool AutocropSpriteCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void AutocropSpriteCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  {
    int bgcolor = color_utils::color_for_image(app_get_colorbar()->getBgColor(), sprite->getPixelFormat());

    UndoTransaction undoTransaction(document, "Trim Sprite");
    undoTransaction.trimSprite(bgcolor);
    undoTransaction.commit();
  }
  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createCropSpriteCommand()
{
  return new CropSpriteCommand;
}

Command* CommandFactory::createAutocropSpriteCommand()
{
  return new AutocropSpriteCommand;
}
