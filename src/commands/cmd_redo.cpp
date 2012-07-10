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
#include "base/thread.h"
#include "commands/command.h"
#include "document_undo.h"
#include "document_wrappers.h"
#include "ini_file.h"
#include "modules/editors.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "widgets/editor/editor.h"
#include "widgets/status_bar.h"

class RedoCommand : public Command
{
public:
  RedoCommand();
  Command* clone() { return new RedoCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

RedoCommand::RedoCommand()
  : Command("Redo",
            "Redo",
            CmdUIOnlyFlag)
{
}

bool RedoCommand::onEnabled(Context* context)
{
  ActiveDocumentWriter document(context);
  return
    document != NULL &&
    document->getUndo()->canRedo();
}

void RedoCommand::onExecute(Context* context)
{
  ActiveDocumentWriter document(context);
  DocumentUndo* undo = document->getUndo();
  Sprite* sprite = document->getSprite();

  if (get_config_bool("Options", "UndoGotoModified", true)) {
    if (undo->getNextRedoFrame() != sprite->getCurrentFrame() ||
        undo->getNextRedoLayer() != sprite->getCurrentLayer()) {
      sprite->setCurrentFrame(undo->getNextRedoFrame());
      sprite->setCurrentLayer(undo->getNextRedoLayer());

      current_editor->drawSpriteSafe(0, 0, sprite->getWidth(), sprite->getHeight());
      update_screen_for_document(document);
      gui_feedback();

      base::this_thread::sleep_for(0.01);
    }
  }

  StatusBar::instance()
    ->showTip(1000, "Redid %s",
              undo->getNextRedoLabel());

  undo->doRedo();
  document->generateMaskBoundaries();
  document->destroyExtraCel(); // Regenerate extras

  update_screen_for_document(document);
}

//////////////////////////////////////////////////////////////////////
// CommandFactory

Command* CommandFactory::createRedoCommand()
{
  return new RedoCommand;
}
