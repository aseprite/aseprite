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

#include "commands/command.h"
#include "context_access.h"
#include "modules/gui.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "undo_transaction.h"
#include "undoers/set_mask.h"

//////////////////////////////////////////////////////////////////////
// reselect_mask

class ReselectMaskCommand : public Command
{
public:
  ReselectMaskCommand();
  Command* clone() { return new ReselectMaskCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

ReselectMaskCommand::ReselectMaskCommand()
  : Command("ReselectMask",
            "Reselect Mask",
            CmdRecordableFlag)
{
}

bool ReselectMaskCommand::onEnabled(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  return
     document &&                      // The document does exist
    !document->isMaskVisible() &&     // The mask is hidden
     document->getMask() &&           // The mask does exist
    !document->getMask()->isEmpty();  // But it is not empty
}

void ReselectMaskCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  {
    UndoTransaction undo(writer.context(), "Mask Reselection", undo::DoesntModifyDocument);
    if (undo.isEnabled())
      undo.pushUndoer(new undoers::SetMask(undo.getObjects(), document));

    // Make the mask visible again.
    document->setMaskVisible(true);

    undo.commit();
  }

  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createReselectMaskCommand()
{
  return new ReselectMaskCommand;
}
