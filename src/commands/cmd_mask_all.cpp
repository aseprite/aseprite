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
#include "modules/gui.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo_history.h"
#include "document_wrappers.h"

//////////////////////////////////////////////////////////////////////
// mask_all

class MaskAllCommand : public Command
{
public:
  MaskAllCommand();
  Command* clone() { return new MaskAllCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

MaskAllCommand::MaskAllCommand()
  : Command("MaskAll",
	    "Mask All",
	    CmdRecordableFlag)
{
}

bool MaskAllCommand::onEnabled(Context* context)
{
  const ActiveDocumentReader document(context);
  const Sprite* sprite(document ? document->getSprite(): 0);
  return sprite != NULL;
}
  
void MaskAllCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  UndoHistory* undo(document->getUndoHistory());

  // Undo
  if (undo->isEnabled()) {
    undo->setLabel("Mask All");
    undo->undo_set_mask(document);
  }

  // Change the selection
  mask_replace(document->getMask(), 0, 0, sprite->getWidth(), sprite->getHeight());
  document->setMaskVisible(true);

  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createMaskAllCommand()
{
  return new MaskAllCommand;
}
