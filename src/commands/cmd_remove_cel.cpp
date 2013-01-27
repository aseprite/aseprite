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

#include "config.h"

#include "commands/command.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "undo_transaction.h"
#include "document_wrappers.h"

//////////////////////////////////////////////////////////////////////
// remove_cel

class RemoveCelCommand : public Command
{
public:
  RemoveCelCommand();
  Command* clone() { return new RemoveCelCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

RemoveCelCommand::RemoveCelCommand()
  : Command("RemoveCel",
            "Remove Cel",
            CmdRecordableFlag)
{
}

bool RemoveCelCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::ActiveLayerIsReadable |
                             ContextFlags::ActiveLayerIsWritable |
                             ContextFlags::ActiveLayerIsImage |
                             ContextFlags::HasActiveCel);
}

void RemoveCelCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  Cel* cel = static_cast<LayerImage*>(sprite->getCurrentLayer())->getCel(sprite->getCurrentFrame());
  {
    UndoTransaction undoTransaction(document, "Remove Cel");
    undoTransaction.removeCel(static_cast<LayerImage*>(sprite->getCurrentLayer()), cel);
    undoTransaction.commit();
  }
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createRemoveCelCommand()
{
  return new RemoveCelCommand;
}
