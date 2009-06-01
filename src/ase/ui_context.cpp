/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include <cassert>
#include <allegro/file.h>

#include "ase/ui_context.h"
#include "core/app.h"
#include "modules/editors.h"
#include "raster/sprite.h"
#include "widgets/tabs.h"

//////////////////////////////////////////////////////////////////////
// UIContext singleton

static UIContext* g_instance = NULL;

UIContext* UIContext::instance()
{
  if (!g_instance)
    g_instance = new UIContext;
  return g_instance;
}

void UIContext::destroy_instance()
{
  delete g_instance;
  g_instance = NULL;
}

//////////////////////////////////////////////////////////////////////

UIContext::UIContext()
{
}

UIContext::~UIContext()
{
}

void UIContext::show_sprite(Sprite* sprite) const
{
  set_sprite_in_more_reliable_editor(sprite);
}

void UIContext::on_add_sprite(Sprite* sprite)
{
  // base method
  Context::on_add_sprite(sprite);

  // add the tab for this sprite
  tabs_append_tab(app_get_tabsbar(),
		  get_filename(sprite->filename), sprite);

  // rebuild the menu list of sprites
  app_realloc_sprite_list();
}

void UIContext::on_remove_sprite(Sprite* sprite)
{
  // base method
  Context::on_remove_sprite(sprite);

  // remove this sprite from tabs
  tabs_remove_tab(app_get_tabsbar(), sprite);

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
  tabs_select_tab(app_get_tabsbar(), sprite);
}
