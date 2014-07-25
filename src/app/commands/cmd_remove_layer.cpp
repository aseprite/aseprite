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
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/ui/timeline.h"
#include "app/undo_transaction.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "ui/widget.h"

namespace app {
  
class RemoveLayerCommand : public Command {
public:
  RemoveLayerCommand();
  Command* clone() const OVERRIDE { return new RemoveLayerCommand(*this); }

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
                             ContextFlags::HasActiveSprite |
                             ContextFlags::HasActiveLayer |
                             ContextFlags::ActiveLayerIsReadable |
                             ContextFlags::ActiveLayerIsWritable);
}

void RemoveLayerCommand::onExecute(Context* context)
{
  std::string layer_name;
  ContextWriter writer(context);
  Document* document(writer.document());
  Layer* layer(writer.layer());
  {
    UndoTransaction undoTransaction(writer.context(), "Remove Layer");

    // TODO the range of selected layer should be in the DocumentLocation.
    Timeline::Range range = App::instance()->getMainWindow()->getTimeline()->range();
    if (range.enabled()) {
      Sprite* sprite = writer.sprite();

      for (LayerIndex layer = range.layerEnd(); layer >= range.layerBegin(); --layer) {
        document->getApi().removeLayer(sprite->indexToLayer(layer));
      }
    }
    else {
      layer_name = layer->getName();
      document->getApi().removeLayer(layer);
    }

    undoTransaction.commit();
  }
  update_screen_for_document(document);

  StatusBar::instance()->invalidate();
  if (!layer_name.empty())
    StatusBar::instance()->showTip(1000, "Layer `%s' removed", layer_name.c_str());
  else
    StatusBar::instance()->showTip(1000, "Layers removed");
}

Command* CommandFactory::createRemoveLayerCommand()
{
  return new RemoveLayerCommand;
}

} // namespace app
