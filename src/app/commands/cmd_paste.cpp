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

#include "app/commands/command.h"
#include "app/context.h"
#include "app/util/clipboard.h"
#include "base/override.h"
#include "raster/layer.h"
#include "raster/sprite.h"

namespace app {

class PasteCommand : public Command {
public:
  PasteCommand();
  Command* clone() const OVERRIDE { return new PasteCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

PasteCommand::PasteCommand()
  : Command("Paste",
            "Paste",
            CmdUIOnlyFlag)
{
}

bool PasteCommand::onEnabled(Context* context)
{
  return
    (clipboard::get_current_format() == clipboard::ClipboardImage &&
      context->checkFlags(ContextFlags::ActiveDocumentIsWritable |
        ContextFlags::ActiveLayerIsReadable |
        ContextFlags::ActiveLayerIsWritable |
        ContextFlags::ActiveLayerIsImage)) ||
    (clipboard::get_current_format() == clipboard::ClipboardDocumentRange &&
      context->checkFlags(ContextFlags::ActiveDocumentIsWritable));
}

void PasteCommand::onExecute(Context* context)
{
  clipboard::paste();
}

Command* CommandFactory::createPasteCommand()
{
  return new PasteCommand;
}

} // namespace app
