// Aseprite
// Copyright (C) 2021-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/commands/debugger.h"

#include "app/app.h"

namespace app {

using namespace ui;

DebuggerCommand::DebuggerCommand() : Command(CommandId::Debugger())
{
}

bool DebuggerCommand::onEnabled(Context*)
{
  return false;
}

void DebuggerCommand::onExecute(Context*)
{
  // TODO: Implement a DAP host and the UI to turn it on/off.
}

Command* CommandFactory::createDebuggerCommand()
{
  return new DebuggerCommand;
}

} // namespace app
