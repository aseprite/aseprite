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

#ifndef APP_CMD_ADD_PALETTE_H_INCLUDED
#define APP_CMD_ADD_PALETTE_H_INCLUDED
#pragma once

#include "app/cmd.h"
#include "app/cmd/with_palette.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

#include <sstream>

namespace doc {
  class Sprite;
}

namespace app {
namespace cmd {
  using namespace doc;

  class AddPalette : public Cmd
                   , public WithSprite
                   , public WithPalette {
  public:
    AddPalette(Sprite* sprite, Palette* pal);

  protected:
    void onExecute() override;
    void onUndo() override;
    void onRedo() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        const_cast<std::stringstream*>(&m_stream)->tellp();
    }

  private:
    frame_t m_frame;
    std::stringstream m_stream;
  };

} // namespace cmd
} // namespace app

#endif  // CMD_ADD_PALETTE_H_INCLUDED
