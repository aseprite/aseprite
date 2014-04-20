/* Aseprite
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

    typedef SharedPtr<SkinSliderProperty> SkinSliderPropertyPtr;

  } // namespace skin
} // namespace app

#endif
