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

#ifndef ASE_CURRENT_SPRITE_H
#define ASE_CURRENT_SPRITE_H

#include <list>

class Context;
class Sprite;

class CurrentSprite
{
  Context* m_context;
  Sprite* m_sprite;
  bool m_writeable;

  // // No-default constructor (undefined)
  // CurrentSprite();

  // Non-copyable
  CurrentSprite(const CurrentSprite&);
  CurrentSprite& operator=(const CurrentSprite&);

public:
  CurrentSprite();
  CurrentSprite(Context* context);
  ~CurrentSprite();

  bool writeable() const { return m_writeable; }
  void destroy();

  operator Sprite* () { return m_sprite; }

  Sprite* operator->() {
    assert(m_sprite != NULL);
    return m_sprite;
  }

};

#endif // ASE_CURRENT_SPRITE_H
