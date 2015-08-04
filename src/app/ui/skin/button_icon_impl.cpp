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

ButtonIconImpl::ButtonIconImpl(const SkinPartPtr& normalIcon,
                               const SkinPartPtr& selectedIcon,
                               const SkinPartPtr& disabledIcon,
                               int iconAlign)
  : m_normalIcon(normalIcon)
  , m_selectedIcon(selectedIcon)
  , m_disabledIcon(disabledIcon)
  , m_iconAlign(iconAlign)
{
}

void ButtonIconImpl::destroy()
{
  delete this;
}

gfx::Size ButtonIconImpl::getSize()
{
  return m_normalIcon ? m_normalIcon->getSize(): gfx::Size(0, 0);
}

she::Surface* ButtonIconImpl::getNormalIcon()
{
  return m_normalIcon ? m_normalIcon->getBitmap(0): nullptr;
}

she::Surface* ButtonIconImpl::getSelectedIcon()
{
  return m_selectedIcon ? m_selectedIcon->getBitmap(0): nullptr;
}

she::Surface* ButtonIconImpl::getDisabledIcon()
{
  return m_disabledIcon ? m_disabledIcon->getBitmap(0): nullptr;
}

int ButtonIconImpl::getIconAlign()
{
  return m_iconAlign;
}

} // namespace app
