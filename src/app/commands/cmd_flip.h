// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_FLIP_H_INCLUDED
#define APP_COMMANDS_CMD_FLIP_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "doc/algorithm/flip_type.h"

namespace app {

  class FlipCommand : public Command {
  public:
    FlipCommand();

    doc::algorithm::FlipType getFlipType() const { return m_flipType; }

  protected:
    void onLoadParams(const Params& params) override;
    bool onEnabled(Context* context) override;
    void onExecute(Context* context) override;
    std::string onGetFriendlyName() const override;

  private:
    bool m_flipMask;
    doc::algorithm::FlipType m_flipType;
  };

} // namespace app

#endif // APP_COMMANDS_CMD_FLIP_H_INCLUDED
