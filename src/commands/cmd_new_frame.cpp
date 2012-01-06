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
#include "app/color.h"
#include "commands/command.h"
#include "console.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "modules/gui.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "undo_transaction.h"
#include "widgets/statebar.h"

#include <stdexcept>

//////////////////////////////////////////////////////////////////////
// new_frame

class NewFrameCommand : public Command
{
public:
  NewFrameCommand();
  Command* clone() { return new NewFrameCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

NewFrameCommand::NewFrameCommand()
  : Command("NewFrame",
            "New Frame",
            CmdRecordableFlag)
{
}

bool NewFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite |
                             ContextFlags::HasActiveLayer |
                             ContextFlags::ActiveLayerIsReadable |
                             ContextFlags::ActiveLayerIsWritable |
                             ContextFlags::ActiveLayerIsImage);
}

void NewFrameCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  {
    UndoTransaction undoTransaction(document, "New Frame");
    undoTransaction.newFrame();
    undoTransaction.commit();
  }
  update_screen_for_document(document);
  app_get_statusbar()
    ->showTip(1000, "New frame %d/%d",
              sprite->getCurrentFrame()+1,
              sprite->getTotalFrames());
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createNewFrameCommand()
{
  return new NewFrameCommand;
}
