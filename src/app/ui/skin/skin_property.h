// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SKIN_SKIN_PROPERTY_H_INCLUDED
#define APP_UI_SKIN_SKIN_PROPERTY_H_INCLUDED
#pragma once

#include "ui/property.h"

#include <memory>

namespace ui {
  class Widget;
}

namespace app {
  namespace skin {

    enum LookType {
      NormalLook,
      MiniLook,
      WithoutBordersLook,
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

    private:
      LookType m_look;
      bool m_miniFont;
    };

    typedef std::shared_ptr<SkinProperty> SkinPropertyPtr;

    SkinPropertyPtr get_skin_property(ui::Widget* widget);

  } // namespace skin
} // namespace app

#endif
