// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_DEBUGGER_H_INCLUDED
#define APP_COMMANDS_DEBUGGER_H_INCLUDED
#pragma once

#include "app/commands/command.h"

#include <memory>

namespace app {

class Debugger;

class DebuggerCommand : public Command {
public:
  DebuggerCommand();

  void closeDebugger(Context* ctx);

protected:
  void onExecute(Context* context) override;

  std::unique_ptr<Debugger> m_debugger;
};

} // namespace app

#endif
