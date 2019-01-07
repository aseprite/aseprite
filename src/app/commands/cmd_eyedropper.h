// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_EYEDROPPER_H_INCLUDED
#define APP_COMMANDS_CMD_EYEDROPPER_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/commands/command.h"
#include "gfx/point.h"

namespace render {
  class Projection;
}

namespace app {
  class Editor;
  class Site;

  class EyedropperCommand : public Command {
  public:
    EyedropperCommand();

    // Returns the color in the given sprite pos.
    void pickSample(const Site& site,
                    const gfx::PointF& pixelPos,
                    const render::Projection& proj,
                    app::Color& color);

    void executeOnMousePos(Context* context,
                           Editor* editor,
                           const gfx::Point& mousePos,
                           const bool foreground);

  protected:
    void onLoadParams(const Params& params) override;
    void onExecute(Context* context) override;

    // True means "pick background color", false the foreground color.
    bool m_background;
  };

} // namespace app

#endif // APP_COMMANDS_CMD_EYEDROPPER_H_INCLUDED
