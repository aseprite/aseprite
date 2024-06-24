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
#include "doc/algorithm/fill_selection.h"
#include "doc/cel.h"
#include "doc/grid.h"
#include "doc/layer.h"
#include "doc/layer_list.h"
#include "doc/primitives.h"

namespace app {
namespace cmd {

using namespace doc;

ClearSlices::ClearSlices(const LayerList& layers, frame_t frame, const std::vector<SliceKey>& slicesKeys)
{
  //Doc* doc = static_cast<Doc*>(cel->document());

  if (layers.empty())
    return;

  Doc* doc = static_cast<Doc*>((*layers.begin())->sprite()->document());

  for (const auto& sk : slicesKeys) {
    m_mask.add(sk.bounds());
  }
  const gfx::Rect maskBounds = m_mask.bounds();

//  gfx::Rect maskBounds;
//  if (image->pixelFormat() == IMAGE_TILEMAP) {
//    auto grid = cel->grid();
//    imageBounds = gfx::Rect(grid.canvasToTile(cel->position()),
//                            cel->image()->size());
//    maskBounds = grid.canvasToTile(mask->bounds());
//    m_bgcolor = doc::notile; // TODO configurable empty tile
//  }
//  else {
  for (auto* layer : layers) {
    Cel* cel = layer->cel(frame);
    const gfx::Rect imageBounds = cel->bounds();
    gfx::Rect cropBounds = (imageBounds & maskBounds);
    if (cropBounds.isEmpty())
      continue;

    cropBounds.offset(-imageBounds.origin());

    Image* image = cel->image();
    assert(image);
    if (!image)
      continue;

    SlicesContent sc;
    sc.cel = cel;
    sc.cropPos = cropBounds.origin();
    sc.bgcolor = doc->bgColor(layer);
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

    Grid grid = sc.cel->grid();
    doc::algorithm::fill_selection(
      sc.cel->image(),
      sc.cel->bounds(),
      &m_mask,
      sc.bgcolor,
      (sc.cel->image()->isTilemap() ? &grid: nullptr));

  }
}

void ClearSlices::restore()
{
  for (auto& sc : m_slicesContents) {
    if (!sc.copy)
      continue;

    copy_image(sc.cel->image(),
              sc.copy.get(),
              sc.cropPos.x,
              sc.cropPos.y);
  }
}

} // namespace cmd
} // namespace app
