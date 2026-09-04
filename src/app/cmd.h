// Aseprite
// Copyright (C) 2023-present  Igara Studio SA
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_CMD_H_INCLUDED
#define APP_CMD_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "undo/undo_command.h"

#include <string>

namespace app {

class Context;

class Cmd : public undo::UndoCommand {
public:
  Cmd();
  virtual ~Cmd();

  void execute(Context* ctx);

  // undo::UndoCommand impl
  void undo(undo::UndoContext* ctx) override;
  void redo(undo::UndoContext* ctx) override;
  void dispose() override;

  std::string label() const;
  size_t memSize() const;

protected:
  virtual void onExecute(Context* ctx);
  virtual void onUndo(Context* ctx);
  virtual void onRedo(Context* ctx);
  virtual void onFireNotifications(Context* ctx);
  virtual std::string onLabel() const;
  virtual size_t onMemSize() const;

private:
#if _DEBUG
  enum class State { NotExecuted, Executed, Undone, Redone };
  State m_state;
#endif

  DISABLE_COPYING(Cmd);
};

} // namespace app

#endif
