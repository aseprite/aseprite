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

#ifndef COMMANDS_COMMANDS_H_INCLUDED
#define COMMANDS_COMMANDS_H_INCLUDED

#include "jinete/jbase.h"

struct CommandId
{
#undef FOR_EACH_COMMAND
  #define FOR_EACH_COMMAND(name) \
    static const char* name;
  #include "commands/commands_list.h"
#undef FOR_EACH_COMMAND
};

#include <vector>

class Command;

typedef std::vector<Command*> CommandsList;

class CommandsModule
{
  static CommandsModule* m_instance;
  CommandsList m_commands;

public:
  CommandsModule();
  ~CommandsModule();

  static CommandsModule* instance();

  Command* get_command_by_name(const char* name);
};

#endif
