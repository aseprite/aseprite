/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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
#include "commands/commands.h"
#include "document_wrappers.h"
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "undo/undo_history.h"

//////////////////////////////////////////////////////////////////////
// invert_mask

class InvertMaskCommand : public Command
{
public:
  InvertMaskCommand();
  Command* clone() { return new InvertMaskCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

InvertMaskCommand::InvertMaskCommand()
  : Command("InvertMask",
	    "Invert Mask",
	    CmdRecordableFlag)
{
}

bool InvertMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
			     ContextFlags::HasActiveSprite);
}

void InvertMaskCommand::onExecute(Context* context)
{
  bool hasMask = false;
  {
    const ActiveDocumentReader document(context);
    if (document->isMaskVisible())
      hasMask = true;
  }

  // without mask?...
  if (!hasMask) {
    // so we select all
    Command* mask_all_cmd =
      CommandsModule::instance()->getCommandByName(CommandId::MaskAll);
    context->executeCommand(mask_all_cmd);
  }
  // invert the current mask
  else {
    ActiveDocumentWriter document(context);
    Sprite* sprite(document->getSprite());
    undo::UndoHistory* undo = document->getUndoHistory();

    /* undo */
    if (undo->isEnabled()) {
      undo->setLabel("Mask Invert");
      undo->setModification(undo::DoesntModifyDocument);
      undo->undo_set_mask(document);
    }

    /* create a new mask */
    Mask* mask = mask_new();

    /* select all the sprite area */
    mask_replace(mask, 0, 0, sprite->getWidth(), sprite->getHeight());

    /* remove in the new mask the current sprite marked region */
    image_rectfill(mask->bitmap,
		   document->getMask()->x, document->getMask()->y,
		   document->getMask()->x + document->getMask()->w-1,
		   document->getMask()->y + document->getMask()->h-1, 0);

    /* invert the current mask in the sprite */
    mask_invert(document->getMask());
    if (document->getMask()->bitmap) {
      /* copy the inverted region in the new mask */
      image_copy(mask->bitmap, document->getMask()->bitmap,
		 document->getMask()->x, document->getMask()->y);
    }

    /* we need only need the area inside the sprite */
    mask_intersect(mask, 0, 0, sprite->getWidth(), sprite->getHeight());

    /* set the new mask */
    document->setMask(mask);
    mask_free(mask);

    document->generateMaskBoundaries();
    update_screen_for_document(document);
  }
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createInvertMaskCommand()
{
  return new InvertMaskCommand;
}
