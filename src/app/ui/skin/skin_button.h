// Aseprite
// Copyright (C) 2001-2015, 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SKIN_SKIN_BUTTON_H_INCLUDED
#define APP_UI_SKIN_SKIN_BUTTON_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_theme.h"
#include "she/surface.h"
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
                 const SkinPartPtr& partSelected,
                 const SkinPartPtr& partIcon)
        : Base("")
        , m_partNormal(partNormal)
        , m_partHot(partHot)
        , m_partSelected(partSelected)
        , m_partIcon(partIcon)
      {
      }

      void setIcon(const SkinPartPtr& partIcon) {
        m_partIcon = partIcon;
        Base::invalidate();
      }

    protected:
      void onPaint(ui::PaintEvent& ev) override {
        gfx::Rect bounds(Base::clientBounds());
        ui::Graphics* g = ev.graphics();
        SkinPartPtr part;
        gfx::Color fg;

        if (Base::isSelected()) {
          fg = SkinTheme::instance()->colors.buttonSelectedText();
          part = m_partSelected;
        }
        else if (Base::hasMouseOver()) {
          fg = SkinTheme::instance()->colors.buttonHotText();
          part = m_partHot;
        }
        else {
          fg = SkinTheme::instance()->colors.buttonNormalText();
          part = m_partNormal;
        }

        g->drawRgbaSurface(part->bitmap(0), bounds.x, bounds.y);
        gfx::Size sz(part->bitmap(0)->width(),
                     part->bitmap(0)->height());

        part = m_partIcon;
        g->drawColoredRgbaSurface(part->bitmap(0), fg,
                                  bounds.x+sz.w/2-part->bitmap(0)->width()/2,
                                  bounds.y+sz.h/2-part->bitmap(0)->height()/2);
      }

    private:
      SkinPartPtr m_partNormal;
      SkinPartPtr m_partHot;
      SkinPartPtr m_partSelected;
      SkinPartPtr m_partIcon;
    };

  } // namespace skin
} // namespace app

#endif
