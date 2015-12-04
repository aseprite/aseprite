// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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
