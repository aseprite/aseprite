// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

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

  class BrushPopupDelegate {
  public:
    virtual ~BrushPopupDelegate() { }
    virtual doc::BrushRef onCreateBrushFromActivePreferences() = 0;
    virtual void onSelectBrush(const doc::BrushRef& brush) = 0;
    virtual void onSelectBrushSlot(int slot) = 0;
    virtual void onDeleteBrushSlot(int slot) = 0;
    virtual void onDeleteAllBrushes() = 0;
    virtual bool onIsBrushSlotLocked(int slot) const = 0;
    virtual void onLockBrushSlot(int slot) = 0;
    virtual void onUnlockBrushSlot(int slot) = 0;
  };

  class BrushPopup : public ui::PopupWindow {
  public:
    BrushPopup(BrushPopupDelegate* delegate);

    void setBrush(doc::Brush* brush);
    void regenerate(const gfx::Rect& box);

    void setupTooltips(ui::TooltipManager* tooltipManager) {
      m_tooltipManager = tooltipManager;
    }

    static she::Surface* createSurfaceForBrush(const doc::BrushRef& brush);

  private:
    void onStandardBrush();
    void onBrushChanges();

    ui::TooltipManager* m_tooltipManager;
    ui::VBox m_box;
    ButtonSet m_standardBrushes;
    ButtonSet* m_customBrushes;
    BrushPopupDelegate* m_delegate;
  };

} // namespace app

#endif
