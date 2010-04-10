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

#ifndef CONTEXT_H_INCLUDED
#define CONTEXT_H_INCLUDED

#include <list>
#include "Vaca/NonCopyable.h"
#include "ase_exception.h"
#include "settings/settings.h"

class Sprite;
class SpriteReader;
class Command;
class Params;

typedef std::list<Sprite*> SpriteList;

class CommandPreconditionException : public ase_exception
{
public:
  CommandPreconditionException() throw()
  : ase_exception("Cannot execute the command because its pre-conditions are false.") { }
};

class Context : Vaca::NonCopyable
{
public:
  virtual ~Context();

  virtual bool is_ui_available() const		{ return false; }
  virtual bool is_recording_macro() const	{ return false; }
  virtual bool is_executing_macro() const	{ return false; }
  virtual bool is_executing_script() const	{ return false; }

  ISettings* getSettings() { return m_settings; }

  const SpriteList& get_sprite_list() const;
  Sprite* get_first_sprite() const;
  Sprite* get_next_sprite(Sprite* sprite) const;

  void add_sprite(Sprite* sprite);
  void remove_sprite(Sprite* sprite);
  void send_sprite_to_top(Sprite* sprite);

  Sprite* get_current_sprite() const;
  void set_current_sprite(Sprite* sprite);

  virtual void execute_command(Command* command, Params* params = NULL);

protected:

  // The "settings" are deleted automatically in the ~Context destructor
  Context(ISettings* settings);

  virtual void on_add_sprite(Sprite* sprite);
  virtual void on_remove_sprite(Sprite* sprite);
  virtual void on_set_current_sprite(Sprite* sprite);

private:

  // Without default constructor
  Context();

  // List of all sprites.
  SpriteList m_sprites;

  // Current selected sprite to operate.
  Sprite* m_currentSprite;

  // Settings in this context.
  ISettings* m_settings;

};

#endif
