// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/skin/button_icon_impl.h"

#include "app/ui/skin/skin_theme.h"
#include "she/surface.h"

namespace app {

using namespace app::skin;

ButtonIconImpl::ButtonIconImpl(SkinTheme* theme,
                               int normalIcon,
                               int selectedIcon,
                               int disabledIcon,
                               int iconAlign)
  : m_theme(theme)
  , m_normalIcon(normalIcon)
  , m_selectedIcon(selectedIcon)
  , m_disabledIcon(disabledIcon)
  , m_iconAlign(iconAlign)
{
  ASSERT(theme != NULL);
}

void ButtonIconImpl::destroy()
{
  delete this;
}

int ButtonIconImpl::getWidth()
{
  return m_theme->get_part(m_normalIcon)->width();
}

int ButtonIconImpl::getHeight()
{
  return m_theme->get_part(m_normalIcon)->height();
}

she::Surface* ButtonIconImpl::getNormalIcon()
{
  return m_theme->get_part(m_normalIcon);
}

she::Surface* ButtonIconImpl::getSelectedIcon()
{
  return m_theme->get_part(m_selectedIcon);
}

she::Surface* ButtonIconImpl::getDisabledIcon()
{
  return m_theme->get_part(m_disabledIcon);
}

int ButtonIconImpl::getIconAlign()
{
  return m_iconAlign;
}

} // namespace app
