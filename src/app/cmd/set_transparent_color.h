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

#ifndef APP_CMD_SET_TRANSPARENT_COLOR_H_INCLUDED
#define APP_CMD_SET_TRANSPARENT_COLOR_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/color.h"

namespace app {
namespace cmd {
  using namespace doc;

  class SetTransparentColor : public Cmd
                            , public WithSprite {
  public:
    SetTransparentColor(Sprite* sprite, color_t newMask);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onFireNotifications();
    size_t onMemSize() const override {
      return sizeof(*this);
    }

  private:
    color_t m_oldMaskColor;
    color_t m_newMaskColor;
  };

} // namespace cmd
} // namespace app

#endif
