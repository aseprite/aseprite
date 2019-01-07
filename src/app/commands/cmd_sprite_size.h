// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COMMANDS_CMD_SPRITE_SIZE_H_INCLUDED
#define APP_COMMANDS_CMD_SPRITE_SIZE_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "doc/algorithm/resize_image.h"

#include <string>

namespace ui {
  class CheckBox;
  class Entry;
}

namespace app {

  class SpriteSizeCommand : public Command {
  public:
    SpriteSizeCommand();

    void setScale(double x, double y) {
      m_scaleX = x;
      m_scaleY = y;
    }

  protected:
    virtual void onLoadParams(const Params& params) override;
    virtual bool onEnabled(Context* context) override;
    virtual void onExecute(Context* context) override;

  private:
    bool m_useUI;
    int m_width;
    int m_height;
    double m_scaleX;
    double m_scaleY;
    doc::algorithm::ResizeMethod m_resizeMethod;
  };

} // namespace app

#endif
