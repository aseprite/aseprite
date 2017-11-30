// Aseprite
// Copyright (C) 2001-2015, 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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

  class Commands {
    static Commands* m_instance;
    CommandsList m_commands;

  public:
    Commands();
    ~Commands();

    static Commands* instance();

    Command* byId(const char* id);
  };

} // namespace app

#endif
