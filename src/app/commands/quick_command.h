// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_QUICK_COMMAND_H_INCLUDED
#define APP_COMMANDS_QUICK_COMMAND_H_INCLUDED
#pragma once

#include "app/commands/command.h"

#include <functional>

namespace app {

  class QuickCommand : public Command {
  public:
    QuickCommand(const char* id, std::function<void()> execute);
    ~QuickCommand();

  protected:
    void onExecute(Context* context) override;

    std::function<void()> m_execute;
  };

} // namespace app

#endif
