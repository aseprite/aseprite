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

#ifndef ASE_CONTEXT_H
#define ASE_CONTEXT_H

#include <list>

class Sprite;

typedef std::list<Sprite*> SpriteList;

class Context
{
  /**
   * List of all sprites.
   */
  SpriteList m_sprites;

  /**
   * Current selected sprite to operate.
   */
  Sprite* m_current_sprite;

public:

  Context();
  virtual ~Context();

  const SpriteList& get_sprite_list() const;
  Sprite* get_first_sprite() const;
  Sprite* get_next_sprite(Sprite* sprite) const;

  void add_sprite(Sprite* sprite);
  void remove_sprite(Sprite* sprite);
  void send_sprite_to_top(Sprite* sprite);

  Sprite* get_current_sprite() const;
  void set_current_sprite(Sprite* sprite);

protected:

  virtual void on_add_sprite(Sprite* sprite);
  virtual void on_remove_sprite(Sprite* sprite);
  virtual void on_set_current_sprite(Sprite* sprite);

};

#endif // ASE_CONTEXT_H

