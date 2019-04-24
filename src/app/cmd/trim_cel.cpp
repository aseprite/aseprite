// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/trim_cel.h"

#include "app/cmd/crop_cel.h"
#include "app/cmd/remove_cel.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace cmd {

using namespace doc;

TrimCel::TrimCel(Cel* cel)
{
  gfx::Rect newBounds;
  if (algorithm::shrink_bounds(cel->image(), newBounds,
                               cel->image()->maskColor())) {
    newBounds.offset(cel->position());
    if (cel->bounds() != newBounds) {
      add(new cmd::CropCel(cel, newBounds));
    }
  }
  else {
    // Delete the given "cel" and all its links.
    Sprite* sprite = cel->sprite();
    Layer* layer = cel->layer();
    CelData* celData = cel->dataRef().get();

    for (frame_t fr=sprite->totalFrames()-1; fr>=0; --fr) {
      Cel* c = layer->cel(fr);
      if (c && c->dataRef().get() == celData)
        add(new cmd::RemoveCel(c));
    }
  }
}

} // namespace cmd
} // namespace app
