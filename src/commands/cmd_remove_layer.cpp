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
#include "commands/command.h"
#include "document_wrappers.h"
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "ui/widget.h"
#include "undo_transaction.h"
#include "widgets/statebar.h"

//////////////////////////////////////////////////////////////////////
// remove_layer

class RemoveLayerCommand : public Command
{
public:
  RemoveLayerCommand();
  Command* clone() { return new RemoveLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

RemoveLayerCommand::RemoveLayerCommand()
  : Command("RemoveLayer",
            "Remove Layer",
            CmdRecordableFlag)
{
}

bool RemoveLayerCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveLayer);
}

void RemoveLayerCommand::onExecute(Context* context)
{
  std::string layer_name;
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  {
    UndoTransaction undoTransaction(document, "Remove Layer");

    layer_name = sprite->getCurrentLayer()->getName();

    undoTransaction.removeLayer(sprite->getCurrentLayer());
    undoTransaction.commit();
  }
  update_screen_for_document(document);

  app_get_statusbar()->invalidate();
  app_get_statusbar()
    ->showTip(1000, "Layer `%s' removed",
              layer_name.c_str());
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createRemoveLayerCommand()
{
  return new RemoveLayerCommand;
}
