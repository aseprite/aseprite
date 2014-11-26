/* Aseprite
 * Copyright (C) 2014  David Capello
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
#include "ui/preferred_size_event.h"
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

void PalettesListBox::onResourcePreferredSize(Resource* resource, gfx::Size& size)
{
  size = gfx::Size(0, (2+16+2)*guiscale());
}

} // namespace app
