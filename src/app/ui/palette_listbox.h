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

#ifndef APP_UI_PALETTE_LISTBOX_H_INCLUDED
#define APP_UI_PALETTE_LISTBOX_H_INCLUDED
#pragma once

#include "app/palettes_loader.h"
#include "base/compiler_specific.h"
#include "base/unique_ptr.h"
#include "ui/listbox.h"
#include "ui/timer.h"

namespace app {

  class PaletteListBox : public ui::ListBox {
  public:
    PaletteListBox();

    raster::Palette* selectedPalette();

    Signal1<void, raster::Palette*> PalChange;

  protected:
    bool onProcessMessage(ui::Message* msg) OVERRIDE;
    void onChangeSelectedItem() OVERRIDE;
    void onTick();
    void stop();

  private:
    base::UniquePtr<PalettesLoader> m_palettesLoader;
    ui::Timer m_palettesTimer;

    class LoadingItem;
    LoadingItem* m_loadingItem;
  };

} // namespace app

#endif
