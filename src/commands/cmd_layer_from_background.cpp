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
#include "document_wrappers.h"
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "ui/gui.h"
#include "undo_transaction.h"

//////////////////////////////////////////////////////////////////////
// layer_from_background

class LayerFromBackgroundCommand : public Command
{
public:
  LayerFromBackgroundCommand();
  Command* clone() { return new LayerFromBackgroundCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

LayerFromBackgroundCommand::LayerFromBackgroundCommand()
  : Command("LayerFromBackground",
            "Layer From Background",
            CmdRecordableFlag)
{
}

bool LayerFromBackgroundCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite |
                             ContextFlags::HasActiveLayer |
                             ContextFlags::ActiveLayerIsReadable |
                             ContextFlags::ActiveLayerIsWritable |
                             ContextFlags::ActiveLayerIsImage |
                             ContextFlags::ActiveLayerIsBackground);
}

void LayerFromBackgroundCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  {
    UndoTransaction undoTransaction(document, "Layer from Background");
    undoTransaction.layerFromBackground();
    undoTransaction.commit();
  }
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createLayerFromBackgroundCommand()
{
  return new LayerFromBackgroundCommand;
}
