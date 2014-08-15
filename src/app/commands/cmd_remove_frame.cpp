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
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/undo_transaction.h"
#include "raster/sprite.h"
#include "ui/ui.h"

namespace app {
  
class RemoveFrameCommand : public Command {
public:
  RemoveFrameCommand();
  Command* clone() const override { return new RemoveFrameCommand(*this); }

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
  ContextWriter writer(context);
  Sprite* sprite(writer.sprite());
  return
    sprite &&
    sprite->totalFrames() > 1;
}

void RemoveFrameCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    UndoTransaction undoTransaction(writer.context(), "Remove Frame");

    // TODO the range of selected frames should be in the DocumentLocation.
    Timeline::Range range = App::instance()->getMainWindow()->getTimeline()->range();
    if (range.enabled()) {
      for (FrameNumber frame = range.frameEnd(),
             begin = range.frameBegin().previous();
           frame != begin;
           frame = frame.previous()) {
        document->getApi().removeFrame(sprite, frame);
      }
    }
    else {
      document->getApi().removeFrame(sprite, writer.frame());
    }

    undoTransaction.commit();
  }
  update_screen_for_document(document);
}

Command* CommandFactory::createRemoveFrameCommand()
{
  return new RemoveFrameCommand;
}

} // namespace app
