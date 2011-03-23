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
#include "commands/params.h"
#include "dialogs/filesel.h"
#include "gui/alert.h"
#include "modules/gui.h" 
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/undo_history.h"
#include "document_wrappers.h"
#include "util/msk_file.h"

class LoadMaskCommand : public Command
{
  std::string m_filename;

public:
  LoadMaskCommand();
  Command* clone() const { return new LoadMaskCommand(*this); }

protected:
  void onLoadParams(Params* params);

  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

LoadMaskCommand::LoadMaskCommand()
  : Command("LoadMask",
	    "LoadMask",
	    CmdRecordableFlag)
{
  m_filename = "";
}

void LoadMaskCommand::onLoadParams(Params* params)
{
  m_filename = params->get("filename");
}

bool LoadMaskCommand::onEnabled(Context* context)
{
  const ActiveDocumentReader document(context);
  const Sprite* sprite(document ? document->getSprite(): 0);
  return sprite != NULL;
}

void LoadMaskCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  Sprite* sprite(document->getSprite());
  UndoHistory* undo(document->getUndoHistory());

  base::string filename = m_filename;

  if (context->isUiAvailable()) {
    filename = ase_file_selector("Load .msk File", filename, "msk");
    if (filename.empty())
      return;

    m_filename = filename;
  }

  Mask *mask = load_msk_file(m_filename.c_str());
  if (!mask)
    throw base::Exception("Error loading .msk file: %s",
			  static_cast<const char*>(m_filename.c_str()));

  // Add the mask change into the undo history.
  if (undo->isEnabled()) {
    undo->setLabel("Mask Load");
    undo->undo_set_mask(document);
  }

  document->setMask(mask);
  mask_free(mask);

  document->generateMaskBoundaries();
  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createLoadMaskCommand()
{
  return new LoadMaskCommand;
}
