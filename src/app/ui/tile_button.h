// Aseprite
// Copyright (c) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_TILE_BUTTON_H_INCLUDED
#define APP_UI_TILE_BUTTON_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/context_observer.h"
#include "app/ui/tile_source.h"
#include "doc/tile.h"
#include "ui/button.h"

namespace app {

class TileButton : public ui::ButtonBase,
                   public ContextObserver,
                   public ITileSource {
public:
  TileButton();
  ~TileButton();

  doc::tile_t getTile() const;
  void setTile(doc::tile_t tile);

  // ITileSource
  doc::tile_t getTileByPosition(const gfx::Point& pos) override;

  // Signals
  obs::signal<void(doc::tile_t&)> BeforeChange;
  obs::signal<void(doc::tile_t)> Change;

protected:
  // Events
  void onInitTheme(ui::InitThemeEvent& ev) override;
  bool onProcessMessage(ui::Message* msg) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;

private:
  // ContextObserver impl
  void onActiveSiteChange(const Site& site) override;

  doc::tile_t m_tile = doc::notile;
};

} // namespace app

#endif
