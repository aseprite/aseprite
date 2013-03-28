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

#include "app.h"
#include "app/color_utils.h"
#include "commands/command.h"
#include "context_access.h"
#include "document_api.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "undo_transaction.h"
#include "widgets/color_bar.h"

//////////////////////////////////////////////////////////////////////
// flatten_layers

class FlattenLayersCommand : public Command
{
public:
  FlattenLayersCommand();
  Command* clone() { return new FlattenLayersCommand(*this); }

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
                                             sprite->getPixelFormat());
  {
    UndoTransaction undoTransaction(writer.context(), "Flatten Layers");
    document->getApi().flattenLayers(sprite, bgcolor);
    undoTransaction.commit();
  }
  update_screen_for_document(writer.document());
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createFlattenLayersCommand()
{
  return new FlattenLayersCommand;
}
