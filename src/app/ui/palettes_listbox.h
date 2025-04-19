// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_PALETTES_LISTBOX_H_INCLUDED
#define APP_UI_PALETTES_LISTBOX_H_INCLUDED
#pragma once

#include "app/ui/resources_listbox.h"
#include "app/ui/separator_in_view.h"
#include "obs/connection.h"
#include "ui/tooltips.h"

namespace doc {
class Palette;
}

namespace app {

class PalettesListBox : public ResourcesListBox {
public:
  PalettesListBox();

  const doc::Palette* selectedPalette();

  void sortItems() override;
  void togglePinned(const std::string& paletteId);
  void loadPinned();

  obs::signal<void(const doc::Palette*)> PalChange;

protected:
  virtual ResourceListItem* onCreateResourceItem(Resource* resource) override;
  virtual void onResourceChange(Resource* resource) override;
  virtual void onPaintResource(ui::Graphics* g, gfx::Rect& bounds, Resource* resource) override;
  virtual void onResourceSizeHint(Resource* resource, gfx::Size& size) override;
  void loadPinnedQuiet();
  void savePinned();
  void toggleSeparator();

private:
  std::vector<std::string> m_pinned;
  ui::TooltipManager m_tooltips;
  app::SeparatorInView m_pinnedSeparator;
  obs::scoped_connection m_extPaletteChanges;
  obs::scoped_connection m_extPresetsChanges;
};

} // namespace app

#endif
