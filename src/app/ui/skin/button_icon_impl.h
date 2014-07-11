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
