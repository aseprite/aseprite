// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/clear_mask.h"

#include "app/cmd/clear_cel.h"
#include "app/doc.h"
#include "doc/algorithm/fill_selection.h"
#include "doc/cel.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/mask.h"
#include "doc/primitives.h"

namespace app { namespace cmd {

using namespace doc;

ClearMask::ClearMask(Cel* cel) : WithCel(cel)
{
  Doc* doc = static_cast<Doc*>(cel->document());

  // If the mask is empty or is not visible then we have to clear the
  // entire image in the cel.
  if (!doc->isMaskVisible()) {
    m_seq.add(new cmd::ClearCel(cel));

    // In this case m_copy will be nullptr, so the clear()/restore()
    // member functions will have no effect.
    return;
  }

  Image* image = cel->image();
  assert(image);
  if (!image)
    return;

  const Mask* mask = doc->mask();
  gfx::Rect imageBounds;
  gfx::Rect maskBounds;
  if (image->pixelFormat() == IMAGE_TILEMAP) {
    auto grid = cel->grid();
    imageBounds = gfx::Rect(grid.canvasToTile(cel->position()), cel->image()->size());
    maskBounds = grid.canvasToTile(mask->bounds());
    m_bgcolor = doc::notile; // TODO configurable empty tile
  }
  else {
    imageBounds = cel->bounds();
    maskBounds = mask->bounds();
    m_bgcolor = doc->bgColor(cel->layer());
  }

  gfx::Rect cropBounds = (imageBounds & maskBounds);
  if (cropBounds.isEmpty())
    return;

  cropBounds.offset(-imageBounds.origin());
  m_cropPos = cropBounds.origin();

  m_copy.reset(crop_image(image, cropBounds, m_bgcolor));
}

void ClearMask::onExecute()
{
  m_seq.execute(context());
  clear();
}

void ClearMask::onUndo()
{
  restore();
  m_seq.undo();
}

void ClearMask::onRedo()
{
  m_seq.redo();
  clear();
}

void ClearMask::clear()
{
  if (!m_copy)
    return;

  Cel* cel = this->cel();
  Doc* doc = static_cast<Doc*>(cel->document());
  Mask* mask = doc->mask();

  Grid grid = cel->grid();
  doc::algorithm::fill_selection(cel->image(),
                                 cel->bounds(),
                                 mask,
                                 m_bgcolor,
                                 (cel->image()->isTilemap() ? &grid : nullptr));
}

void ClearMask::restore()
{
  if (!m_copy)
    return;

  Cel* cel = this->cel();
  copy_image(cel->image(), m_copy.get(), m_cropPos.x, m_cropPos.y);
}

}} // namespace app::cmd
