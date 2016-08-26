// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/palettes_listbox.h"

#include "app/modules/palettes.h"
#include "app/res/palette_resource.h"
#include "app/res/palettes_loader_delegate.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "doc/palette.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/view.h"

namespace app {

using namespace ui;

PalettesListBox::PalettesListBox()
  : ResourcesListBox(new ResourcesLoader(new PalettesLoaderDelegate))
{
}

doc::Palette* PalettesListBox::selectedPalette()
{
  Resource* resource = selectedResource();
  if (!resource)
    return NULL;

  return static_cast<PaletteResource*>(resource)->palette();
}

void PalettesListBox::onResourceChange(Resource* resource)
{
  ResourcesListBox::onResourceChange(resource);

  doc::Palette* palette = static_cast<PaletteResource*>(resource)->palette();
  PalChange(palette);
}

void PalettesListBox::onPaintResource(Graphics* g, const gfx::Rect& bounds, Resource* resource)
{
  doc::Palette* palette = static_cast<PaletteResource*>(resource)->palette();

  gfx::Rect box(
    bounds.x, bounds.y+bounds.h-6*guiscale(),
    4*guiscale(), 4*guiscale());

  for (int i=0; i<palette->size(); ++i) {
    doc::color_t c = palette->getEntry(i);

    g->fillRect(gfx::rgba(
        doc::rgba_getr(c),
        doc::rgba_getg(c),
        doc::rgba_getb(c)), box);

    box.x += box.w;
  }

  // g->drawString(getText(), fgcolor, gfx::ColorNone, false,
  //   gfx::Point(
  //     bounds.x + guiscale()*2,
  //     bounds.y + bounds.h/2 - g->measureUIString(getText()).h/2));
}

void PalettesListBox::onResourceSizeHint(Resource* resource, gfx::Size& size)
{
  size = gfx::Size(0, (2+16+2)*guiscale());
}

} // namespace app
