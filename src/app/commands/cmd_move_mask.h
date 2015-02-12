// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#define APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#pragma once

#include "app/commands/command.h"

namespace app {

  class MoveMaskCommand : public Command {
  public:
    enum Target { Boundaries, Content };
    enum Direction { Left, Up, Right, Down, }; // TODO join this enum with scroll command
    enum Units {
      Pixel,
      TileWidth,
      TileHeight,
      ZoomedPixel,
      ZoomedTileWidth,
      ZoomedTileHeight,
      ViewportWidth,
      ViewportHeight
    };

    MoveMaskCommand();
    Command* clone() const override { return new MoveMaskCommand(*this); }

    Target getTarget() const { return m_target; }

  protected:
    void onLoadParams(Params* params);
    bool onEnabled(Context* context);
    void onExecute(Context* context);
    std::string onGetFriendlyName() const;

  private:
    Target m_target;
    Direction m_direction;
    Units m_units;
    int m_quantity;
  };

} // namespace app

#endif
