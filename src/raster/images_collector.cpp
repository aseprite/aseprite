/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "config.h"

#include "raster/cel.h"
#include "raster/image.h"
#include "raster/images_collector.h"
#include "raster/layer.h"
#include "raster/mask.h"
#include "raster/sprite.h"
#include "raster/stock.h"

ImagesCollector::ImagesCollector(const Sprite* sprite, bool allLayers, bool allFrames, bool forWrite)
  : m_allFrames(allFrames)
  , m_forWrite(forWrite)
{
  Layer* layer = allLayers ? sprite->getFolder():
                             sprite->getCurrentLayer();

  collectFromLayer(layer);
}

void ImagesCollector::collectFromLayer(Layer* layer)
{
  const Sprite* sprite = layer->getSprite();

  if (!layer->is_readable())
    return;

  if (m_forWrite && !layer->is_writable())
    return;

  switch (layer->getType()) {

    case GFXOBJ_LAYER_IMAGE: {
      if (m_allFrames) {
        for (FrameNumber frame(0); frame<sprite->getTotalFrames(); ++frame) {
          Cel* cel = static_cast<LayerImage*>(layer)->getCel(frame);
          if (cel != NULL)
            collectImage(layer, cel);
        }
      }
      else {
        FrameNumber frame = sprite->getCurrentFrame();
        Cel* cel = static_cast<LayerImage*>(layer)->getCel(frame);
        if (cel != NULL)
          collectImage(layer, cel);
      }
      break;
    }

    case GFXOBJ_LAYER_FOLDER: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->get_layer_begin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->get_layer_end();

      for (; it != end; ++it)
        collectFromLayer(*it);

      break;
    }

  }
}

void ImagesCollector::collectImage(Layer* layer, Cel* cel)
{
  Image* image = layer->getSprite()->getStock()->getImage(cel->getImage());

  m_items.push_back(Item(layer, cel, image));
}
