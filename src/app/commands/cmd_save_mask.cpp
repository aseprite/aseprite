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

#include <allegro/file.h>

#include "app/commands/command.h"
#include "app/context_access.h"
#include "app/file_selector.h"
#include "app/util/msk_file.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "ui/alert.h"

namespace app {

class SaveMaskCommand : public Command {
public:
  SaveMaskCommand();
  Command* clone() { return new SaveMaskCommand(*this); }

protected:
  bool onEnabled(Context* context);
  void onExecute(Context* context);
};

SaveMaskCommand::SaveMaskCommand()
  : Command("SaveMask",
            "Save Mask",
            CmdUIOnlyFlag)
{
}

bool SaveMaskCommand::onEnabled(Context* context)
{
  return context->checkFlags(ContextFlags::ActiveDocumentIsReadable);
}

void SaveMaskCommand::onExecute(Context* context)
{
  const ContextReader reader(context);
  const Document* document(reader.document());
  base::string filename = "default.msk";
  int ret;

  for (;;) {
    filename = app::show_file_selector("Save .msk File", filename, "msk");
    if (filename.empty())
      return;

    /* does the file exist? */
    if (exists(filename.c_str())) {
      /* ask if the user wants overwrite the file? */
      ret = ui::Alert::show("Warning<<File exists, overwrite it?<<%s||&Yes||&No||&Cancel",
                            get_filename(filename.c_str()));
    }
    else
      break;

    /* "yes": we must continue with the operation... */
    if (ret == 1)
      break;
    /* "cancel" or <esc> per example: we back doing nothing */
    else if (ret != 2)
      return;

    /* "no": we must back to select other file-name */
  }

  if (save_msk_file(document->getMask(), filename.c_str()) != 0)
    ui::Alert::show("Error<<Error saving .msk file<<%s||&Close", filename.c_str());
}

Command* CommandFactory::createSaveMaskCommand()
{
  return new SaveMaskCommand;
}

} // namespace app
