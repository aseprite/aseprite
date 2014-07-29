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
#include "app/color.h"
#include "app/commands/command.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "app/ui/status_bar.h"
#include "app/undo_transaction.h"
#include "raster/cel.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/sprite.h"
#include "ui/ui.h"

#include <stdexcept>

namespace app {
  
class NewFrameCommand : public Command {
public:
  NewFrameCommand();
  Command* clone() const OVERRIDE { return new NewFrameCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

NewFrameCommand::NewFrameCommand()
  : Command("NewFrame",
            "New Frame",
            CmdRecordableFlag)
{
}

bool NewFrameCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::HasActiveSprite);
}

void NewFrameCommand::onExecute(Context* context)
{
  ContextWriter writer(context);
  Document* document(writer.document());
  Sprite* sprite(writer.sprite());
  {
    UndoTransaction undoTransaction(writer.context(), "New Frame");
    document->getApi().addFrame(sprite, writer.frame().next());
    undoTransaction.commit();
  }
  update_screen_for_document(document);

  StatusBar::instance()
    ->showTip(1000, "New frame %d/%d",
              (int)context->activeLocation().frame()+1,
              (int)sprite->getTotalFrames());

  App::instance()->getMainWindow()->popTimeline();
}

Command* CommandFactory::createNewFrameCommand()
{
  return new NewFrameCommand;
}

} // namespace app
