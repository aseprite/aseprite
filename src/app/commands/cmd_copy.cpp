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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/util/clipboard.h"
#include "app/util/misc.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "ui/base.h"

namespace app {

class CopyCommand : public Command {
public:
  CopyCommand();
  Command* clone() const { return new CopyCommand(*this); }

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
  return context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
                             ContextFlags::ActiveLayerIsReadable |
                             ContextFlags::ActiveLayerIsWritable |
                             ContextFlags::HasActiveImage |
                             ContextFlags::HasVisibleMask);
}

void CopyCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  clipboard::copy(reader);
}

Command* CommandFactory::createCopyCommand()
{
  return new CopyCommand;
}

} // namespace app
