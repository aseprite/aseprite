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
#include "modules/gui.h"
#include "raster/image.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo_history.h"
#include "document_wrappers.h"

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
  const ActiveDocumentReader document(context);
  return document != NULL && document->getSprite() != NULL;
}

void InvertMaskCommand::onExecute(Context* context)
{
  bool has_mask = false;
  {
    const ActiveDocumentReader document(context);
    const Sprite* sprite(document->getSprite());
    if (sprite->getMask()->bitmap)
      has_mask = true;
  }

  // without mask?...
  if (!has_mask) {
    // so we select all
    Command* mask_all_cmd =
      CommandsModule::instance()->getCommandByName(CommandId::MaskAll);
    context->executeCommand(mask_all_cmd);
  }
  // invert the current mask
  else {
    ActiveDocumentWriter document(context);
    Sprite* sprite(document->getSprite());
    UndoHistory* undo = document->getUndoHistory();

    /* undo */
    if (undo->isEnabled()) {
      undo->setLabel("Mask Invert");
      undo->undo_set_mask(sprite);
    }

    /* create a new mask */
    Mask* mask = mask_new();

    /* select all the sprite area */
    mask_replace(mask, 0, 0, sprite->getWidth(), sprite->getHeight());

    /* remove in the new mask the current sprite marked region */
    image_rectfill(mask->bitmap,
		   sprite->getMask()->x, sprite->getMask()->y,
		   sprite->getMask()->x + sprite->getMask()->w-1,
		   sprite->getMask()->y + sprite->getMask()->h-1, 0);

    /* invert the current mask in the sprite */
    mask_invert(sprite->getMask());
    if (sprite->getMask()->bitmap) {
      /* copy the inverted region in the new mask */
      image_copy(mask->bitmap, sprite->getMask()->bitmap,
		 sprite->getMask()->x, sprite->getMask()->y);
    }

    /* we need only need the area inside the sprite */
    mask_intersect(mask, 0, 0, sprite->getWidth(), sprite->getHeight());

    /* set the new mask */
    sprite->setMask(mask);
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
