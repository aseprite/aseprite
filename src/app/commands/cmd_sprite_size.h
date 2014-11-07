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

#ifndef APP_COMMANDS_CMD_SPRITE_SIZE_H_INCLUDED
#define APP_COMMANDS_CMD_SPRITE_SIZE_H_INCLUDED
#pragma once

#include "app/commands/command.h"
#include "raster/algorithm/resize_image.h"

#include <string>

namespace ui {
  class CheckBox;
  class Entry;
}

namespace app {

  class SpriteSizeCommand : public Command {
  public:
    SpriteSizeCommand();
    Command* clone() const override;

    void setScale(double x, double y) {
      m_scaleX = x;
      m_scaleY = y;
    }

  protected:
    virtual void onLoadParams(Params* params) override;
    virtual bool onEnabled(Context* context) override;
    virtual void onExecute(Context* context) override;

  private:
    void onLockRatioClick();
    void onWidthPxChange();
    void onHeightPxChange();
    void onWidthPercChange();
    void onHeightPercChange();

    ui::CheckBox* m_lockRatio;
    ui::Entry* m_widthPx;
    ui::Entry* m_heightPx;
    ui::Entry* m_widthPerc;
    ui::Entry* m_heightPerc;

    int m_width;
    int m_height;
    double m_scaleX;
    double m_scaleY;
    raster::algorithm::ResizeMethod m_resizeMethod;
  };

} // namespace app

#endif
