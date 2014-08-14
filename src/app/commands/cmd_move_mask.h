/* Aseprite
 * Copyright (C) 2001-2014  David Capello
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

#ifndef APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#define APP_COMMANDS_CMD_EXPORT_SPRITE_SHEET_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "base/override.h"

namespace app {

  class MoveMaskCommand : public Command {
  public:
    enum Target { Outline, Content };
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
    Command* clone() const OVERRIDE { return new MoveMaskCommand(*this); }

    Target getTarget() const { return m_target; }

  protected:
    void onLoadParams(Params* params);
    bool onEnabled(Context* context);
    void onExecute(Context* context);

  private:
    Target m_target;
    Direction m_direction;
    Units m_units;
    int m_quantity;
  };

} // namespace app

#endif
