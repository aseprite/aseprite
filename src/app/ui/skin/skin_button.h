// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_SKIN_SKIN_BUTTON_H_INCLUDED
#define APP_UI_SKIN_SKIN_BUTTON_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_theme.h"
#include "ui/button.h"
#include "ui/graphics.h"
#include "ui/paint_event.h"

namespace app {
  namespace skin {

    template<typename Base = ui::Button>
    class SkinButton : public Base {
    public:
      SkinButton(const SkinPartPtr& partNormal,
                 const SkinPartPtr& partHot,
                 const SkinPartPtr& partSelected)
        : Base("")
        , m_partNormal(partNormal)
        , m_partHot(partHot)
        , m_partSelected(partSelected)
      {
      }

      void setParts(const SkinPartPtr& partNormal,
                    const SkinPartPtr& partHot,
                    const SkinPartPtr& partSelected) {
        m_partNormal = partNormal;
        m_partHot = partHot;
        m_partSelected = partSelected;
        Base::invalidate();
      }

    protected:
      void onPaint(ui::PaintEvent& ev) override {
        gfx::Rect bounds(Base::clientBounds());
        ui::Graphics* g = ev.graphics();
        SkinPartPtr part;

        if (Base::isSelected())
          part = m_partSelected;
        else if (Base::hasMouseOver())
          part = m_partHot;
        else
          part = m_partNormal;

        g->drawRgbaSurface(part->bitmap(0), bounds.x, bounds.y);
      }

    private:
      SkinPartPtr m_partNormal;
      SkinPartPtr m_partHot;
      SkinPartPtr m_partSelected;
    };

  } // namespace skin
} // namespace app

#endif
