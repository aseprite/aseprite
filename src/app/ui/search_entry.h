// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SEARCH_ENTRY_H_INCLUDED
#define APP_UI_SEARCH_ENTRY_H_INCLUDED
#pragma once

#include "ui/entry.h"

namespace app {

class SearchEntry : public ui::Entry {
public:
  SearchEntry();

private:
  bool onProcessMessage(ui::Message* msg) override;
  void onPaint(ui::PaintEvent& ev) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  gfx::Rect onGetEntryTextBounds() const override;

  gfx::Rect getCloseIconBounds() const;
};

} // namespace app

#endif
