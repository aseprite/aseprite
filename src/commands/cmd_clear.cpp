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

#include "app.h"
#include "commands/command.h"
#include "document_wrappers.h"
#include "modules/gui.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "undo/undo_history.h"
#include "undo_transaction.h"
#include "widgets/color_bar.h"

//////////////////////////////////////////////////////////////////////
// ClearCommand

class ClearCommand : public Command
{
public:
  ClearCommand();
  Command* clone() const { return new ClearCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

ClearCommand::ClearCommand()
  : Command("Clear",
	    "Clear",
	    CmdUIOnlyFlag)
{
}

bool ClearCommand::onEnabled(Context* context)
{
  const ActiveDocumentReader document(context);
  const Sprite* sprite(document ? document->getSprite(): 0);
  return
    sprite != NULL &&
    sprite->getCurrentLayer() != NULL &&
    sprite->getCurrentLayer()->is_image() &&
    sprite->getCurrentLayer()->is_readable() &&
    sprite->getCurrentLayer()->is_writable();
}

void ClearCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  bool visibleMask = document->isMaskVisible();
  {
    UndoTransaction undoTransaction(document, "Clear");
    undoTransaction.clearMask(app_get_color_to_clear_layer(sprite->getCurrentLayer()));
    if (visibleMask)
      undoTransaction.deselectMask();
    undoTransaction.commit();
  }
  if (visibleMask)
    document->generateMaskBoundaries();
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createClearCommand()
{
  return new ClearCommand;
}
