// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/util/cel_ops.h"

#include "app/cmd/add_tile.h"
#include "app/cmd/clear_cel.h"
#include "app/cmd/clear_mask.h"
#include "app/cmd/copy_region.h"
#include "app/cmd/remap_tilemaps.h"
#include "app/cmd/remap_tileset.h"
#include "app/cmd/remove_tile.h"
#include "app/cmd/replace_image.h"
#include "app/cmd/set_cel_position.h"
#include "app/cmd_sequence.h"
#include "app/doc.h"
#include "doc/algorithm/fill_selection.h"
#include "doc/algorithm/resize_image.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/grid.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "doc/tileset.h"
#include "doc/tilesets.h"
#include "gfx/region.h"
#include "render/dithering.h"
#include "render/ordered_dither.h"
#include "render/quantization.h"
#include "render/render.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#define OPS_TRACE(...) // TRACE(__VA_ARGS__)

namespace app {

using namespace doc;

namespace {

template<typename ImageTraits>
void mask_image_templ(Image* image, const Image* bitmap)
{
  LockImageBits<ImageTraits> bits1(image);
  const LockImageBits<BitmapTraits> bits2(bitmap);
  typename LockImageBits<ImageTraits>::iterator it1, end1;
  LockImageBits<BitmapTraits>::const_iterator it2, end2;
  for (it1 = bits1.begin(), end1 = bits1.end(),
       it2 = bits2.begin(), end2 = bits2.end();
       it1 != end1 && it2 != end2; ++it1, ++it2) {
    if (!*it2)
      *it1 = image->maskColor();
  }
  ASSERT(it1 == end1);
  ASSERT(it2 == end2);
}

void mask_image(Image* image, Image* bitmap)
{
  ASSERT(image->bounds() == bitmap->bounds());
  switch (image->pixelFormat()) {
    case IMAGE_RGB:       return mask_image_templ<RgbTraits>(image, bitmap);
    case IMAGE_GRAYSCALE: return mask_image_templ<GrayscaleTraits>(image, bitmap);
    case IMAGE_INDEXED:   return mask_image_templ<IndexedTraits>(image, bitmap);
  }
}

template<typename ImageTraits>
void create_region_with_differences_templ(const Image* a,
                                          const Image* b,
                                          const gfx::Rect& bounds,
                                          gfx::Region& output)
{
  for (int y=bounds.y; y<bounds.y2(); ++y) {
    for (int x=bounds.x; x<bounds.x2(); ++x) {
      if (get_pixel_fast<ImageTraits>(a, x, y) !=
          get_pixel_fast<ImageTraits>(b, x, y)) {
        output.createUnion(output, gfx::Region(gfx::Rect(x, y, 1, 1)));
      }
    }
  }
}

// TODO merge this with Sprite::getTilemapsByTileset()
template<typename UnaryFunction>
void for_each_tile_using_tileset(Tileset* tileset, UnaryFunction f)
{
  for (Cel* cel : tileset->sprite()->uniqueCels()) {
    if (!cel->layer()->isTilemap() ||
        static_cast<LayerTilemap*>(cel->layer())->tileset() != tileset)
      continue;

    Image* tilemapImage = cel->image();
    for_each_pixel<TilemapTraits>(tilemapImage, f);
  }
}

struct Mod {
  tile_index tileIndex;
  ImageRef tileDstImage;
  ImageRef tileImage;
  gfx::Region tileRgn;
};

} // anonymous namespace

void create_region_with_differences(const Image* a,
                                    const Image* b,
                                    const gfx::Rect& bounds,
                                    gfx::Region& output)
{
  ASSERT(a->pixelFormat() == b->pixelFormat());
  switch (a->pixelFormat()) {
    case IMAGE_RGB: create_region_with_differences_templ<RgbTraits>(a, b, bounds, output); break;
    case IMAGE_GRAYSCALE: create_region_with_differences_templ<GrayscaleTraits>(a, b, bounds, output); break;
    case IMAGE_INDEXED: create_region_with_differences_templ<IndexedTraits>(a, b, bounds, output); break;
  }
}

static void remove_unused_tiles_from_tileset(
  CmdSequence* cmds,
  doc::Tileset* tileset,
  std::vector<size_t>& tilesHistogram,
  const std::vector<bool>& modifiedTileIndexes);

doc::ImageRef crop_cel_image(
  const doc::Cel* cel,
  const color_t bgcolor)
{
  doc::Sprite* sprite = cel->sprite();

  if (cel->layer()->isTilemap()) {
    doc::ImageRef dstImage(doc::Image::create(sprite->spec()));

    render::Render().renderCel(
      dstImage.get(),
      cel,
      sprite,
      cel->image(),
      cel->layer(),
      sprite->palette(cel->frame()),
      dstImage->bounds(),
      gfx::Clip(cel->position(), dstImage->bounds()),
      255, BlendMode::NORMAL);

    return dstImage;
  }
  else {
    return doc::ImageRef(
      doc::crop_image(
        cel->image(),
        gfx::Rect(sprite->bounds()).offset(-cel->position()),
        bgcolor));
  }
}

Cel* create_cel_copy(CmdSequence* cmds,
                     const Cel* srcCel,
                     const Sprite* dstSprite,
                     Layer* dstLayer,
                     const frame_t dstFrame)
{
  const Image* srcImage = srcCel->image();
  doc::PixelFormat dstPixelFormat =
    (dstLayer->isTilemap() ? IMAGE_TILEMAP:
                             dstSprite->pixelFormat());
  gfx::Size dstSize(srcImage->width(),
                    srcImage->height());

  // From Tilemap -> Image
  if (srcCel->layer()->isTilemap() && !dstLayer->isTilemap()) {
    auto layerTilemap = static_cast<doc::LayerTilemap*>(srcCel->layer());
    dstSize = layerTilemap->tileset()->grid().tilemapSizeToCanvas(dstSize);
  }
  // From Image or Tilemap -> Tilemap
  else if (dstLayer->isTilemap()) {
    auto dstLayerTilemap = static_cast<doc::LayerTilemap*>(dstLayer);

    // Tilemap -> Tilemap
    Grid grid;
    if (srcCel->layer()->isTilemap()) {
      grid = dstLayerTilemap->tileset()->grid();
      if (srcCel->layer()->isTilemap())
        grid.origin(srcCel->position());
    }
    // Image -> Tilemap
    else {
      auto gridBounds = dstLayerTilemap->sprite()->gridBounds();
      grid.origin(gridBounds.origin());
      grid.tileSize(gridBounds.size());
    }

    const gfx::Rect tilemapBounds = grid.canvasToTile(srcCel->bounds());
    dstSize = tilemapBounds.size();
  }

  // New cel
  auto dstCel = std::make_unique<Cel>(
    dstFrame, ImageRef(Image::create(dstPixelFormat, dstSize.w, dstSize.h)));

  dstCel->setOpacity(srcCel->opacity());
  dstCel->setZIndex(srcCel->zIndex());
  dstCel->data()->setUserData(srcCel->data()->userData());

  // Special case were we copy from a tilemap...
  if (srcCel->layer()->isTilemap()) {
    if (dstLayer->isTilemap()) {
      // Tilemap -> Tilemap (with same tileset)
      // Best case, copy a cel in the same layer (we have the same
      // tileset available, so we just copy the tilemap as it is).
      if (srcCel->layer() == dstLayer) {
        dstCel->image()->copy(srcImage, gfx::Clip(0, 0, srcImage->bounds()));
      }
      // Tilemap -> Tilemap (with different tilesets)
      else {
        doc::ImageSpec spec = dstSprite->spec();
        spec.setSize(srcCel->bounds().size());
        doc::ImageRef tmpImage(doc::Image::create(spec));
        render::Render().renderCel(
          tmpImage.get(),
          srcCel,
          dstSprite,
          srcImage,
          srcCel->layer(),
          dstSprite->palette(dstCel->frame()),
          gfx::Rect(gfx::Point(0, 0), srcCel->bounds().size()),
          gfx::Clip(0, 0, tmpImage->bounds()),
          255, BlendMode::NORMAL);

        doc::ImageRef tilemap = dstCel->imageRef();

        draw_image_into_new_tilemap_cel(
          cmds, static_cast<doc::LayerTilemap*>(dstLayer), dstCel.get(),
          tmpImage.get(),
          srcCel->bounds().origin(),
          srcCel->bounds().origin(),
          srcCel->bounds(),
          tilemap);
      }
      dstCel->setPosition(srcCel->position());
    }
    // Tilemap -> Image (so we convert the tilemap to a regular image)
    else {
      render::Render().renderCel(
        dstCel->image(),
        srcCel,
        dstSprite,
        srcImage,
        srcCel->layer(),
        dstSprite->palette(dstCel->frame()),
        gfx::Rect(gfx::Point(0, 0), srcCel->bounds().size()),
        gfx::Clip(0, 0, dstCel->image()->bounds()),
        255, BlendMode::NORMAL);

      // Shrink image
      if (dstLayer->isTransparent()) {
        auto bg = dstCel->image()->maskColor();
        gfx::Rect bounds;
        if (algorithm::shrink_bounds(dstCel->image(), bg, dstLayer, bounds)) {
          ImageRef trimmed(doc::crop_image(dstCel->image(), bounds, bg));
          dstCel->data()->setImage(trimmed, dstLayer);
          dstCel->setPosition(srcCel->position() + bounds.origin());
          return dstCel.release();
        }
      }
    }
  }
  // Image -> Tilemap (we'll need to generate new tilesets)
  else if (dstLayer->isTilemap()) {
    doc::ImageRef tilemap = dstCel->imageRef();
    draw_image_into_new_tilemap_cel(
      cmds, static_cast<doc::LayerTilemap*>(dstLayer), dstCel.get(),
      srcImage,
      // Use the grid origin of the sprite
      srcCel->sprite()->gridBounds().origin(),
      srcCel->bounds().origin(),
      srcCel->bounds(),
      tilemap);
  }
  else if ((dstSprite->pixelFormat() != srcImage->pixelFormat()) ||
           // If both images are indexed but with different palette, we can
           // convert the source cel to RGB first.
           (dstSprite->pixelFormat() == IMAGE_INDEXED &&
            srcImage->pixelFormat() == IMAGE_INDEXED &&
            srcCel->sprite()->palette(srcCel->frame())->countDiff(
              dstSprite->palette(dstFrame), nullptr, nullptr))) {
    ImageRef tmpImage(Image::create(IMAGE_RGB, srcImage->width(), srcImage->height()));
    tmpImage->clear(0);

    render::convert_pixel_format(
      srcImage,
      tmpImage.get(),
      IMAGE_RGB,
      render::Dithering(),
      srcCel->sprite()->rgbMap(srcCel->frame()),
      srcCel->sprite()->palette(srcCel->frame()),
      srcCel->layer()->isBackground(),
      0);

    render::convert_pixel_format(
      tmpImage.get(),
      dstCel->image(),
      IMAGE_INDEXED,
      render::Dithering(),
      dstSprite->rgbMap(dstFrame),
      dstSprite->palette(dstFrame),
      srcCel->layer()->isBackground(),
      dstSprite->transparentColor());
  }
  // Simple case, where we copy both images
  else {
    render::composite_image(
      dstCel->image(),
      srcImage,
      srcCel->sprite()->palette(srcCel->frame()),
      0, 0, 255, BlendMode::SRC);
  }

  // Resize a referece cel to a non-reference layer
  if (srcCel->layer()->isReference() && !dstLayer->isReference()) {
    gfx::RectF srcBounds = srcCel->boundsF();

    std::unique_ptr<Cel> dstCel2(
      new Cel(dstFrame,
              ImageRef(Image::create(dstSprite->pixelFormat(),
                                     std::ceil(srcBounds.w),
                                     std::ceil(srcBounds.h)))));
    algorithm::resize_image(
      dstCel->image(), dstCel2->image(),
      algorithm::RESIZE_METHOD_NEAREST_NEIGHBOR,
      nullptr, nullptr, 0);

    dstCel.reset(dstCel2.release());
    dstCel->setPosition(gfx::Point(srcBounds.origin()));
  }
  // Copy original cel bounds
  else if (!dstLayer->isTilemap()) {
    if (srcCel->layer() &&
        srcCel->layer()->isReference()) {
      dstCel->setBoundsF(srcCel->boundsF());
    }
    else {
      dstCel->setPosition(srcCel->position());
    }
  }

  return dstCel.release();
}

void draw_image_into_new_tilemap_cel(
  CmdSequence* cmds,
  doc::LayerTilemap* dstLayer,
  doc::Cel* dstCel,
  const doc::Image* srcImage,
  const gfx::Point& gridOrigin,
  const gfx::Point& srcImagePos,
  const gfx::Rect& canvasBounds,
  doc::ImageRef& newTilemap)
{
  ASSERT(dstLayer->isTilemap());

  doc::Tileset* tileset = dstLayer->tileset();
  doc::Grid grid = tileset->grid();
  grid.origin(gridOrigin);

  gfx::Size tileSize = grid.tileSize();
  const gfx::Rect tilemapBounds = grid.canvasToTile(canvasBounds);

  if (!newTilemap) {
    newTilemap.reset(doc::Image::create(IMAGE_TILEMAP,
                                        tilemapBounds.w,
                                        tilemapBounds.h));
    newTilemap->setMaskColor(doc::notile);
    newTilemap->clear(doc::notile);
  }
  else {
    ASSERT(tilemapBounds.w == newTilemap->width());
    ASSERT(tilemapBounds.h == newTilemap->height());
  }

  for (const gfx::Point& tilePt : grid.tilesInCanvasRegion(gfx::Region(canvasBounds))) {
    const gfx::Point tilePtInCanvas = grid.tileToCanvas(tilePt);
    doc::ImageRef tileImage(
      doc::crop_image(srcImage,
                      tilePtInCanvas.x-srcImagePos.x,
                      tilePtInCanvas.y-srcImagePos.y,
                      tileSize.w, tileSize.h,
                      srcImage->maskColor()));
    if (grid.hasMask())
      mask_image(tileImage.get(), grid.mask().get());

    preprocess_transparent_pixels(tileImage.get());

    doc::tile_index tileIndex;
    if (!tileset->findTileIndex(tileImage, tileIndex)) {
      auto addTile = new cmd::AddTile(tileset, tileImage);

      if (cmds)
        cmds->executeAndAdd(addTile);
      else {
        // TODO a little hacky
        addTile->execute(
          static_cast<Doc*>(dstLayer->sprite()->document())->context());
      }

      tileIndex = addTile->tileIndex();

      if (!cmds)
        delete addTile;
    }

    // We were using newTilemap->putPixel() directly but received a
    // crash report about an "access violation". So now we've added
    // some checks to the operation.
    {
      const int u = tilePt.x-tilemapBounds.x;
      const int v = tilePt.y-tilemapBounds.y;
      ASSERT((u >= 0) && (v >= 0) && (u < newTilemap->width()) && (v < newTilemap->height()));
      doc::put_pixel(newTilemap.get(), u, v, tileIndex);
    }
  }

  static_cast<Doc*>(dstLayer->sprite()->document())
    ->notifyTilesetChanged(tileset);

  dstCel->data()->setImage(newTilemap, dstLayer);
  dstCel->setPosition(grid.tileToCanvas(tilemapBounds.origin()));
}

void modify_tilemap_cel_region(
  CmdSequence* cmds,
  doc::Cel* cel,
  doc::Tileset* tileset,
  const gfx::Region& region,
  const TilesetMode tilesetMode,
  const GetTileImageFunc& getTileImage,
  const gfx::Region& forceRegion)
{
  OPS_TRACE("modify_tilemap_cel_region %d %d %d %d\n",
            region.bounds().x, region.bounds().y,
            region.bounds().w, region.bounds().h);

  if (region.isEmpty())
    return;

  ASSERT(cel->layer() && cel->layer()->isTilemap());
  ASSERT(cel->image()->pixelFormat() == IMAGE_TILEMAP);

  doc::LayerTilemap* tilemapLayer = static_cast<doc::LayerTilemap*>(cel->layer());

  Doc* doc = static_cast<Doc*>(tilemapLayer->sprite()->document());
  bool addUndoToTileset = false;
  if (!tileset) {
    tileset = tilemapLayer->tileset();
    addUndoToTileset = true;
  }
  doc::Grid grid = tileset->grid();
  grid.origin(grid.origin() + cel->position());

  const gfx::Size tileSize = grid.tileSize();
  const gfx::Rect oldTilemapBounds(grid.canvasToTile(cel->position()),
                                   cel->image()->bounds().size());
  const gfx::Rect patchTilemapBounds = grid.canvasToTile(region.bounds());
  const gfx::Rect newTilemapBounds = (oldTilemapBounds | patchTilemapBounds);

  OPS_TRACE("modify_tilemap_cel_region:\n"
            " - grid.origin       =%d %d\n"
            " - cel.position      =%d %d\n"
            " - oldTilemapBounds  =%d %d %d %d\n"
            " - patchTilemapBounds=%d %d %d %d (region.bounds = %d %d %d %d)\n"
            " - newTilemapBounds  =%d %d %d %d\n",
            grid.origin().x, grid.origin().y,
            cel->position().x, cel->position().y,
            oldTilemapBounds.x, oldTilemapBounds.y, oldTilemapBounds.w, oldTilemapBounds.h,
            patchTilemapBounds.x, patchTilemapBounds.y, patchTilemapBounds.w, patchTilemapBounds.h,
            region.bounds().x, region.bounds().y, region.bounds().w, region.bounds().h,
            newTilemapBounds.x, newTilemapBounds.y, newTilemapBounds.w, newTilemapBounds.h);

  // Autogenerate tiles
  if (tilesetMode == TilesetMode::Auto ||
      tilesetMode == TilesetMode::Stack) {
    // TODO create a smaller image
    doc::ImageRef newTilemap(
      doc::Image::create(IMAGE_TILEMAP,
                         newTilemapBounds.w,
                         newTilemapBounds.h));

    newTilemap->setMaskColor(doc::notile);
    newTilemap->clear(doc::notile);   // TODO find the tile with empty content?
    newTilemap->copy(
      cel->image(),
      gfx::Clip(oldTilemapBounds.x-newTilemapBounds.x,
                oldTilemapBounds.y-newTilemapBounds.y, 0, 0,
                oldTilemapBounds.w, oldTilemapBounds.h));

    gfx::Region tilePtsRgn;

    // This region includes the modified region by the user + the
    // extra region added as we've incremented the tilemap size
    // (newTilemapBounds).
    gfx::Region regionToPatch(grid.tileToCanvas(newTilemapBounds));
    regionToPatch -= gfx::Region(grid.tileToCanvas(oldTilemapBounds));
    regionToPatch |= region;

    std::vector<bool> modifiedTileIndexes(tileset->size(), false);
    std::vector<size_t> tilesHistogram(tileset->size(), 0);
    if (tilesetMode == TilesetMode::Auto) {
      for_each_tile_using_tileset(
        tileset, [tileset, &tilesHistogram](const doc::tile_t t){
                   if (t != doc::notile) {
                     doc::tile_index ti = doc::tile_geti(t);
                     if (ti >= 0 && ti < tileset->size())
                       ++tilesHistogram[ti];
                   }
                 });
    }

    for (const gfx::Point& tilePt : grid.tilesInCanvasRegion(regionToPatch)) {
      const int u = tilePt.x-newTilemapBounds.x;
      const int v = tilePt.y-newTilemapBounds.y;
      OPS_TRACE(" - modify tile xy=%d %d uv=%d %d\n", tilePt.x, tilePt.y, u, v);
      if (!newTilemap->bounds().contains(u, v))
        continue;

      const doc::tile_t t = newTilemap->getPixel(u, v);
      const doc::tile_index ti = (t != doc::notile ? doc::tile_geti(t): doc::notile);
      const doc::ImageRef existentTileImage = tileset->get(ti);

      const gfx::Rect tileInCanvasRc(grid.tileToCanvas(tilePt), tileSize);
      ImageRef tileImage(getTileImage(existentTileImage, tileInCanvasRc));
      if (grid.hasMask())
        mask_image(tileImage.get(), grid.mask().get());

      preprocess_transparent_pixels(tileImage.get());

      tile_index tileIndex;
      if (tileset->findTileIndex(tileImage, tileIndex)) {
        // We can re-use an existent tile (tileIndex) from the tileset
      }
      else if (tilesetMode == TilesetMode::Auto &&
               t != doc::notile &&
               ti >= 0 && ti < tilesHistogram.size() &&
               // If the tile is just used once, we can modify this
               // same tile
               tilesHistogram[ti] == 1) {
        // Common case: Re-utilize the same tile in Auto mode.
        tileIndex = ti;
        cmds->executeAndAdd(
          new cmd::CopyTileRegion(
            existentTileImage.get(),
            tileImage.get(),
            gfx::Region(tileImage->bounds()), // TODO calculate better region
            gfx::Point(0, 0),
            false,
            tileIndex,
            tileset));
      }
      else {
        auto addTile = new cmd::AddTile(tileset, tileImage);
        cmds->executeAndAdd(addTile);

        tileIndex = addTile->tileIndex();
      }

      // If the tile changed, we have to remove the old tile index
      // (ti) from the histogram count.
      if (tilesetMode == TilesetMode::Auto &&
          t != doc::notile &&
          ti >= 0 && ti < tilesHistogram.size() &&
          ti != tileIndex) {
        --tilesHistogram[ti];

        // It indicates that the tile "ti" was modified to
        // "tileIndex", so then, in case that we have to remove tiles,
        // we can check the ones that were modified & are unused.
        modifiedTileIndexes[ti] = true;
      }

      OPS_TRACE(" - tile %d -> %d\n",
                (t == doc::notile ? -1: ti),
                tileIndex);

      const doc::tile_t tile = doc::tile(tileIndex, 0);
      if (t != tile) {
        newTilemap->putPixel(u, v, tile);
        tilePtsRgn |= gfx::Region(gfx::Rect(u, v, 1, 1));

        // We add the new one tileIndex in the histogram count.
        if (tilesetMode == TilesetMode::Auto &&
            tileIndex != doc::notile &&
            tileIndex >= 0 && tileIndex < tilesHistogram.size()) {
          ++tilesHistogram[tileIndex];
        }
      }
    }

    if (newTilemap->width() != cel->image()->width() ||
        newTilemap->height() != cel->image()->height()) {
      gfx::Point newPos = grid.tileToCanvas(newTilemapBounds.origin());
      if (cel->position() != newPos) {
        cmds->executeAndAdd(
          new cmd::SetCelPosition(cel, newPos.x, newPos.y));
      }
      cmds->executeAndAdd(
        new cmd::ReplaceImage(cel->sprite(), cel->imageRef(), newTilemap));
    }
    else if (!tilePtsRgn.isEmpty()) {
      cmds->executeAndAdd(
        new cmd::CopyRegion(
          cel->image(),
          newTilemap.get(),
          tilePtsRgn,
          gfx::Point(0, 0)));
    }

    // Remove unused tiles
    if (tilesetMode == TilesetMode::Auto) {
      remove_unused_tiles_from_tileset(cmds, tileset,
                                       tilesHistogram,
                                       modifiedTileIndexes);
    }

    doc->notifyTilesetChanged(tileset);
  }
  // Modify active set of tiles manually / don't auto-generate new tiles
  else if (tilesetMode == TilesetMode::Manual) {
    std::vector<Mod> mods;

    for (const gfx::Point& tilePt : grid.tilesInCanvasRegion(region)) {
      // Ignore modifications outside the tilemap
      if (!cel->image()->bounds().contains(tilePt.x, tilePt.y))
        continue;

      const doc::tile_t t = cel->image()->getPixel(tilePt.x, tilePt.y);
      if (t == doc::notile)
        continue;

      const doc::tile_index ti = doc::tile_geti(t);
      const doc::ImageRef existentTileImage = tileset->get(ti);
      if (!existentTileImage) {
        // TODO add support to fill the tileset with the tile "ti"
        continue;
      }

      const gfx::Rect tileInCanvasRc(grid.tileToCanvas(tilePt), tileSize);
      ImageRef tileImage(getTileImage(existentTileImage, tileInCanvasRc));
      if (grid.hasMask())
        mask_image(tileImage.get(), grid.mask().get());

      gfx::Region tileRgn(tileInCanvasRc);
      tileRgn.createIntersection(tileRgn, region);
      tileRgn.offset(-tileInCanvasRc.origin());

      ImageRef tileDstImage = tileset->get(ti);

      // Compare with the original tile from the original tileset
      gfx::Region diffRgn;
      create_region_with_differences(tilemapLayer->tileset()->get(ti).get(),
                                     tileImage.get(),
                                     tileRgn.bounds(),
                                     diffRgn);

      // Keep only the modified region for this specific modification
      tileRgn &= diffRgn;

      if (!forceRegion.isEmpty()) {
        gfx::Region fr(forceRegion);
        fr.offset(-tileInCanvasRc.origin());
        tileRgn |= fr;
      }

      if (!tileRgn.isEmpty()) {
        if (addUndoToTileset) {
          Mod mod;
          mod.tileIndex = ti;
          mod.tileDstImage = tileDstImage;
          mod.tileImage = tileImage;
          mod.tileRgn = tileRgn;
          mods.push_back(mod);
        }
        else {
          copy_image(tileDstImage.get(),
                     tileImage.get(),
                     tileRgn);
          tileset->notifyTileContentChange(ti);
        }
      }
    }

    // Apply all modifications to tiles
    if (addUndoToTileset) {
      for (auto& mod : mods) {
        // TODO avoid creating several CopyTileRegion for the same tile,
        //      merge all mods for the same tile in some way
        cmds->executeAndAdd(
          new cmd::CopyTileRegion(
            mod.tileDstImage.get(),
            mod.tileImage.get(),
            mod.tileRgn,
            gfx::Point(0, 0),
            false,
            mod.tileIndex,
            tileset));
      }
    }

    doc->notifyTilesetChanged(tileset);
  }

#ifdef _DEBUG
  tileset->assertValidHashTable();
#endif
}

void clear_mask_from_cel(CmdSequence* cmds,
                         doc::Cel* cel,
                         const TilemapMode tilemapMode,
                         const TilesetMode tilesetMode)
{
  ASSERT(cmds);
  ASSERT(cel);
  ASSERT(cel->layer());

  if (cel->layer()->isTilemap() && tilemapMode == TilemapMode::Pixels) {
    Doc* doc = static_cast<Doc*>(cel->document());

    // Simple case (there is no visible selection, so we remove the
    // whole cel)
    if (!doc->isMaskVisible()) {
      cmds->executeAndAdd(new cmd::ClearCel(cel));
      return;
    }

    color_t bgcolor = doc->bgColor(cel->layer());
    doc::Mask* mask = doc->mask();

    modify_tilemap_cel_region(
      cmds, cel, nullptr,
      gfx::Region(doc->mask()->bounds()),
      tilesetMode,
      [bgcolor, mask](const doc::ImageRef& origTile,
                      const gfx::Rect& tileBoundsInCanvas) -> doc::ImageRef {
        doc::ImageRef modified(doc::Image::createCopy(origTile.get()));
        doc::algorithm::fill_selection(
          modified.get(),
          tileBoundsInCanvas,
          mask,
          bgcolor,
          nullptr);
        return modified;
      });
  }
  else {
    cmds->executeAndAdd(new cmd::ClearMask(cel));
  }
}

static void remove_unused_tiles_from_tileset(
  CmdSequence* cmds,
  doc::Tileset* tileset,
  std::vector<size_t>& tilesHistogram,
  const std::vector<bool>& modifiedTileIndexes)
{
  OPS_TRACE("remove_unused_tiles_from_tileset\n");

  int n = tileset->size();
#ifdef _DEBUG
  // Histogram just to check that we've a correct tilesHistogram
  std::vector<size_t> tilesHistogram2(n, 0);
#endif

  for_each_tile_using_tileset(
    tileset,
    [&n
#ifdef _DEBUG
     , &tilesHistogram2
#endif
     ](const doc::tile_t t){
      if (t != doc::notile) {
        const doc::tile_index ti = doc::tile_geti(t);
        n = std::max<int>(n, ti+1);
#ifdef _DEBUG
        // This check is necessary in case the tilemap has a reference
        // to a tile outside the valid range (e.g. when we resize the
        // tileset deleting tiles that will not be present anymore)
        if (ti >= 0 && ti < tilesHistogram2.size())
          ++tilesHistogram2[ti];
#endif
      }
    });

#ifdef _DEBUG
  for (int k=0; k<tilesHistogram.size(); ++k) {
    OPS_TRACE("comparing [%d] -> %d vs %d\n", k, tilesHistogram[k], tilesHistogram2[k]);
    ASSERT(tilesHistogram[k] == tilesHistogram2[k]);
  }
#endif

  doc::Remap remap(n);
  doc::tile_index ti, tj;
  ti = tj = 0;
  for (; ti<remap.size(); ++ti) {
    OPS_TRACE(" - ti=%d tj=%d tilesHistogram[%d]=%d\n",
              ti, tj, ti, (ti < tilesHistogram.size() ? tilesHistogram[ti]: 0));
    if (ti < tilesHistogram.size() &&
        tilesHistogram[ti] == 0 &&
        modifiedTileIndexes[ti]) {
      cmds->executeAndAdd(new cmd::RemoveTile(tileset, tj));
      // Map to nothing, so the map can be invertible
      remap.notile(ti);
    }
    else {
      remap.map(ti, tj++);
    }
  }

  if (!remap.isIdentity()) {
#ifdef _DEBUG
    for (ti=0; ti<remap.size(); ++ti) {
      OPS_TRACE(" - remap tile[%d] -> %d\n", ti, remap[ti]);
    }
#endif
    cmds->executeAndAdd(new cmd::RemapTilemaps(tileset, remap));
  }
}

void move_tiles_in_tileset(
  CmdSequence* cmds,
  doc::Tileset* tileset,
  doc::PalettePicks& picks,
  int& currentEntry,
  int beforeIndex)
{
  OPS_TRACE("move_tiles_in_tileset\n");

  // We cannot move the empty tile (index 0) no any place
  if (beforeIndex == 0)
    ++beforeIndex;
  if (picks.size() > 0 && picks[0])
    picks[0] = false;
  if (!picks.picks())
    return;

  picks.resize(std::max<int>(picks.size(), beforeIndex));

  int n = beforeIndex - tileset->size();
  if (n > 0) {
    while (n-- > 0)
      cmds->executeAndAdd(new cmd::AddTile(tileset, tileset->makeEmptyTile()));
  }

  Remap remap = create_remap_to_move_picks(picks, beforeIndex);
  cmds->executeAndAdd(new cmd::RemapTileset(tileset, remap));

  // New selection
  auto oldPicks = picks;
  for (int i=0; i<picks.size(); ++i)
    picks[remap[i]] = oldPicks[i];
  currentEntry = remap[currentEntry];
}

void copy_tiles_in_tileset(
  CmdSequence* cmds,
  doc::Tileset* tileset,
  doc::PalettePicks& picks,
  int& currentEntry,
  int beforeIndex)
{
  // We cannot move tiles before the empty tile
  if (beforeIndex == 0)
    ++beforeIndex;

  OPS_TRACE("copy_tiles_in_tileset beforeIndex=%d npicks=%d\n", beforeIndex, picks.picks());

  std::vector<ImageRef> newTiles;
  std::vector<UserData> newDatas;
  for (int i=0; i<picks.size(); ++i) {
    if (!picks[i])
      continue;
    else if (i >= 0 && i < tileset->size()) {
      newTiles.emplace_back(Image::createCopy(tileset->get(i).get()));
      newDatas.emplace_back(tileset->getTileData(i));
    }
    else {
      newTiles.emplace_back(tileset->makeEmptyTile());
      newDatas.emplace_back(UserData());
    }
  }

  int n;
  if (beforeIndex >= picks.size()) {
    n = beforeIndex;
    picks.resize(n);
  }
  else {
    n = tileset->size();
  }

  const int npicks = picks.picks();
  const int m = n + npicks;
  int j = 0;
  picks.resize(m);
  ASSERT(newTiles.size() == npicks);
  for (int i=0; i<m; ++i) {
    picks[i] = (i >= beforeIndex && i < beforeIndex + npicks);
    if (picks[i]) {
      // Fill the gap between the end of the tileset and the
      // "beforeIndex" with empty tiles
      while (tileset->size() < i)
        cmds->executeAndAdd(new cmd::AddTile(tileset, tileset->makeEmptyTile()));
      tileset->insert(i, newTiles[j], newDatas[j]);
      j++;
      cmds->executeAndAdd(new cmd::AddTile(tileset, i));
    }
  }
}

} // namespace app
