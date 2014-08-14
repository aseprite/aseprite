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

#include "app/commands/cmd_set_palette.h"
#include "app/context_access.h"
#include "app/document_api.h"
#include "app/file_selector.h"
#include "app/ini_file.h"
#include "app/modules/palettes.h"
#include "app/undo_transaction.h"
#include "base/override.h"
#include "base/unique_ptr.h"
#include "raster/palette.h"
#include "ui/alert.h"
#include "ui/manager.h"

namespace app {

using namespace ui;

SetPaletteCommand::SetPaletteCommand()
  : Command("SetPalette",
            "Set Palette",
            CmdRecordableFlag)
  , m_palette(NULL)
{
}

void SetPaletteCommand::onExecute(Context* context)
{
  ASSERT(m_palette);
  if (!m_palette)
    return;

  ContextWriter writer(context);

  if (writer.document()) {
    UndoTransaction undoTransaction(writer.context(), "Set Palette");
    writer.document()->getApi()
      .setPalette(writer.sprite(), writer.frame(), m_palette);
    undoTransaction.commit();
  }
  // Set default palette
  else {
    set_default_palette(m_palette);
    set_config_string("GfxMode", "Palette", m_palette->filename().c_str());
  }

  // Set the palette calling the hooks
  set_current_palette(m_palette, false);

  // Redraw the entire screen
  ui::Manager::getDefault()->invalidate();
}

Command* CommandFactory::createSetPaletteCommand()
{
  return new SetPaletteCommand;
}

} // namespace app
