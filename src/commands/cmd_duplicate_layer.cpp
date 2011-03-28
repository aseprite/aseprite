/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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
#include "commands/command.h"
#include "console.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "undo/undo_history.h"
#include "undo_transaction.h"

//////////////////////////////////////////////////////////////////////
// Duplicate Layer command

class DuplicateLayerCommand : public Command
{
public:
  DuplicateLayerCommand();
  Command* clone() const { return new DuplicateLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

DuplicateLayerCommand::DuplicateLayerCommand()
  : Command("DuplicateLayer",
	    "Duplicate Layer",
	    CmdRecordableFlag)
{
}

bool DuplicateLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
			     ContextFlags::HasActiveLayer |
			     ContextFlags::ActiveLayerIsImage);
}

void DuplicateLayerCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite = document->getSprite();
  undo::UndoHistory* undo = document->getUndoHistory();
  UndoTransaction undoTransaction(document, "Layer Duplication");
  LayerImage* sourceLayer = static_cast<LayerImage*>(sprite->getCurrentLayer());

  // Create a new layer
  UniquePtr<LayerImage> newLayerPtr(new LayerImage(sprite));

  // Copy the layer name
  newLayerPtr->setName(sourceLayer->getName() + " Copy");

  // Copy the layer content (cels + images)
  document->copyLayerContent(sourceLayer, newLayerPtr);

  // Add the new layer in the sprite.
  if (undo->isEnabled())
    undo->undo_add_layer(sourceLayer->get_parent(), newLayerPtr);

  sourceLayer->get_parent()->add_layer(newLayerPtr);

  // Release the pointer as it is owned by the sprite now
  Layer* newLayer = newLayerPtr.release();

  if (undo->isEnabled()) {
    undo->undo_move_layer(newLayer);
    undo->undo_set_layer(sprite);
  }

  undoTransaction.commit();

  sourceLayer->get_parent()->move_layer(newLayer, sourceLayer);
  sprite->setCurrentLayer(newLayer);

  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createDuplicateLayerCommand()
{
  return new DuplicateLayerCommand;
}
