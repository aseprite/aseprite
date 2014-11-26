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
#include "app/res/palettes_loader_delegate.h"
#include "app/ui/palettes_listbox.h"
#include "app/ui_context.h"
#include "base/bind.h"
#include "ui/box.h"
#include "ui/button.h"
#include "ui/theme.h"
#include "ui/view.h"

#include "generated_palette_popup.h"

namespace app {

using namespace ui;

PalettePopup::PalettePopup()
  : PopupWindow("Palettes", kCloseOnClickInOtherWindow)
  , m_popup(new gen::PalettePopup())
{
  setAutoRemap(false);
  setBorder(gfx::Border(4*guiscale()));

  addChild(m_popup);

  m_popup->loadPal()->Click.connect(Bind<void>(&PalettePopup::onLoadPal, this, false));
  m_popup->setDefault()->Click.connect(Bind<void>(&PalettePopup::onLoadPal, this, true));
  m_popup->openFolder()->Click.connect(Bind<void>(&PalettePopup::onOpenFolder, this));

  m_popup->view()->attachToView(&m_paletteListBox);

  m_paletteListBox.PalChange.connect(&PalettePopup::onPalChange, this);
}

void PalettePopup::showPopup(const gfx::Rect& bounds)
{
  m_popup->loadPal()->setEnabled(false);
  m_popup->setDefault()->setEnabled(false);
  m_paletteListBox.selectChild(NULL);

  moveWindow(bounds);

  // Setup the hot-region
  setHotRegion(gfx::Region(gfx::Rect(bounds).enlarge(32 * guiscale())));

  openWindow();
}

void PalettePopup::onPalChange(doc::Palette* palette)
{
  m_popup->loadPal()->setEnabled(
    UIContext::instance()->activeDocument() &&
    palette != NULL);

  m_popup->setDefault()->setEnabled(palette != NULL);
}

void PalettePopup::onLoadPal(bool asDefault)
{
  doc::Palette* palette = m_paletteListBox.selectedPalette();
  if (!palette)
    return;

  SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
    CommandsModule::instance()->getCommandByName(CommandId::SetPalette));

  cmd->setPalette(palette);
  if (asDefault)
    cmd->setTarget(SetPaletteCommand::Target::App);
  else
    cmd->setTarget(SetPaletteCommand::Target::Document);

  UIContext::instance()->executeCommand(cmd);
}

void PalettePopup::onOpenFolder()
{
  launcher::open_folder(PalettesLoaderDelegate().resourcesLocation());
}

} // namespace app
