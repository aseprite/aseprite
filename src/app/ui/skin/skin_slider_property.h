// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SKIN_SKIN_SLIDER_PROPERTY_H_INCLUDED
#define APP_UI_SKIN_SKIN_SLIDER_PROPERTY_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_property.h"
#include "gfx/rect.h"

#include <memory>

namespace ui {
  class Slider;
  class Graphics;
}

namespace app {
  namespace skin {

    class ISliderBgPainter {
    public:
      virtual ~ISliderBgPainter() { }
      virtual void paint(ui::Slider* slider, ui::Graphics* graphics, const gfx::Rect& rc) = 0;
    };

    class SkinSliderProperty : public ui::Property {
    public:
      static const char* Name;

      // The given painter is deleted automatically when this
      // property the destroyed.
      SkinSliderProperty(ISliderBgPainter* painter);
      ~SkinSliderProperty();

      ISliderBgPainter* getBgPainter() const;

    private:
      ISliderBgPainter* m_painter;
    };

  } // namespace skin
} // namespace app

#endif
