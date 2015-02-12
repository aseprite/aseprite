// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_SKIN_BUTTON_ICON_IMPL_H_INCLUDED
#define APP_UI_SKIN_BUTTON_ICON_IMPL_H_INCLUDED
#pragma once

#include "ui/button.h"

namespace app {
  namespace skin {

    class SkinTheme;

    class ButtonIconImpl : public ui::IButtonIcon {
    public:
      ButtonIconImpl(SkinTheme* theme,
                     int normalIcon,
                     int selectedIcon,
                     int disabledIcon,
                     int iconAlign);

      // IButtonIcon implementation
      void destroy();
      int getWidth();
      int getHeight();
      she::Surface* getNormalIcon();
      she::Surface* getSelectedIcon();
      she::Surface* getDisabledIcon();
      int getIconAlign();

    public:
      SkinTheme* m_theme;
      int m_normalIcon;
      int m_selectedIcon;
      int m_disabledIcon;
      int m_iconAlign;
    };

  } // namespace skin
} // namespace app

#endif
