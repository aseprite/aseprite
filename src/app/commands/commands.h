// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_COMMANDS_H_INCLUDED
#define APP_COMMANDS_COMMANDS_H_INCLUDED
#pragma once

#include "app/commands/command_ids.h"
#include "ui/base.h"

#include <map>
#include <string>
#include <vector>

namespace app {
  class Command;

  class Commands {
    static Commands* m_instance;

  public:
    Commands();
    ~Commands();

    static Commands* instance();

    Command* byId(const char* id);
    Commands* add(Command* command);

    // Remove the command but doesn't delete it
    void remove(Command* command);

    void getAllIds(std::vector<std::string>& ids);

  private:
    std::map<std::string, Command*> m_commands;
  };

} // namespace app

#endif
