// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_BRUSH_POPUP_H_INCLUDED
#define APP_UI_BRUSH_POPUP_H_INCLUDED
#pragma once

#include "base/signal.h"
#include "ui/popup_window.h"

namespace doc {
  class Brush;
}

namespace app {
  class ButtonSet;

  class BrushPopup : public ui::PopupWindow {
  public:
    BrushPopup(const gfx::Rect& rc, doc::Brush* brush);

    Signal1<void, doc::Brush*> BrushChange;

  private:
    void onBrushTypeChange();

    ButtonSet* m_brushTypeButton;
  };

} // namespace app

#endif
