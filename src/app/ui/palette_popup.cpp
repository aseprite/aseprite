// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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

#include "palette_popup.xml.h"

namespace app {

using namespace ui;

PalettePopup::PalettePopup()
  : PopupWindow("Palettes", kCloseOnClickInOtherWindow)
  , m_popup(new gen::PalettePopup())
{
  setAutoRemap(false);
  setBorder(gfx::Border(4*guiscale()));

  addChild(m_popup);

  m_popup->loadPal()->Click.connect(Bind<void>(&PalettePopup::onLoadPal, this));
  m_popup->openFolder()->Click.connect(Bind<void>(&PalettePopup::onOpenFolder, this));

  m_popup->view()->attachToView(&m_paletteListBox);

  m_paletteListBox.PalChange.connect(&PalettePopup::onPalChange, this);
}

void PalettePopup::showPopup(const gfx::Rect& bounds)
{
  m_popup->loadPal()->setEnabled(false);
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
}

void PalettePopup::onLoadPal()
{
  doc::Palette* palette = m_paletteListBox.selectedPalette();
  if (!palette)
    return;

  SetPaletteCommand* cmd = static_cast<SetPaletteCommand*>(
    CommandsModule::instance()->getCommandByName(CommandId::SetPalette));
  cmd->setPalette(palette);
  UIContext::instance()->executeCommand(cmd);
}

void PalettePopup::onOpenFolder()
{
  launcher::open_folder(PalettesLoaderDelegate().resourcesLocation());
}

} // namespace app
