/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "config.h"

#include <allegro/file.h>

#include "app.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "settings/ui_settings_impl.h"
#include "ui_context.h"
#include "widgets/tabs.h"

UIContext* UIContext::m_instance = NULL;

UIContext::UIContext()
  : Context(new UISettingsImpl)
{
  ASSERT(m_instance == NULL);
  m_instance = this;
}

UIContext::~UIContext()
{
  ASSERT(m_instance == this);
  m_instance = NULL;
}

void UIContext::on_add_sprite(Sprite* sprite)
{
  // base method
  Context::on_add_sprite(sprite);

  // add the tab for this sprite
  app_get_tabsbar()->addTab(get_filename(sprite->getFilename()), sprite);

  // rebuild the menu list of sprites
  app_realloc_sprite_list();
}

void UIContext::on_remove_sprite(Sprite* sprite)
{
  // base method
  Context::on_remove_sprite(sprite);

  // remove this sprite from tabs
  app_get_tabsbar()->removeTab(sprite);

  // rebuild the menu list of sprites
  app_realloc_sprite_list();

  // select other sprites in the editors where are this sprite
  editors_hide_sprite(sprite);
}

void UIContext::on_set_current_sprite(Sprite* sprite)
{
  // base method
  Context::on_set_current_sprite(sprite);

  // select the sprite in the tabs
  app_get_tabsbar()->selectTab(sprite);
}
