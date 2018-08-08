// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/cel.h"
#include "doc/frame.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "gfx/rect.h"
#include "render/render.h"

#include <memory>

namespace app {

using namespace doc;

static bool has_cels(const Layer* layer, frame_t frame);

LayerImage* create_flatten_layer_copy(Sprite* dstSprite, const Layer* srcLayer,
                                      const gfx::Rect& bounds,
                                      frame_t frmin, frame_t frmax)
{
  std::unique_ptr<LayerImage> flatLayer(new LayerImage(dstSprite));
  render::Render render;

  for (frame_t frame=frmin; frame<=frmax; ++frame) {
    // Does this frame have cels to render?
    if (has_cels(srcLayer, frame)) {
      // Create a new image to render each frame.
      ImageRef image(Image::create(flatLayer->sprite()->pixelFormat(), bounds.w, bounds.h));

      // Create the new cel for the output layer.
      std::unique_ptr<Cel> cel(new Cel(frame, image));
      cel->setPosition(bounds.x, bounds.y);

      // Render this frame.
      render.renderLayer(image.get(), srcLayer, frame,
        gfx::Clip(0, 0, bounds));

      // Add the cel (and release the std::unique_ptr).
      flatLayer->addCel(cel.get());
      cel.release();
    }
  }

  return flatLayer.release();
}

// Returns true if the "layer" or its children have any cel to render
// in the given "frame".
static bool has_cels(const Layer* layer, frame_t frame)
{
  if (!layer->isVisible())
    return false;

  switch (layer->type()) {

    case ObjectType::LayerImage:
      return (layer->cel(frame) ? true: false);

    case ObjectType::LayerGroup: {
      for (const Layer* child : static_cast<const LayerGroup*>(layer)->layers()) {
        if (has_cels(child, frame))
          return true;
      }
      break;
    }

  }

  return false;
}

} // namespace app
