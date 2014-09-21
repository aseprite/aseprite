/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/commands.h"

#include "app/commands/command.h"
#include "app/console.h"
#include "base/string.h"
#include "ui/ui.h"

#include <cstring>
#include <exception>

namespace app {

#undef FOR_EACH_COMMAND
  #define FOR_EACH_COMMAND(Name) \
    const char* CommandId::Name = #Name;
  #include "app/commands/commands_list.h"
#undef FOR_EACH_COMMAND

CommandsModule* CommandsModule::m_instance = NULL;

CommandsModule::CommandsModule()
{
  ASSERT(m_instance == NULL);
  m_instance = this;

  #undef FOR_EACH_COMMAND
  #define FOR_EACH_COMMAND(Name) \
    m_commands.push_back(CommandFactory::create##Name##Command());

  #include "app/commands/commands_list.h"
  #undef FOR_EACH_COMMAND
}

CommandsModule::~CommandsModule()
{
  ASSERT(m_instance == this);

  for (Command* cmd : m_commands)
    delete cmd;

  m_commands.clear();
  m_instance = NULL;
}

CommandsModule* CommandsModule::instance()
{
  ASSERT(m_instance != NULL);
  return m_instance;
}

Command* CommandsModule::getCommandByName(const char* name)
{
  if (!name)
    return NULL;

  std::string lname = base::string_to_lower(name);
  for (Command* cmd : m_commands) {
    if (base::string_to_lower(cmd->short_name()) == lname)
      return cmd;
  }

  return NULL;
}

} // namespace app
