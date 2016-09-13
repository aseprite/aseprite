// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_PALETTES_LISTBOX_H_INCLUDED
#define APP_UI_PALETTES_LISTBOX_H_INCLUDED
#pragma once

#include "app/ui/resources_listbox.h"

namespace doc {
  class Palette;
}

namespace app {

  class PalettesListBox : public ResourcesListBox {
  public:
    PalettesListBox();

    doc::Palette* selectedPalette();

    obs::signal<void(doc::Palette*)> PalChange;

  protected:
    virtual void onResourceChange(Resource* resource) override;
    virtual void onPaintResource(ui::Graphics* g, const gfx::Rect& bounds, Resource* resource) override;
    virtual void onResourceSizeHint(Resource* resource, gfx::Size& size) override;
  };

} // namespace app

#endif
