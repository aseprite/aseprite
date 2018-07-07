// Aseprite
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
    void undo() override;
    void redo() override;
    void dispose() override;

    std::string label() const;
    size_t memSize() const;

    Context* context() const { return m_ctx; }

  protected:
    virtual void onExecute();
    virtual void onUndo();
    virtual void onRedo();
    virtual void onFireNotifications();
    virtual std::string onLabel() const;
    virtual size_t onMemSize() const;

  private:
    Context* m_ctx;
#if _DEBUG
    enum class State { NotExecuted, Executed, Undone, Redone };
    State m_state;
#endif

    DISABLE_COPYING(Cmd);
  };

} // namespace app

#endif
