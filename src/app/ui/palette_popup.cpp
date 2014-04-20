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

#include "app/ui/palette_popup.h"

#include "app/commands/cmd_set_palette.h"
#include "app/commands/commands.h"
#include "app/launcher.h"
#include "app/load_widget.h"
#include "app/palettes_loader.h"
#include "app/ui/palette_listbox.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/theme.h"
#include "ui/view.h"

namespace app {

using namespace ui;

PalettePopup::PalettePopup()
  : PopupWindow("Palettes", kCloseOnClickInOtherWindow)
{
  setAutoRemap(false);
  setBorder(gfx::Border(4*jguiscale()));

  addChild(app::load_widget<Box>("palette_popup.xml", "mainbox"));

  m_load = findChildT<Button>("load");
  m_load->Click.connect(Bind<void>(&PalettePopup::onLoad, this));
  findChildT<Button>("openfolder")->Click.connect(Bind<void>(&PalettePopup::onOpenFolder, this));

  m_view = findChildT<View>("view");
  m_view->attachToView(&m_paletteListBox);

  m_paletteListBox.PalChange.connect(&PalettePopup::onPalChange, this);
}

void PalettePopup::showPopup(const gfx::Rect& bounds)
{
  m_load->setEnabled(false);
  m_paletteListBox.selectChild(NULL);

  if (!UIContext::instance()->getActiveDocument())
    m_load->setText("Set as Default");
  else
    m_load->setText("Load");

  moveWindow(bounds);
  openWindow();
}

void PalettePopup::onPalChange(raster::Palette* palette)
{
  m_load->setEnabled(palette != NULL);
}

void PalettePopup::onLoad()
{
  raster::Palette* palette = m_paletteListBox.selectedPalette();
  if (!palette)
    return;

  SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
    CommandsModule::instance()->getCommandByName(CommandId::SetPalette));
  cmd->setPalette(palette);

  UIContext::instance()->executeCommand(cmd);
}

void PalettePopup::onOpenFolder()
{
  launcher::open_folder(PalettesLoader::palettesLocation());
}

} // namespace app
