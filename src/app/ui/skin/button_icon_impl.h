// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_SKIN_BUTTON_ICON_IMPL_H_INCLUDED
#define APP_UI_SKIN_BUTTON_ICON_IMPL_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_part.h"
#include "ui/button.h"

namespace app {
  namespace skin {

    class SkinTheme;

    class ButtonIconImpl : public ui::IButtonIcon {
    public:
      ButtonIconImpl(const SkinPartPtr& normalIcon,
                     const SkinPartPtr& selectedIcon,
                     const SkinPartPtr& disabledIcon,
                     int iconAlign);

      // IButtonIcon implementation
      void destroy();
      gfx::Size size();
      she::Surface* normalIcon();
      she::Surface* selectedIcon();
      she::Surface* disabledIcon();
      int iconAlign();

    public:
      SkinPartPtr m_normalIcon;
      SkinPartPtr m_selectedIcon;
      SkinPartPtr m_disabledIcon;
      int m_iconAlign;
    };

  } // namespace skin
} // namespace app

#endif
