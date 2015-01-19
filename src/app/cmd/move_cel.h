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

#ifndef APP_CMD_MOVE_CEL_H_INCLUDED
#define APP_CMD_MOVE_CEL_H_INCLUDED
#pragma once

#include "app/cmd/with_layer.h"
#include "app/cmd_sequence.h"
#include "doc/color.h"
#include "doc/frame.h"

namespace doc {
  class LayerImage;
}

namespace app {
namespace cmd {
  using namespace doc;

  class MoveCel : public CmdSequence {
  public:
    MoveCel(
      LayerImage* srcLayer, frame_t srcFrame,
      LayerImage* dstLayer, frame_t dstFrame);

  protected:
    void onExecute() override;
    void onFireNotifications() override;

  private:
    WithLayer m_srcLayer, m_dstLayer;
    frame_t m_srcFrame, m_dstFrame;
  };

} // namespace cmd
} // namespace app

#endif
