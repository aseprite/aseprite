/* Aseprite
 * Copyright (C) 2001-2015  David Capello
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
#include "app/transaction.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {

class ClearCelCommand : public Command {
public:
  ClearCelCommand();
  Command* clone() const override { return new ClearCelCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

ClearCelCommand::ClearCelCommand()
  : Command("ClearCel",
            "Clear Cel",
            CmdRecordableFlag)
{
}

bool ClearCelCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::ActiveLayerIsVisible |
                             ContextFlags::ActiveLayerIsEditable |
                             ContextFlags::ActiveLayerIsImage |
                             ContextFlags::HasActiveCel);
}

void ClearCelCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  {
    Transaction transaction(writer.context(), "Clear Cel");
    
    // TODO the range of selected frames should be in the DocumentLocation.
    Timeline::Range range = App::instance()->getMainWindow()->getTimeline()->range();
    if (range.enabled()) {
      Sprite* sprite = writer.sprite();

      for (LayerIndex layerIdx = range.layerBegin(); layerIdx <= range.layerEnd(); ++layerIdx) {
        Layer* layer = sprite->indexToLayer(layerIdx);
        if (!layer->isImage())
          continue;

        LayerImage* layerImage = static_cast<LayerImage*>(layer);

        for (frame_t frame = range.frameEnd(),
               begin = range.frameBegin()-1;
             frame != begin;
             --frame) {
          document->getApi(transaction).clearCel(layerImage, frame);
        }
      }
    }
    else {
      document->getApi(transaction).clearCel(writer.cel());
    }

    transaction.commit();
  }
  update_screen_for_document(document);
}

Command* CommandFactory::createClearCelCommand()
{
  return new ClearCelCommand;
}

} // namespace app
