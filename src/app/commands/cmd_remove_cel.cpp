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
#include "app/ui/timeline.h"
#include "app/undo_transaction.h"
#include "raster/cel.h"
#include "raster/layer.h"
#include "raster/sprite.h"

namespace app {

class RemoveCelCommand : public Command {
public:
  RemoveCelCommand();
  Command* clone() const OVERRIDE { return new RemoveCelCommand(*this); }

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
  ContextWriter writer(context);
  Document* document(writer.document());
  {
    UndoTransaction undoTransaction(writer.context(), "Remove Cel");

    // TODO the range of selected frames should be in the DocumentLocation.
    Timeline::Range range = App::instance()->getMainWindow()->getTimeline()->range();
    if (range.enabled()) {
      Sprite* sprite = writer.sprite();

      for (LayerIndex layerIdx = range.layerBegin(); layerIdx <= range.layerEnd(); ++layerIdx) {
        Layer* layer = sprite->indexToLayer(layerIdx);
        if (!layer->isImage())
          continue;

        LayerImage* layerImage = static_cast<LayerImage*>(layer);

        for (FrameNumber frame = range.frameEnd(),
               begin = range.frameBegin().previous();
             frame != begin;
             frame = frame.previous()) {
          Cel* cel = layerImage->getCel(frame);
          if (cel)
            document->getApi().removeCel(layerImage, cel);
        }
      }
    }
    else {
      document->getApi().removeCel(static_cast<LayerImage*>(writer.layer()), writer.cel());
    }

    undoTransaction.commit();
  }
  update_screen_for_document(document);
}

Command* CommandFactory::createRemoveCelCommand()
{
  return new RemoveCelCommand;
}

} // namespace app
