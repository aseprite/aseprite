// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_BRUSH_POPUP_H_INCLUDED
#define APP_UI_BRUSH_POPUP_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "base/signal.h"
#include "doc/brushes.h"
#include "ui/popup_window.h"

namespace doc {
  class Brush;
}

namespace ui {
  class TooltipManager;
}

namespace app {
  class ButtonSet;

  class BrushPopup : public ui::PopupWindow {
  public:
    BrushPopup();

    void setBrush(doc::Brush* brush);
    void regenerate(const gfx::Rect& box, const doc::Brushes& brushes);

    void setupTooltips(ui::TooltipManager* tooltipManager) {
      m_tooltipManager = tooltipManager;
    }

    Signal1<void, const doc::BrushRef&> BrushChange;

    static she::Surface* createSurfaceForBrush(const doc::BrushRef& brush);

  private:
    void onButtonChange();

    base::SharedPtr<ButtonSet> m_buttons;
    ui::TooltipManager* m_tooltipManager;
  };

} // namespace app

#endif
