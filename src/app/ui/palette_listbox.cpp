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

#include "app/ui/palette_listbox.h"

#include "app/modules/palettes.h"
#include "app/palettes_loader.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "raster/palette.h"
#include "ui/graphics.h"
#include "ui/listitem.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"
#include "ui/view.h"

namespace app {

using namespace ui;
using namespace skin;

class PaletteListItem : public ListItem {
public:
  PaletteListItem(raster::Palette* palette, const std::string& name)
    : ListItem(name), m_palette(palette) {
  }

  raster::Palette* palette() const {
    return m_palette;
  }

protected:
  bool onProcessMessage(ui::Message* msg) OVERRIDE {
    switch (msg->type()) {
      case kMouseLeaveMessage:
      case kMouseEnterMessage:
        invalidate();
        break;
    }
    return ListItem::onProcessMessage(msg);
  }

  void onPaint(PaintEvent& ev) OVERRIDE {
    SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
    Graphics* g = ev.getGraphics();
    gfx::Rect bounds = getClientBounds();
    ui::Color bgcolor, fgcolor;

    if (isSelected()) {
      bgcolor = theme->getColor(ThemeColor::ListItemSelectedFace);
      fgcolor = theme->getColor(ThemeColor::ListItemSelectedText);
    }
    else {
      bgcolor = theme->getColor(ThemeColor::ListItemNormalFace);
      fgcolor = theme->getColor(ThemeColor::ListItemNormalText);
    }

    g->fillRect(bgcolor, bounds);

    gfx::Rect box(
      bounds.x, bounds.y+bounds.h-6*jguiscale(),
      4*jguiscale(), 4*jguiscale());

    for (int i=0; i<m_palette->size(); ++i) {
      raster::color_t c = m_palette->getEntry(i);

      g->fillRect(ui::rgba(
          raster::rgba_getr(c),
          raster::rgba_getg(c),
          raster::rgba_getb(c)), box);

      box.x += box.w;
    }

    g->drawString(getText(), fgcolor, ui::ColorNone, false,
      gfx::Point(
        bounds.x + jguiscale()*2,
        bounds.y + bounds.h/2 - g->measureString(getText()).h/2));
  }

  void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE {
    ev.setPreferredSize(
      gfx::Size(0, (2+16+2)*jguiscale()));
  }

private:
  base::UniquePtr<raster::Palette> m_palette;
};

class PaletteListBox::LoadingItem : public ListItem {
public:
  LoadingItem()
    : ListItem("Loading")
    , m_state(0) {
  }

  void makeProgress() {
    std::string text = "Loading ";

    switch ((++m_state) % 4) {
      case 0: text += "/"; break;
      case 1: text += "-"; break;
      case 2: text += "\\"; break;
      case 3: text += "|"; break;
    }

    setText(text);
  }

private:
  int m_state;
};

PaletteListBox::PaletteListBox()
  : m_palettesTimer(100)
  , m_loadingItem(NULL)
{
  m_palettesTimer.Tick.connect(Bind<void>(&PaletteListBox::onTick, this));
}

raster::Palette* PaletteListBox::selectedPalette()
{
  if (PaletteListItem* item = dynamic_cast<PaletteListItem*>(getSelectedChild()))
    return item->palette();
  else
    return NULL;
}

bool PaletteListBox::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage: {
      if (m_palettesLoader == NULL) {
        m_palettesLoader.reset(new PalettesLoader());
        m_palettesTimer.start();
      }
      break;
    }

  }
  return ListBox::onProcessMessage(msg);
}

void PaletteListBox::onChangeSelectedItem()
{
  raster::Palette* palette = selectedPalette();
  if (palette)
    PalChange(palette);
}

void PaletteListBox::onTick()
{
  if (m_palettesLoader == NULL) {
    stop();
    return;
  }

  if (!m_loadingItem) {
    m_loadingItem = new LoadingItem;
    addChild(m_loadingItem);
  }
  m_loadingItem->makeProgress();
    
  base::UniquePtr<raster::Palette> palette;
  std::string name;

  if (!m_palettesLoader->next(palette, name)) {
    if (m_palettesLoader->isDone()) {
      stop();

      PRINTF("Done\n");
    }
    return;
  }

  base::UniquePtr<PaletteListItem> item(new PaletteListItem(palette, name));
  insertChild(getItemsCount()-1, item);
  layout();

  View* view = View::getView(this);
  if (view)
    view->updateView();

  palette.release();
  item.release();
}

void PaletteListBox::stop()
{
  if (m_loadingItem) {
    removeChild(m_loadingItem);
    delete m_loadingItem;
    m_loadingItem = NULL;

    invalidate();
  }

  m_palettesTimer.stop();
}

} // namespace app
