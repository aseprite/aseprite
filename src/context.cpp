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

#include <algorithm>

#include "console.h"
#include "context.h"
#include "commands/command.h"
#include "raster/sprite.h"

Context::Context(ISettings* settings)
{
  m_currentSprite = NULL;
  m_settings = settings;
}

Context::~Context()
{
  for (SpriteList::iterator
	 it = m_sprites.begin(); it != m_sprites.end(); ++it) {
    Sprite* sprite = *it;
    delete sprite;
  }
  m_sprites.clear();

  delete m_settings;
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
  ASSERT(sprite != NULL);

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
  ASSERT(sprite != NULL);

  m_sprites.push_front(sprite);

  // generate on_add_sprite event
  on_add_sprite(sprite);
}

void Context::remove_sprite(Sprite* sprite)
{
  ASSERT(sprite != NULL);

  SpriteList::iterator it = std::find(m_sprites.begin(), m_sprites.end(), sprite);
  ASSERT(it != m_sprites.end());

  // remove the item from the sprites list
  m_sprites.erase(it);

  // generate on_remove_sprite event
  on_remove_sprite(sprite);

  // the current sprite cannot be the removed sprite anymore
  if (m_currentSprite == sprite)
    set_current_sprite(NULL);
}

void Context::send_sprite_to_top(Sprite* sprite)
{
  ASSERT(sprite);

  SpriteList::iterator it = std::find(m_sprites.begin(), m_sprites.end(), sprite);
  ASSERT(it != m_sprites.end());

  // remove the item from the sprites list
  m_sprites.erase(it);

  // add it again
  m_sprites.push_front(sprite);
}

Sprite* Context::get_current_sprite() const
{
  return m_currentSprite;
}

void Context::set_current_sprite(Sprite* sprite)
{
  m_currentSprite = sprite;

  on_set_current_sprite(sprite);
}

void Context::execute_command(Command* command, Params* params)
{
  Console console;

  ASSERT(command != NULL);

  PRINTF("Executing '%s' command.\n", command->short_name());

  try {
    if (params)
      command->loadParams(params);

    if (command->isEnabled(this))
      command->execute(this);
  }
  catch (ase_exception& e) {
    PRINTF("ase_exception caught executing '%s' command\n%s\n",
	   command->short_name(), e.what());

    e.show();
  }
  catch (std::exception& e) {
    PRINTF("std::exception caught executing '%s' command\n%s\n",
	   command->short_name(), e.what());

    console.printf("An error ocurred executing the command.\n\nDetails:\n%s", e.what());
  }
#ifndef DEBUGMODE
  catch (...) {
    PRINTF("unknown exception executing '%s' command\n",
	   command->short_name());

    console.printf("An unknown error ocurred executing the command.\n"
  		   "Please save your work, close the program, try it\n"
		   "again, and report this bug.\n\n"
  		   "Details: Unknown exception caught. This can be bad\n"
		   "memory access, divison by zero, etc.");
  }
#endif
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
