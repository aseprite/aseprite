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
#include "dialogs/maskcol.h"
#include "gui/base.h"
#include "document_wrappers.h"

//////////////////////////////////////////////////////////////////////
// mask_by_color

class MaskByColorCommand : public Command
{
public:
  MaskByColorCommand();
  Command* clone() { return new MaskByColorCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

MaskByColorCommand::MaskByColorCommand()
  : Command("MaskByColor",
	    "Mask By Color",
	    CmdUIOnlyFlag)
{
}

bool MaskByColorCommand::onEnabled(Context* context)
{
  const ActiveDocumentReader document(context);
  const Sprite* sprite(document ? document->getSprite(): 0);
  return sprite != NULL;
}

void MaskByColorCommand::onExecute(Context* context)
{
  dialogs_mask_color(context->getActiveDocument());
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createMaskByColorCommand()
{
  return new MaskByColorCommand;
}
