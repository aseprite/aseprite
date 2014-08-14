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
#include "app/color_utils.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/undo_transaction.h"
#include "raster/sprite.h"

namespace app {

class FlattenLayersCommand : public Command {
public:
  FlattenLayersCommand();
  Command* clone() const OVERRIDE { return new FlattenLayersCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

FlattenLayersCommand::FlattenLayersCommand()
  : Command("FlattenLayers",
            "Flatten Layers",
            CmdUIOnlyFlag)
{
}

bool FlattenLayersCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void FlattenLayersCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document = writer.document();
  Sprite* sprite = writer.sprite();
  int bgcolor = color_utils::color_for_image(ColorBar::instance()->getBgColor(),
                                             sprite->pixelFormat());
  {
    UndoTransaction undoTransaction(writer.context(), "Flatten Layers");
    document->getApi().flattenLayers(sprite, bgcolor);
    undoTransaction.commit();
  }
  update_screen_for_document(writer.document());
}

Command* CommandFactory::createFlattenLayersCommand()
{
  return new FlattenLayersCommand;
}

} // namespace app
