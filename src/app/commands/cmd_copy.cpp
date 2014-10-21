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
#include "app/ui/main_window.h"
#include "app/ui/timeline.h"
#include "app/util/clipboard.h"
#include "app/util/misc.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "ui/base.h"

namespace app {

class CopyCommand : public Command {
public:
  CopyCommand();
  Command* clone() const override { return new CopyCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

CopyCommand::CopyCommand()
  : Command("Copy",
            "Copy",
            CmdUIOnlyFlag)
{
}

bool CopyCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::HasActiveDocument);
}

void CopyCommand::onExecute(Context* context)
{
  const ContextReader reader(context);

  // Copy a range from the timeline.
  DocumentRange range = App::instance()->getMainWindow()->getTimeline()->range();
  if (range.enabled()) {
    clipboard::copy_range(reader, range);
  }
  else if (reader.location()->document() &&
           reader.location()->document()->isMaskVisible() &&
           reader.location()->image()) {
    clipboard::copy(reader);
  }
}

Command* CommandFactory::createCopyCommand()
{
  return new CopyCommand;
}

} // namespace app
