// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#define APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "app/commands/move_thing.h"

namespace app {

  class MoveMaskCommand : public Command {
  public:
    enum Target { Boundaries, Content };

    MoveMaskCommand();

    Target getTarget() const { return m_target; }
    gfx::Point getDelta(Context* context) const;

  protected:
    bool onNeedsParams() const override { return true; }
    void onLoadParams(const Params& params) override;
    bool onEnabled(Context* context) override;
    void onExecute(Context* context) override;
    std::string onGetFriendlyName() const override;

  private:
    Target m_target;
    MoveThing m_moveThing;
    bool m_wrap;
  };

} // namespace app

#endif
