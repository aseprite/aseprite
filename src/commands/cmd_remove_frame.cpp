/* ASE - Allegro Sprite Editor
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

#include "commands/command.h"
#include "document_wrappers.h"
#include "gui/gui.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "undo_transaction.h"

//////////////////////////////////////////////////////////////////////
// remove_frame

class RemoveFrameCommand : public Command
{
public:
  RemoveFrameCommand();
  Command* clone() { return new RemoveFrameCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

RemoveFrameCommand::RemoveFrameCommand()
  : Command("RemoveFrame",
            "Remove Frame",
            CmdRecordableFlag)
{
}

bool RemoveFrameCommand::onEnabled(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document ? document->getSprite(): NULL);
  return
    sprite &&
    sprite->getTotalFrames() > 1;
}

void RemoveFrameCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  const Sprite* sprite(document->getSprite());
  {
    UndoTransaction undoTransaction(document, "Remove Frame");
    undoTransaction.removeFrame(sprite->getCurrentFrame());
    undoTransaction.commit();
  }
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createRemoveFrameCommand()
{
  return new RemoveFrameCommand;
}
