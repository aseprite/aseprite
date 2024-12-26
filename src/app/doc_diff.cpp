// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/doc_diff.h"

#include "app/doc.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "doc/tag.h"
#include "doc/tileset.h"
#include "doc/tilesets.h"
#include "doc/user_data.h"

#ifdef _DEBUG
namespace doc {

static std::ostream& operator<<(std::ostream& os, const UserData& userData)
{
  return os << "(" << userData.text() << ", " << userData.color() << ")";
}

} // namespace doc
#endif

namespace app {

#ifdef _DEBUG
  #define TRACEDIFF(a, b)                                                                          \
    if (a != b) {                                                                                  \
      TRACEARGS(#a " != " #b, a, b);                                                               \
    }
#else
  #define TRACEDIFF(a, b)
#endif

DocDiff compare_docs(const Doc* a, const Doc* b)
{
  DocDiff diff;

  // Don't compare filenames
  // if (a->filename() != b->filename())...

  // Compare sprite specs
  if (a->sprite()->width() != b->sprite()->width() ||
      a->sprite()->height() != b->sprite()->height() ||
      a->sprite()->pixelFormat() != b->sprite()->pixelFormat() ||
      a->sprite()->userData() != b->sprite()->userData()) {
    diff.anything = diff.canvas = true;

    TRACEDIFF(a->sprite()->size(), b->sprite()->size());
    TRACEDIFF(a->sprite()->pixelFormat(), b->sprite()->pixelFormat());
    TRACEDIFF(a->sprite()->userData(), b->sprite()->userData());
  }

  // Frames layers
  if (a->sprite()->totalFrames() != b->sprite()->totalFrames()) {
    diff.anything = diff.totalFrames = true;

    TRACEDIFF(a->sprite()->totalFrames(), b->sprite()->totalFrames());
  }
  else {
    for (frame_t f = 0; f < a->sprite()->totalFrames(); ++f) {
      if (a->sprite()->frameDuration(f) != b->sprite()->frameDuration(f)) {
        diff.anything = diff.frameDuration = true;

        TRACEDIFF(a->sprite()->frameDuration(f), b->sprite()->frameDuration(f));
        break;
      }
    }
  }

  // Tags
  if (a->sprite()->tags().size() != b->sprite()->tags().size()) {
    diff.anything = diff.tags = true;

    TRACEDIFF(a->sprite()->tags().size(), b->sprite()->tags().size());
  }
  else {
    auto aIt = a->sprite()->tags().begin(), aEnd = a->sprite()->tags().end();
    auto bIt = b->sprite()->tags().begin(), bEnd = b->sprite()->tags().end();
    for (; aIt != aEnd && bIt != bEnd; ++aIt, ++bIt) {
      const Tag* aTag = *aIt;
      const Tag* bTag = *bIt;
      if (aTag->fromFrame() != bTag->fromFrame() || aTag->toFrame() != bTag->toFrame() ||
          aTag->name() != bTag->name() || aTag->color() != bTag->color() ||
          aTag->aniDir() != bTag->aniDir() || aTag->repeat() != bTag->repeat() ||
          aTag->userData() != bTag->userData()) {
        diff.anything = diff.tags = true;

        TRACEDIFF(aTag->fromFrame(), bTag->fromFrame());
        TRACEDIFF(aTag->toFrame(), bTag->toFrame());
        TRACEDIFF(aTag->name(), bTag->name());
        TRACEDIFF(aTag->color(), bTag->color());
        TRACEDIFF((int)aTag->aniDir(), (int)bTag->aniDir());
        TRACEDIFF(aTag->repeat(), bTag->repeat());
        TRACEDIFF(aTag->userData(), bTag->userData());
      }
    }
  }

  // Palettes
  if (a->sprite()->getPalettes().size() != b->sprite()->getPalettes().size()) {
    const PalettesList& aPals = a->sprite()->getPalettes();
    const PalettesList& bPals = b->sprite()->getPalettes();
    auto aIt = aPals.begin(), aEnd = aPals.end();
    auto bIt = bPals.begin(), bEnd = bPals.end();

    for (; aIt != aEnd && bIt != bEnd; ++aIt, ++bIt) {
      const Palette* aPal = *aIt;
      const Palette* bPal = *bIt;

      if (aPal->countDiff(bPal, nullptr, nullptr)) {
        diff.anything = diff.palettes = true;
        break;
      }
    }
  }

  // Compare tilesets
  const tile_index aTilesetSize = (a->sprite()->hasTilesets() ? a->sprite()->tilesets()->size() :
                                                                0);
  const tile_index bTilesetSize = (b->sprite()->hasTilesets() ? b->sprite()->tilesets()->size() :
                                                                0);
  if (aTilesetSize != bTilesetSize) {
    diff.anything = diff.tilesets = true;
  }
  else {
    for (int i = 0; i < aTilesetSize; ++i) {
      Tileset* aTileset = a->sprite()->tilesets()->get(i);
      Tileset* bTileset = b->sprite()->tilesets()->get(i);

      if (aTileset == nullptr && bTileset == nullptr) {
        // Both tilesets nullptr, it's ok
        continue;
      }
      else if (aTileset == nullptr || bTileset == nullptr) {
        diff.anything = diff.tilesets = true;
        break;
      }
      else if (aTileset->grid().tileSize() != bTileset->grid().tileSize() ||
               aTileset->size() != bTileset->size() ||
               aTileset->userData() != bTileset->userData()) {
        diff.anything = diff.tilesets = true;

        TRACEDIFF(aTileset->grid().tileSize(), bTileset->grid().tileSize());
        TRACEDIFF(aTileset->size(), bTileset->size());
        TRACEDIFF(aTileset->userData(), bTileset->userData());
        break;
      }
      else {
        for (tile_index ti = 0; ti < aTileset->size(); ++ti) {
          if (!is_same_image(aTileset->get(ti).get(), bTileset->get(ti).get())) {
            diff.anything = diff.tilesets = true;
            goto done;
          }
        }
      }
    }
  done:;
  }

  // Compare layers
  if (a->sprite()->allLayersCount() != b->sprite()->allLayersCount()) {
    diff.anything = diff.layers = true;
  }
  else {
    LayerList aLayers = a->sprite()->allLayers();
    LayerList bLayers = b->sprite()->allLayers();
    auto aIt = aLayers.begin(), aEnd = aLayers.end();
    auto bIt = bLayers.begin(), bEnd = bLayers.end();

    for (; aIt != aEnd && bIt != bEnd; ++aIt, ++bIt) {
      const Layer* aLay = *aIt;
      const Layer* bLay = *bIt;

      if (aLay->type() != bLay->type() || aLay->name() != bLay->name() ||
          aLay->userData() != bLay->userData() ||
          ((int(aLay->flags()) & int(LayerFlags::StructuralFlagsMask)) !=
           (int(bLay->flags()) & int(LayerFlags::StructuralFlagsMask))) ||
          (aLay->isImage() && bLay->isImage() &&
           (((const LayerImage*)aLay)->opacity() != ((const LayerImage*)bLay)->opacity())) ||
          (aLay->isTilemap() && bLay->isTilemap() &&
           (((const LayerTilemap*)aLay)->tilesetIndex() !=
            ((const LayerTilemap*)bLay)->tilesetIndex()))) {
        diff.anything = diff.layers = true;
        break;
      }

      if (!diff.totalFrames) {
        for (frame_t f = 0; f < a->sprite()->totalFrames(); ++f) {
          const Cel* aCel = aLay->cel(f);
          const Cel* bCel = bLay->cel(f);

          if ((!aCel && bCel) || (aCel && !bCel)) {
            diff.anything = diff.cels = true;
          }
          else if (aCel && bCel) {
            if (aCel->frame() != bCel->frame() || aCel->bounds() != bCel->bounds() ||
                aCel->opacity() != bCel->opacity() ||
                aCel->data()->userData() != bCel->data()->userData()) {
              diff.anything = diff.cels = true;

              TRACEDIFF(aCel->frame(), bCel->frame());
              TRACEDIFF(aCel->bounds(), bCel->bounds());
              TRACEDIFF(aCel->opacity(), bCel->opacity());
              TRACEDIFF(aCel->data()->userData(), bCel->data()->userData());
            }
            if (aCel->image() && bCel->image()) {
              if (aCel->image()->bounds() != bCel->image()->bounds() ||
                  !is_same_image(aCel->image(), bCel->image()))
                diff.anything = diff.images = true;
            }
            // In case one is nullptr and the other not
            else if (aCel->image() != bCel->image())
              diff.anything = diff.images = true;
          }
        }
      }
    }
  }

  // Compare color spaces
  if (!a->sprite()->colorSpace()->nearlyEqual(*b->sprite()->colorSpace())) {
    diff.anything = diff.colorProfiles = true;
  }

  // Compare grid bounds
  if (a->sprite()->gridBounds() != b->sprite()->gridBounds()) {
    diff.anything = diff.gridBounds = true;
  }

  return diff;
}

} // namespace app
