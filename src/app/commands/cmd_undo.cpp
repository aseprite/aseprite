/* Aseprite
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/document_undo.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/modules/gui.h"
#include "app/modules/palettes.h"
#include "app/settings/settings.h"
#include "app/ui/editor/editor.h"
#include "app/ui/status_bar.h"
#include "base/thread.h"
#include "raster/sprite.h"
#include "ui/system.h"

namespace app {

class UndoCommand : public Command {
public:
  enum Type { Undo, Redo };

  UndoCommand(Type type);
  Command* clone() const override { return new UndoCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);

private:
  Type m_type;
};

UndoCommand::UndoCommand(Type type)
  : Command((type == Undo ? "Undo": "Redo"),
            (type == Undo ? "Undo": "Redo"),
            CmdUIOnlyFlag)
  , m_type(type)
{
}

bool UndoCommand::onEnabled(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  return
    document != NULL &&
    ((m_type == Undo ? document->getUndo()->canUndo():
                       document->getUndo()->canRedo()));
}

void UndoCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  DocumentUndo* undo = document->getUndo();
  Sprite* sprite = document->sprite();

  if (context->settings()->undoGotoModified()) {
    SpritePosition spritePosition;
    SpritePosition currentPosition(writer.location()->layerIndex(),
                                   writer.location()->frame());

    if (m_type == Undo)
      spritePosition = undo->getNextUndoSpritePosition();
    else
      spritePosition = undo->getNextRedoSpritePosition();

    if (spritePosition != currentPosition) {
      current_editor->setLayer(sprite->indexToLayer(spritePosition.layerIndex()));
      current_editor->setFrame(spritePosition.frameNumber());

      // Draw the current layer/frame (which is not undone yet) so the
      // user can see the doUndo/doRedo effect.
      current_editor->drawSpriteClipped(
        gfx::Region(gfx::Rect(0, 0, sprite->width(), sprite->height())));

      ui::dirty_display_flag = true;
      gui_feedback();

      base::this_thread::sleep_for(0.01);
    }
  }

  StatusBar::instance()
    ->showTip(1000, "%s %s",
              (m_type == Undo ? "Undid": "Redid"),
              (m_type == Undo ? undo->getNextUndoLabel():
                                undo->getNextRedoLabel()));

  // Effectively undo/redo.
  if (m_type == Undo)
    undo->doUndo();
  else
    undo->doRedo();

  document->generateMaskBoundaries();
  document->destroyExtraCel(); // Regenerate extras

  update_screen_for_document(document);
  set_current_palette(writer.palette(), false);
}

Command* CommandFactory::createUndoCommand()
{
  return new UndoCommand(UndoCommand::Undo);
}

Command* CommandFactory::createRedoCommand()
{
  return new UndoCommand(UndoCommand::Redo);
}

} // namespace app
