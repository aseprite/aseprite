// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
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

Commands* Commands::m_instance = NULL;

Commands::Commands()
{
  ASSERT(m_instance == NULL);
  m_instance = this;

  #undef FOR_EACH_COMMAND
  #define FOR_EACH_COMMAND(Name) \
    add(CommandFactory::create##Name##Command());

  #include "app/commands/commands_list.h"
  #undef FOR_EACH_COMMAND
}

Commands::~Commands()
{
  ASSERT(m_instance == this);

  for (auto& it : m_commands) {
    Command* command = it.second;
    delete command;
  }

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

  auto lid = base::string_to_lower(id);
  auto it = m_commands.find(lid);
  return (it != m_commands.end() ? it->second: nullptr);
}

Commands* Commands::add(Command* command)
{
  auto lid = base::string_to_lower(command->id());
  m_commands[lid] = command;
  return this;
}

void Commands::remove(Command* command)
{
  auto lid = base::string_to_lower(command->id());
  auto it = m_commands.find(lid);
  ASSERT(it != m_commands.end());
  if (it != m_commands.end())
    m_commands.erase(it);
}

void Commands::getAllIds(std::vector<std::string>& ids)
{
  for (auto& it : m_commands)
    ids.push_back(it.second->id());
}

} // namespace app
