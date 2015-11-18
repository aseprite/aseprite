// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_COMMANDS_COMMANDS_H_INCLUDED
#define APP_COMMANDS_COMMANDS_H_INCLUDED
#pragma once

#include "ui/base.h"

#include <vector>

namespace app {

  struct CommandId {
#undef FOR_EACH_COMMAND
#define FOR_EACH_COMMAND(Name)                  \
    static const char* Name;
#include "app/commands/commands_list.h"
#undef FOR_EACH_COMMAND
  };

  class Command;
  typedef std::vector<Command*> CommandsList;

  class CommandsModule {
    static CommandsModule* m_instance;
    CommandsList m_commands;

  public:
    CommandsModule();
    ~CommandsModule();

    static CommandsModule* instance();

    Command* getCommandByName(const char* name);

    CommandsList::iterator begin() { return m_commands.begin(); }
    CommandsList::iterator end() { return m_commands.end(); }
  };

} // namespace app

#endif
