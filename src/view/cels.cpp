// Aseprite View Library
// Copyright (C) 2020-2023  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "view/cels.h"

#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "view/range.h"

namespace view {

using namespace doc;

enum class Target {
  kAllCels,
  kUniqueCels,
  kUniqueCanMoveCels,
  kUniqueCanEditPixelsCels,
};

// TODO the view::Range should be "iteratable" to replace this function
//      or we can wait to C++20 coroutines and co_yield
static CelList get_cels_templ(const Sprite* sprite,
                              Range range,
                              const Target target)
{
  CelList cels;
  if (!range.convertToCels(sprite))
    return cels;

  // Used to visit linked cels just once.
  std::set<ObjectId> visited;

  for (Layer* layer : range.selectedLayers()) {
    if (!layer ||
        !layer->isImage() ||
        (target == Target::kUniqueCanMoveCels && !layer->isEditableHierarchy()) ||
        (target == Target::kUniqueCanEditPixelsCels && !layer->canEditPixels())) {
      continue;
    }

    LayerImage* layerImage = static_cast<LayerImage*>(layer);
    for (frame_t frame : range.selectedFrames()) {
      Cel* cel = layerImage->cel(frame);
      if (!cel)
        continue;

      if (target == Target::kAllCels ||
          visited.find(cel->data()->id()) == visited.end()) {
        // Only unique cels (avoid visited cels)
        if (target != Target::kAllCels)
          visited.insert(cel->data()->id());

        cels.push_back(cel);
      }
    }
  }
  return cels;
}

CelList get_cels(const Sprite* sprite, const Range& range)
{
  return get_cels_templ(sprite, range, Target::kAllCels);
}

CelList get_unique_cels(const Sprite* sprite, const Range& range)
{
  return get_cels_templ(sprite, range, Target::kUniqueCels);
}

CelList get_unique_cels_to_move_cel(const Sprite* sprite, const Range& range)
{
  return get_cels_templ(sprite, range, Target::kUniqueCanMoveCels);
}

CelList get_unique_cels_to_edit_pixels(const Sprite* sprite, const Range& range)
{
  return get_cels_templ(sprite, range, Target::kUniqueCanEditPixelsCels);
}

} // namespace view
