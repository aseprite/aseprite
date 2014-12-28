// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel.h"
#include "doc/image.h"
#include "doc/images_collector.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/sprite.h"
#include "doc/stock.h"

namespace doc {

ImagesCollector::ImagesCollector(Layer* layer,
                                 frame_t frame,
                                 bool allFrames,
                                 bool forEdit)
  : m_allFrames(allFrames)
  , m_forEdit(forEdit)
{
  collectFromLayer(layer, frame);
}

void ImagesCollector::collectFromLayer(Layer* layer, frame_t frame)
{
  const Sprite* sprite = layer->sprite();

  if (!layer->isVisible())
    return;

  if (m_forEdit && !layer->isEditable())
    return;

  switch (layer->type()) {

    case ObjectType::LayerImage: {
      if (m_allFrames) {
        for (frame_t frame(0); frame<sprite->totalFrames(); ++frame) {
          Cel* cel = static_cast<LayerImage*>(layer)->getCel(frame);
          if (cel != NULL)
            collectImage(layer, cel);
        }
      }
      else {
        Cel* cel = static_cast<LayerImage*>(layer)->getCel(frame);
        if (cel != NULL)
          collectImage(layer, cel);
      }
      break;
    }

    case ObjectType::LayerFolder: {
      LayerIterator it = static_cast<LayerFolder*>(layer)->getLayerBegin();
      LayerIterator end = static_cast<LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it)
        collectFromLayer(*it, frame);

      break;
    }

  }
}

void ImagesCollector::collectImage(Layer* layer, Cel* cel)
{
  m_items.push_back(Item(layer, cel, cel->image()));
}

} // namespace doc
