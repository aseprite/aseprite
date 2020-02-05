// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/layer_boundaries.h"

#include "app/cmd/set_mask.h"
#include "app/console.h"
#include "app/context_access.h"
#include "app/modules/gui.h"
#include "app/tx.h"
#include "app/ui_context.h"
#include "doc/cel.h"
#include "doc/document.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/mask.h"

using namespace doc;

namespace app {

void select_layer_boundaries(Layer* layer,
                             const frame_t frame,
                             const SelectLayerBoundariesOp op)
{
  Mask newMask;

  const Cel* cel = layer->cel(frame);
  if (cel) {
    const Image* image = cel->image();
    if (image) {
      newMask.replace(cel->bounds());
      newMask.freeze();
      {
        LockImageBits<BitmapTraits> maskBits(newMask.bitmap());
        auto maskIt = maskBits.begin();
        auto maskEnd = maskBits.end();

        switch (image->pixelFormat()) {

          case IMAGE_RGB: {
            LockImageBits<RgbTraits> rgbBits(image);
            auto rgbIt = rgbBits.begin();
#if _DEBUG
            auto rgbEnd = rgbBits.end();
#endif
            for (; maskIt != maskEnd; ++maskIt, ++rgbIt) {
              ASSERT(rgbIt != rgbEnd);
              color_t c = *rgbIt;
              *maskIt = (rgba_geta(c) >= 128); // TODO configurable threshold
            }
            break;
          }

          case IMAGE_GRAYSCALE: {
            LockImageBits<GrayscaleTraits> grayBits(image);
            auto grayIt = grayBits.begin();
#if _DEBUG
            auto grayEnd = grayBits.end();
#endif
            for (; maskIt != maskEnd; ++maskIt, ++grayIt) {
              ASSERT(grayIt != grayEnd);
              color_t c = *grayIt;
              *maskIt = (graya_geta(c) >= 128); // TODO configurable threshold
            }
            break;
          }

          case IMAGE_INDEXED: {
            const doc::color_t maskColor = image->maskColor();
            LockImageBits<IndexedTraits> idxBits(image);
            auto idxIt = idxBits.begin();
#if _DEBUG
            auto idxEnd = idxBits.end();
#endif
            for (; maskIt != maskEnd; ++maskIt, ++idxIt) {
              ASSERT(idxIt != idxEnd);
              color_t c = *idxIt;
              *maskIt = (c != maskColor);
            }
            break;
          }

        }
      }
      newMask.unfreeze();
    }
  }

  try {
    ContextWriter writer(UIContext::instance());
    Doc* doc = writer.document();
    ASSERT(doc == layer->sprite()->document());

    if (doc->isMaskVisible()) {
      switch (op) {
        case SelectLayerBoundariesOp::REPLACE:
          // newMask is the new mask
          break;
        case SelectLayerBoundariesOp::ADD:
          newMask.add(*doc->mask());
          break;
        case SelectLayerBoundariesOp::SUBTRACT: {
          Mask oldMask(*doc->mask());
          oldMask.subtract(newMask);
          newMask.copyFrom(&oldMask); // TODO use something like std::swap()
          break;
        }
        case SelectLayerBoundariesOp::INTERSECT:
          newMask.intersect(*doc->mask());
          break;
      }
    }

    Tx tx(writer.context(), "Select Layer Boundaries", DoesntModifyDocument);
    tx(new cmd::SetMask(doc, &newMask));
    tx.commit();

    update_screen_for_document(doc);
  }
  catch (base::Exception& e) {
    Console::showException(e);
  }
}

} // namespace app
