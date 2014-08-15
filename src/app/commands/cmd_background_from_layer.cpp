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

#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/document_location.h"
#include "app/modules/gui.h"
#include "app/settings/settings.h"
#include "app/ui/color_bar.h"
#include "app/undo_transaction.h"
#include "raster/layer.h"
#include "raster/sprite.h"

namespace app {
  
class BackgroundFromLayerCommand : public Command {
public:
  BackgroundFromLayerCommand();
  Command* clone() const override { return new BackgroundFromLayerCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

BackgroundFromLayerCommand::BackgroundFromLayerCommand()
  : Command("BackgroundFromLayer",
            "BackgroundFromLayer",
            CmdRecordableFlag)
{
}

bool BackgroundFromLayerCommand::onEnabled(Context* context)
{
  return
    context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                        ContextFlags::ActiveLayerIsReadable |
                        ContextFlags::ActiveLayerIsWritable |
                        ContextFlags::HasActiveImage) &&
    // Doesn't have a background layer
    !context->checkFlags(ContextFlags::HasBackgroundLayer);
}

void BackgroundFromLayerCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());

  raster::color_t bgcolor =
    color_utils::color_for_target(
      context->settings()->getBgColor(),
      ColorTarget(
        ColorTarget::BackgroundLayer,
        sprite->pixelFormat(),
        sprite->transparentColor()));

  {
    UndoTransaction undo_transaction(writer.context(), "Background from Layer");
    document->getApi().backgroundFromLayer(
      static_cast<LayerImage*>(writer.layer()),
      bgcolor);
    undo_transaction.commit();
  }

  update_screen_for_document(document);
}

Command* CommandFactory::createBackgroundFromLayerCommand()
{
  return new BackgroundFromLayerCommand;
}

} // namespace app
