// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/clear_slices.h"

#include "app/doc.h"
#include "app/site.h"
#include "app/util/cel_ops.h"
#include "doc/algorithm/fill_selection.h"
#include "doc/cel.h"
#include "doc/grid.h"
#include "doc/layer.h"
#include "doc/layer_list.h"
#include "doc/primitives.h"

namespace app {
namespace cmd {

using namespace doc;

ClearSlices::ClearSlices(const Site& site,
                         const LayerList& layers,
                         frame_t frame,
                         const std::vector<SliceKey>& slicesKeys)
                         : m_tilemapMode(site.tilemapMode())
                         , m_tilesetMode(site.tilesetMode())
{
  if (layers.empty())
    return;

  Doc* doc = static_cast<Doc*>((*layers.begin())->sprite()->document());

  for (auto* layer : layers) {
    Cel* cel = layer->cel(frame);
    if (!cel)
      continue;

    SlicesContent sc(cel);
    for (const auto& sk : slicesKeys) {
      sc.mask.add(sk.bounds());
    }
    gfx::Rect maskBounds = sc.mask.bounds();

    Image* image = cel->image();
    assert(image);
    if (!image)
      continue;

    gfx::Rect imageBounds = cel->bounds();

    color_t bgcolor = doc->bgColor(layer);
    if (image->pixelFormat() == IMAGE_TILEMAP) {
      auto grid = cel->grid();
      imageBounds = gfx::Rect(grid.canvasToTile(cel->position()),
                              cel->image()->size());
      maskBounds = grid.canvasToTile(maskBounds);
      bgcolor = doc::notile; // TODO configurable empty tile
    }

    gfx::Rect cropBounds = (imageBounds & maskBounds);
    if (cropBounds.isEmpty())
      continue;

    cropBounds.offset(-imageBounds.origin());

    sc.cropPos = cropBounds.origin();
    sc.bgcolor = bgcolor;
    sc.copy.reset(crop_image(image, cropBounds, sc.bgcolor));
    m_slicesContents.push_back(sc);
  }
}

void ClearSlices::onExecute()
{
  m_seq.execute(context());
  clear();
}

void ClearSlices::onUndo()
{
  restore();
  m_seq.undo();
}

void ClearSlices::onRedo()
{
  m_seq.redo();
  clear();
}

void ClearSlices::clear()
{
  for (auto& sc : m_slicesContents) {
    if (!sc.copy)
      continue;


    if (sc.cel()->layer()->isTilemap() && m_tilemapMode == TilemapMode::Pixels) {

      Doc* doc = static_cast<Doc*>(sc.cel()->document());
      color_t bgcolor = doc->bgColor(sc.cel()->layer());

      modify_tilemap_cel_region(
        &m_seq, sc.cel(), nullptr,
        gfx::Region(sc.mask.bounds()),
        m_tilesetMode,
        [sc, bgcolor](const doc::ImageRef& origTile,
                      const gfx::Rect& tileBoundsInCanvas) -> doc::ImageRef {
          doc::ImageRef modified(doc::Image::createCopy(origTile.get()));
          doc::algorithm::fill_selection(
            modified.get(),
            tileBoundsInCanvas,
            &sc.mask,
            bgcolor,
            nullptr);
          return modified;
        });
    }
    else {
      Grid grid = sc.cel()->grid();
      doc::algorithm::fill_selection(
        sc.cel()->image(),
        sc.cel()->bounds(),
        &sc.mask,
        sc.bgcolor,
        (sc.cel()->image()->isTilemap() ? &grid: nullptr));
    }
  }
}

void ClearSlices::restore()
{
  for (auto& sc : m_slicesContents) {
    if (!sc.copy)
      continue;

    copy_image(sc.cel()->image(),
               sc.copy.get(),
               sc.cropPos.x,
               sc.cropPos.y);
  }
}

} // namespace cmd
} // namespace app
