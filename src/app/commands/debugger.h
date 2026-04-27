// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_DEBUGGER_H_INCLUDED
#define APP_COMMANDS_DEBUGGER_H_INCLUDED
#pragma once

#include "app/commands/command.h"

namespace app {

class DebuggerCommand : public Command {
public:
  DebuggerCommand();

protected:
  bool onEnabled(Context* context) override;
  void onExecute(Context* context) override;
};

} // namespace app

#endif
