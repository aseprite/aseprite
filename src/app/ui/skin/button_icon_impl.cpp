// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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

gfx::Size ButtonIconImpl::size()
{
  return m_normalIcon ? m_normalIcon->size(): gfx::Size(0, 0);
}

she::Surface* ButtonIconImpl::normalIcon()
{
  return m_normalIcon ? m_normalIcon->bitmap(0): nullptr;
}

she::Surface* ButtonIconImpl::selectedIcon()
{
  return m_selectedIcon ? m_selectedIcon->bitmap(0): nullptr;
}

she::Surface* ButtonIconImpl::disabledIcon()
{
  return m_disabledIcon ? m_disabledIcon->bitmap(0): nullptr;
}

int ButtonIconImpl::iconAlign()
{
  return m_iconAlign;
}

} // namespace app
