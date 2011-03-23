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

#include "gui/gui.h"

#include "commands/command.h"
#include "console.h"
#include "app.h"
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "raster/undo_history.h"
#include "document_wrappers.h"

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
  const ActiveDocumentReader document(context);
  const Sprite* sprite(document ? document->getSprite(): 0);
  return sprite && sprite->getCurrentLayer();
}

void DuplicateLayerCommand::onExecute(Context* context)
{
#if 0				// TODO IMPLEMENT THIS
  ActiveDocumentWriter document(context);
  Sprite* sprite = document->getSprite();
  UndoHistory* undo = document->getUndoHistory();
  Undoable undoable(document, "Layer Duplication");

  // Clone the layer
  UniquePtr<Layer> newLayerPtr(sprite->getCurrentLayer()->clone());
  newLayerPtr->set_background(false);
  newLayerPtr->set_moveable(true);
  newLayerPtr->setName(layer_copy->getName() + " Copy");

  // Add the new layer in the sprite.
  if (undo->isEnabled())
    undo->undo_add_layer(sprite->getCurrentLayer()->get_parent(), newLayerPtr);

  sprite->getCurrentLayer()->get_parent()->add_layer(newLayerPtr);

  // Release the pointer as it is owned by the sprite now
  Layer* newLayer = newLayerPtr.release();

  if (undo->isEnabled()) {
    undo->undo_move_layer(newLayer);
    undo->undo_set_layer(sprite);
  }

  undoable.commit();

  sprite->getCurrentLayer()->get_parent()->move_layer(newLayer, sprite->getCurrentLayer());
  sprite->setCurrentLayer(newLayer);

  update_screen_for_document(document);
#endif
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createDuplicateLayerCommand()
{
  return new DuplicateLayerCommand;
}
