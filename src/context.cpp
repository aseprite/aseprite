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

#include <cassert>
#include <algorithm>

#include "console.h"
#include "context.h"
#include "commands/command.h"
#include "raster/sprite.h"

Context::Context()
{
  m_current_sprite = NULL;
}

Context::~Context()
{
  for (SpriteList::iterator
	 it = m_sprites.begin(); it != m_sprites.end(); ++it) {
    Sprite* sprite = *it;
    delete sprite;
  }
  m_sprites.clear();
}

int Context::get_fg_color()
{
  return 0;			// TODO
}

int Context::get_bg_color()
{
  return 0;			// TODO
}

const SpriteList& Context::get_sprite_list() const
{
  return m_sprites;
}

Sprite* Context::get_first_sprite() const
{
  if (!m_sprites.empty())
    return m_sprites.front();
  else
    return NULL;
}

Sprite* Context::get_next_sprite(Sprite* sprite) const
{
  assert(sprite != NULL);

  SpriteList::const_iterator it = std::find(m_sprites.begin(), m_sprites.end(), sprite);

  if (it != m_sprites.end()) {
    ++it;
    if (it != m_sprites.end())
      return *it;
  }
  return NULL;
}

/**
 * Append the sprite to the context's sprites' list.
 */
void Context::add_sprite(Sprite* sprite)
{
  assert(sprite != NULL);

  m_sprites.push_front(sprite);

  // generate on_add_sprite event
  on_add_sprite(sprite);
}

void Context::remove_sprite(Sprite* sprite)
{
  assert(sprite != NULL);

  SpriteList::iterator it = std::find(m_sprites.begin(), m_sprites.end(), sprite);
  assert(it != m_sprites.end());

  // remove the item from the sprites list
  m_sprites.erase(it);

  // generate on_remove_sprite event
  on_remove_sprite(sprite);

  // the current sprite cannot be the removed sprite anymore
  if (m_current_sprite == sprite)
    set_current_sprite(NULL);
}

void Context::send_sprite_to_top(Sprite* sprite)
{
  assert(sprite);

  SpriteList::iterator it = std::find(m_sprites.begin(), m_sprites.end(), sprite);
  assert(it != m_sprites.end());

  // remove the item from the sprites list
  m_sprites.erase(it);

  // add it again
  m_sprites.push_front(sprite);
}

Sprite* Context::get_current_sprite() const
{
  return m_current_sprite;
}

void Context::set_current_sprite(Sprite* sprite)
{
  m_current_sprite = sprite;

  on_set_current_sprite(sprite);
}

void Context::execute_command(Command* command, Params* params)
{
  Console console;

  assert(command != NULL);

  try {
    if (params)
      command->load_params(params);

    if (command->enabled(this))
      command->execute(this);
  }
  catch (ase_exception& e) {
    e.show();
  }
  catch (std::exception& e) {
    console.printf("An error ocurred executing the command.\n\nDetails:\n%s", e.what());
  }
  catch (...) {
    console.printf("An unknown error ocurred executing the command.\n"
		   "Please try again or report this bug.\n\n"
		   "Details: Unknown exception caught.");
  }
}

void Context::on_add_sprite(Sprite* sprite)
{
  // do nothing
}

void Context::on_remove_sprite(Sprite* sprite)
{
  // do nothing
}

void Context::on_set_current_sprite(Sprite* sprite)
{
  // do nothing
}
