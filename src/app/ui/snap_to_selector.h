// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SNAP_TO_SELECTOR_H_INCLUDED
#define APP_UI_SNAP_TO_SELECTOR_H_INCLUDED
#pragma once

#include "app/snap_to_grid.h"
#include "ui/combobox.h"

namespace app {

  class SnapToSelector : public ui::ComboBox {
  public:
    SnapToSelector();

    PreferSnapTo snapTo();
    void snapTo(PreferSnapTo snapTo);
  };

} // namespace app

#endif
