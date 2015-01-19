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

#ifndef APP_CMD_SET_PALETTE_COLORS_H_INCLUDED
#define APP_CMD_SET_PALETTE_COLORS_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_sprite.h"
#include "doc/color.h"
#include "doc/frame.h"

#include <vector>

namespace doc {
  class Palette;
  class Sprite;
}

namespace app {
namespace cmd {
  using namespace doc;

  class SetPalette : public Cmd
                   , public WithSprite {
  public:
    SetPalette(Sprite* sprite, frame_t frame, Palette* newPalette);

  protected:
    void onExecute() override;
    void onUndo() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        sizeof(doc::color_t) * (m_oldColors.size() +
                                m_newColors.size());
    }

  private:
    frame_t m_frame;
    int m_from, m_to;
    std::vector<color_t> m_oldColors;
    std::vector<color_t> m_newColors;
  };

} // namespace cmd
} // namespace app

#endif
