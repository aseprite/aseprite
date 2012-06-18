/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#ifndef BUTTON_ICON_IMPL_H_INCLUDED
#define BUTTON_ICON_IMPL_H_INCLUDED

#include "ui/button.h"

class SkinTheme;

class ButtonIconImpl : public ui::IButtonIcon
{
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
  BITMAP* getNormalIcon();
  BITMAP* getSelectedIcon();
  BITMAP* getDisabledIcon();
  int getIconAlign();

public:
  SkinTheme* m_theme;
  int m_normalIcon;
  int m_selectedIcon;
  int m_disabledIcon;
  int m_iconAlign;
};

#endif
