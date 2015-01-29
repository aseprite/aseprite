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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/context_access.h"
#include "app/document.h"
#include "app/document_range.h"
#include "raster/layer.h"
#include "raster/sprite.h"

namespace app {

using namespace raster;

// TODO the DocumentRange should be "iteratable" to replace this function
CelList get_cels_in_range(Sprite* sprite, const DocumentRange& inrange)
{
  DocumentRange range = inrange;
  CelList cels;

  switch (range.type()) {
    case DocumentRange::kNone:
      return cels;
    case DocumentRange::kCels:
      break;
    case DocumentRange::kFrames:
      range.startRange(LayerIndex(0), inrange.frameBegin(), DocumentRange::kCels);
      range.endRange(LayerIndex(sprite->countLayers()-1), inrange.frameEnd());
      break;
    case DocumentRange::kLayers:
      range.startRange(inrange.layerBegin(), FrameNumber(0), DocumentRange::kCels);
      range.endRange(inrange.layerEnd(), sprite->lastFrame());
      break;
  }

  for (LayerIndex layerIdx = range.layerBegin(); layerIdx <= range.layerEnd(); ++layerIdx) {
    Layer* layer = sprite->indexToLayer(layerIdx);
    if (!layer || !layer->isImage())
      continue;

    LayerImage* layerImage = static_cast<LayerImage*>(layer);
    for (FrameNumber frame = range.frameEnd(),
           begin = range.frameBegin().previous();
         frame != begin;
         frame = frame.previous()) {
      Cel* cel = layerImage->getCel(frame);
      if (cel)
        cels.push_back(cel);
    }
  }
  return cels;
}

} // namespace app
