// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/range_utils.h"

#include "app/context_access.h"
#include "app/doc.h"
#include "app/doc_range.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

#include <set>

namespace app {

using namespace doc;

// TODO the DocRange should be "iteratable" to replace this function
static CelList get_cels_templ(const Sprite* sprite,
                              DocRange range,
                              const bool onlyUniqueCels,
                              const bool onlyUnlockedCel)
{
  CelList cels;
  if (!range.convertToCels(sprite))
    return cels;

  std::set<ObjectId> visited;

  for (Layer* layer : range.selectedLayers()) {
    if (!layer ||
        !layer->isImage() ||
        (onlyUnlockedCel && !layer->isEditable()))
      continue;

    LayerImage* layerImage = static_cast<LayerImage*>(layer);
    for (frame_t frame : range.selectedFrames()) {
      Cel* cel = layerImage->cel(frame);
      if (!cel)
        continue;

      if (!onlyUniqueCels ||
          visited.find(cel->data()->id()) == visited.end()) {
        if (onlyUniqueCels)
          visited.insert(cel->data()->id());

        cels.push_back(cel);
      }
    }
  }
  return cels;
}

CelList get_cels(const doc::Sprite* sprite, const DocRange& range)
{
  return get_cels_templ(sprite, range, false, false);
}

CelList get_unique_cels(const Sprite* sprite, const DocRange& range)
{
  return get_cels_templ(sprite, range, true, false);
}

CelList get_unlocked_unique_cels(const Sprite* sprite, const DocRange& range)
{
  return get_cels_templ(sprite, range, true, true);
}

} // namespace app
