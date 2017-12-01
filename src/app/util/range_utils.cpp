// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/range_utils.h"

#include "app/context_access.h"
#include "app/document.h"
#include "app/document_range.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <set>

namespace app {

using namespace doc;

// TODO the DocumentRange should be "iteratable" to replace this function
CelList get_unlocked_unique_cels(Sprite* sprite, const DocumentRange& inrange)
{
  DocumentRange range = inrange;
  CelList cels;
  if (!range.convertToCels(sprite))
    return cels;

  std::set<ObjectId> visited;

  for (Layer* layer : range.selectedLayers()) {
    if (!layer ||
        !layer->isImage() ||
        !layer->isEditable())
      continue;

    LayerImage* layerImage = static_cast<LayerImage*>(layer);
    for (frame_t frame : range.selectedFrames()) {
      Cel* cel = layerImage->cel(frame);
      if (!cel)
        continue;

      if (visited.find(cel->data()->id()) == visited.end()) {
        visited.insert(cel->data()->id());
        cels.push_back(cel);
      }
    }
  }
  return cels;
}

} // namespace app
