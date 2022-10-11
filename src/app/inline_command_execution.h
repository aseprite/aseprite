// Aseprite
// Copyright (C) 2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_INLINE_COMMAND_EXECUTION_H_INCLUDED
#define APP_INLINE_COMMAND_EXECUTION_H_INCLUDED
#pragma once

#include "app/commands/command.h"

namespace app {

// This class was created to simulate a Command Execution behavior.
// Particularly, this class is used in color_bar.cpp
// (solved issue: locked ContextWriter during a ui palette changes
// while the current editor state is MovingPixelsState)

class InlineCommandExecution : public Command {
public:
  InlineCommandExecution(Context* ctx)
    : Command("", CmdUIOnlyFlag)
    , m_context(ctx)
  {
    CommandExecutionEvent ev(this);
    m_context->BeforeCommandExecution(ev);
  }
  ~InlineCommandExecution()
  {
    CommandExecutionEvent ev(this);
    m_context->AfterCommandExecution(ev);
  }
private:
  Context* m_context;
};

} // namespace app

#endif
