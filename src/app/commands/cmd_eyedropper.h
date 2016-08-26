// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_EYEDROPPER_H_INCLUDED
#define APP_COMMANDS_CMD_EYEDROPPER_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/commands/command.h"

namespace doc {
  class Site;
}

namespace app {

  class EyedropperCommand : public Command {
  public:
    EyedropperCommand();
    Command* clone() const override { return new EyedropperCommand(*this); }

    // Returns the color in the given sprite pos.
    void pickSample(const doc::Site& site,
                    const gfx::Point& pixelPos,
                    app::Color& color);

  protected:
    void onLoadParams(const Params& params) override;
    void onExecute(Context* context) override;

    // True means "pick background color", false the foreground color.
    bool m_background;
  };

} // namespace app

#endif // APP_COMMANDS_CMD_EYEDROPPER_H_INCLUDED
