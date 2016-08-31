// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
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
          if (Cel* cel = layer->cel(frame))
            collectImage(layer, cel);
        }
      }
      else {
        if (Cel* cel = layer->cel(frame))
          collectImage(layer, cel);
      }
      break;
    }

    case ObjectType::LayerGroup: {
      for (Layer* child : static_cast<LayerGroup*>(layer)->layers())
        collectFromLayer(child, frame);
      break;
    }

  }
}

void ImagesCollector::collectImage(Layer* layer, Cel* cel)
{
  m_items.push_back(Item(layer, cel, cel->image()));
}

} // namespace doc
