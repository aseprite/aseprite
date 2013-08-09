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

#include "app/app.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/document_undo.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/ui/editor/editor.h"
#include "app/undo_transaction.h"
#include "app/undoers/add_layer.h"
#include "app/undoers/move_layer.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "ui/ui.h"

namespace app {

class DuplicateLayerCommand : public Command {
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
  ContextWriter writer(context);
  Document* document = writer.document();
  Sprite* sprite = writer.sprite();
  UndoTransaction undo(writer.context(), "Layer Duplication");
  LayerImage* sourceLayer = static_cast<LayerImage*>(writer.layer());

  // Create a new layer
  base::UniquePtr<LayerImage> newLayerPtr(new LayerImage(sprite));

  // Disable undo because the layer content is added as a whole with
  // AddLayer() undoer.
  document->getUndo()->setEnabled(false);

  // Copy the layer content (cels + images)
  document->copyLayerContent(sourceLayer, document, newLayerPtr);

  // Restore enabled status.
  document->getUndo()->setEnabled(undo.isEnabled());

  // Copy the layer name
  newLayerPtr->setName(newLayerPtr->getName() + " Copy");

  // Add the new layer in the sprite.
  document->getApi().addLayer(sourceLayer->getParent(), newLayerPtr, sourceLayer);

  // Release the pointer as it is owned by the sprite now
  Layer* newLayer = newLayerPtr.release();

  undo.commit();

  update_screen_for_document(document);
}

Command* CommandFactory::createDuplicateLayerCommand()
{
  return new DuplicateLayerCommand;
}

} // namespace app
