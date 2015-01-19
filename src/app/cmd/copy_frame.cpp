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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/copy_frame.h"

#include "app/cmd/add_frame.h"
#include "app/cmd/copy_cel.h"
#include "app/cmd/set_frame_duration.h"
#include "doc/sprite.h"
#include "doc/layer.h"

namespace app {
namespace cmd {

using namespace doc;

CopyFrame::CopyFrame(Sprite* sprite, frame_t fromFrame, frame_t newFrame)
  : WithSprite(sprite)
  , m_fromFrame(fromFrame)
  , m_newFrame(newFrame)
{
}

void CopyFrame::onExecute()
{
  Sprite* sprite = this->sprite();
  frame_t fromFrame = m_fromFrame;
  int msecs = sprite->frameDuration(fromFrame);

  executeAndAdd(new cmd::AddFrame(sprite, m_newFrame));
  executeAndAdd(new cmd::SetFrameDuration(sprite, m_newFrame, msecs));

  if (fromFrame >= m_newFrame)
    ++fromFrame;

  for (int i=0; i<sprite->countLayers(); ++i) {
    Layer* layer = sprite->layer(i);
    if (layer->isImage())  {
      executeAndAdd(new cmd::CopyCel(
          static_cast<LayerImage*>(layer), fromFrame,
          static_cast<LayerImage*>(layer), m_newFrame));
    }
  }
}

} // namespace cmd
} // namespace app
