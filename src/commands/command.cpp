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

#include "console.h"
#include "commands/command.h"
#include "commands/params.h"

Command::Command(const char* short_name, const char* friendly_name, CommandFlags flags)
{
  m_short_name = short_name;
  m_friendly_name = friendly_name;
  m_flags = flags;
}

Command::~Command()
{
}

/**
 * Converts specified parameters to class members.
 */
void Command::load_params(Params* params)
{
  // do nothing
}

/**
 * Preconditions to execute the command
 */
bool Command::enabled(Context* context)
{
  return true;
}

/**
 * Should the menu-item be checked?
 */
bool Command::checked(Context* context)
{
  return false;
}

/**
 * Execute the command (after checking the preconditions).
 */
void Command::execute(Context* context)
{
  // do nothing
}
