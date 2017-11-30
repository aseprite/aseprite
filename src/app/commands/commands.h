// Aseprite
// Copyright (C) 2001-2015, 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_COMMANDS_H_INCLUDED
#define APP_COMMANDS_COMMANDS_H_INCLUDED
#pragma once

#include "ui/base.h"

#include <map>
#include <string>

namespace app {

  struct CommandId {
#undef FOR_EACH_COMMAND
#define FOR_EACH_COMMAND(Name)                  \
    static const char* Name;
#include "app/commands/commands_list.h"
#undef FOR_EACH_COMMAND
  };

  class Command;

  class Commands {
    static Commands* m_instance;

  public:
    Commands();
    ~Commands();

    static Commands* instance();

    Command* byId(const char* id);
    void add(Command* command);

  private:
    std::map<std::string, Command*> m_commands;
  };

} // namespace app

#endif
