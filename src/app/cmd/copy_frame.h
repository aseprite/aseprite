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

#ifndef APP_CMD_COPY_FRAME_H_INCLUDED
#define APP_CMD_COPY_FRAME_H_INCLUDED
#pragma once

#include "app/cmd_sequence.h"
#include "app/cmd/with_sprite.h"
#include "doc/frame.h"

namespace app {
namespace cmd {
  using namespace doc;

  class CopyFrame : public CmdSequence
                  , public WithSprite {
  public:
    CopyFrame(Sprite* sprite, frame_t fromFrame, frame_t newFrame);

  protected:
    void onExecute() override;
    size_t onMemSize() const override {
      return sizeof(*this) +
        CmdSequence::onMemSize() - sizeof(CmdSequence);
    }

  private:
    frame_t m_fromFrame;
    frame_t m_newFrame;
    bool m_firstTime;
  };

} // namespace cmd
} // namespace app

#endif
