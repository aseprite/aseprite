// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_SKIN_SKIN_SLIDER_PROPERTY_H_INCLUDED
#define APP_UI_SKIN_SKIN_SLIDER_PROPERTY_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_property.h"
#include "base/shared_ptr.h"
#include "gfx/rect.h"

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

    typedef base::SharedPtr<SkinSliderProperty> SkinSliderPropertyPtr;

  } // namespace skin
} // namespace app

#endif
