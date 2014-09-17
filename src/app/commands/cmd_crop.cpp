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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/undo_transaction.h"
#include "app/util/autocrop.h"
#include "app/util/misc.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"

namespace app {

class CropSpriteCommand : public Command {
public:
  CropSpriteCommand();
  Command* clone() const override { return new CropSpriteCommand(*this); }

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
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  Mask* mask(document->mask());
  {
    UndoTransaction undoTransaction(writer.context(), "Sprite Crop");
    document->getApi().cropSprite(sprite, mask->bounds());
    undoTransaction.commit();
  }
  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

class AutocropSpriteCommand : public Command {
public:
  AutocropSpriteCommand();
  Command* clone() const override { return new AutocropSpriteCommand(*this); }

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
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    UndoTransaction undoTransaction(writer.context(), "Trim Sprite");
    document->getApi().trimSprite(sprite);
    undoTransaction.commit();
  }
  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

Command* CommandFactory::createCropSpriteCommand()
{
  return new CropSpriteCommand;
}

Command* CommandFactory::createAutocropSpriteCommand()
{
  return new AutocropSpriteCommand;
}

} // namespace app
