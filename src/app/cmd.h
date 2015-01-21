/* Aseprite
 * Copyright (C) 2001-2015  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_CMD_H_INCLUDED
#define APP_CMD_H_INCLUDED
#pragma once

#include "base/disable_copying.h"
#include "doc/sprite_position.h"
#include "undo/undo_command.h"

#include <string>

namespace app {
  class Context;

  class Cmd : public undo::UndoCommand {
  public:
    Cmd();
    virtual ~Cmd();

    void execute(Context* ctx);
    void undo() override;
    void redo() override;
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
