// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
    if (base::utf8_icmp(cmd->id(), lname) == 0)
      return cmd;
  }

  return NULL;
}

} // namespace app
