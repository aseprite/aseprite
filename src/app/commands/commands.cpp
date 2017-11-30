// Aseprite
// Copyright (C) 2001-2017  David Capello
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

Commands* Commands::m_instance = NULL;

Commands::Commands()
{
  ASSERT(m_instance == NULL);
  m_instance = this;

  #undef FOR_EACH_COMMAND
  #define FOR_EACH_COMMAND(Name) \
    m_commands.push_back(CommandFactory::create##Name##Command());

  #include "app/commands/commands_list.h"
  #undef FOR_EACH_COMMAND
}

Commands::~Commands()
{
  ASSERT(m_instance == this);

  for (Command* cmd : m_commands)
    delete cmd;

  m_commands.clear();
  m_instance = NULL;
}

Commands* Commands::instance()
{
  ASSERT(m_instance != NULL);
  return m_instance;
}

Command* Commands::byId(const char* id)
{
  if (!id)
    return nullptr;

  std::string lid = base::string_to_lower(id);
  for (Command* cmd : m_commands) {
    if (base::utf8_icmp(cmd->id(), lid) == 0)
      return cmd;
  }

  return nullptr;
}

} // namespace app
