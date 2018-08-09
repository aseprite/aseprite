// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_BRUSH_POPUP_H_INCLUDED
#define APP_UI_BRUSH_POPUP_H_INCLUDED
#pragma once

#include "app/ui/button_set.h"
#include "base/shared_ptr.h"
#include "doc/brushes.h"
#include "ui/box.h"
#include "ui/popup_window.h"
#include "ui/tooltips.h"

namespace app {

  class BrushPopup : public ui::PopupWindow {
  public:
    BrushPopup();

    void setBrush(doc::Brush* brush);
    void regenerate(const gfx::Rect& box);

    void setupTooltips(ui::TooltipManager* tooltipManager) {
      m_tooltipManager = tooltipManager;
    }

    static os::Surface* createSurfaceForBrush(const doc::BrushRef& brush);

  private:
    void onStandardBrush();
    void onBrushChanges();

    ui::TooltipManager* m_tooltipManager;
    ui::VBox m_box;
    ButtonSet m_standardBrushes;
    ButtonSet* m_customBrushes;
  };

} // namespace app

#endif
