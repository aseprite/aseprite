/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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
#include "app/modules/gui.h"
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/document_range_ops.h"

namespace app {

class ReverseFramesCommand : public Command {
public:
  ReverseFramesCommand();
  Command* clone() const override { return new ReverseFramesCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

ReverseFramesCommand::ReverseFramesCommand()
  : Command("ReverseFrames",
            "Reverse Frames",
            CmdUIOnlyFlag)
{
}

bool ReverseFramesCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable);
}

void ReverseFramesCommand::onExecute(Context* context)
{
  DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
  if (!range.enabled())
    return;                     // Nothing to do

  Document* doc = context->activeDocument();

  reverse_frames(doc, range);

  update_screen_for_document(doc);
}

Command* CommandFactory::createReverseFramesCommand()
{
  return new ReverseFramesCommand;
}

} // namespace app
