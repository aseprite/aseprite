/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#include "base/unique_ptr.h"
#include "doc/cel.h"
#include "doc/frame.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "doc/stock.h"
#include "gfx/rect.h"
#include "render/render.h"

namespace app {

using namespace doc;

static bool has_cels(const Layer* layer, frame_t frame);

LayerImage* create_flatten_layer_copy(Sprite* dstSprite, const Layer* srcLayer,
                                      const gfx::Rect& bounds,
                                      frame_t frmin, frame_t frmax)
{
  base::UniquePtr<LayerImage> flatLayer(new LayerImage(dstSprite));
  render::Render render;

  for (frame_t frame=frmin; frame<=frmax; ++frame) {
    // Does this frame have cels to render?
    if (has_cels(srcLayer, frame)) {
      // Create a new image to render each frame.
      base::UniquePtr<Image> imageWrap(Image::create(flatLayer->sprite()->pixelFormat(), bounds.w, bounds.h));

      // Add the image into the sprite's stock too.
      int imageIndex = flatLayer->sprite()->stock()->addImage(imageWrap);
      Image* image = imageWrap.release();

      // Create the new cel for the output layer.
      base::UniquePtr<Cel> cel(new Cel(frame, imageIndex));
      cel->setPosition(bounds.x, bounds.y);

      // Render this frame.
      render.renderLayer(image, srcLayer, frame,
        gfx::Clip(0, 0, bounds));

      // Add the cel (and release the base::UniquePtr).
      flatLayer->addCel(cel);
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

    case ObjectType::LayerFolder: {
      LayerConstIterator it = static_cast<const LayerFolder*>(layer)->getLayerBegin();
      LayerConstIterator end = static_cast<const LayerFolder*>(layer)->getLayerEnd();

      for (; it != end; ++it) {
        if (has_cels(*it, frame))
          return true;
      }
      break;
    }

  }

  return false;
}
  
} // namespace app
