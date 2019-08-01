// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_BRUSH_POPUP_H_INCLUDED
#define APP_UI_BRUSH_POPUP_H_INCLUDED
#pragma once

#include "app/ui/button_set.h"
#include "doc/brushes.h"
#include "ui/box.h"
#include "ui/popup_window.h"

namespace app {

  class BrushPopup : public ui::PopupWindow {
  public:
    BrushPopup();

    void setBrush(doc::Brush* brush);
    void regenerate(const gfx::Rect& box);

    static os::Surface* createSurfaceForBrush(const doc::BrushRef& brush);

  private:
    void onStandardBrush();
    void onBrushChanges();

    ui::VBox m_box;
    ButtonSet m_standardBrushes;
    ButtonSet* m_customBrushes;
  };

} // namespace app

#endif
