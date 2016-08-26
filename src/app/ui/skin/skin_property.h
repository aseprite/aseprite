// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SKIN_SKIN_PROPERTY_H_INCLUDED
#define APP_UI_SKIN_SKIN_PROPERTY_H_INCLUDED
#pragma once

#include "base/shared_ptr.h"
#include "ui/property.h"

namespace ui {
  class Widget;
}

namespace app {
  namespace skin {

    enum LookType {
      NormalLook,
      MiniLook,
      WithoutBordersLook,
      LeftButtonLook,
      RightButtonLook,
    };

    // Property to show widgets with a special look (e.g.: buttons or sliders with mini-borders)
    class SkinProperty : public ui::Property {
    public:
      static const char* Name;

      SkinProperty();
      ~SkinProperty();

      LookType getLook() const { return m_look; }
      void setLook(LookType look) { m_look = look; }

      bool hasMiniFont() const { return m_miniFont; }
      void setMiniFont() { m_miniFont = true; }

      int getUpperLeft() const { return m_upperLeft; }
      int getUpperRight() const { return m_upperRight; }
      int getLowerLeft() const { return m_lowerLeft; }
      int getLowerRight() const { return m_lowerRight; }

      void setUpperLeft(int value) { m_upperLeft = value; }
      void setUpperRight(int value) { m_upperRight = value; }
      void setLowerLeft(int value) { m_lowerLeft = value; }
      void setLowerRight(int value) { m_lowerRight = value; }

    private:
      LookType m_look;
      bool m_miniFont;
      int m_upperLeft;
      int m_upperRight;
      int m_lowerLeft;
      int m_lowerRight;
    };

    typedef base::SharedPtr<SkinProperty> SkinPropertyPtr;

    SkinPropertyPtr get_skin_property(ui::Widget* widget);

  } // namespace skin
} // namespace app

#endif
