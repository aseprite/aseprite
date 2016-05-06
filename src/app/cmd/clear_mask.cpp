// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/clear_mask.h"

#include "app/cmd/clear_cel.h"
#include "app/document.h"
#include "doc/cel.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/primitives.h"

namespace app {
namespace cmd {

using namespace doc;

ClearMask::ClearMask(Cel* cel)
  : WithCel(cel)
{
  app::Document* doc = static_cast<app::Document*>(cel->document());

  // If the mask is empty or is not visible then we have to clear the
  // entire image in the cel.
  if (!doc->isMaskVisible()) {
    m_seq.add(new cmd::ClearCel(cel));
    return;
  }

  Image* image = (cel ? cel->image(): NULL);
  if (!image)
    return;

  Mask* mask = doc->mask();
  m_offsetX = mask->bounds().x - cel->x();
  m_offsetY = mask->bounds().y - cel->y();

  gfx::Rect bounds =
    image->bounds().createIntersection(
      gfx::Rect(
        m_offsetX, m_offsetY,
        mask->bounds().w, mask->bounds().h));
  if (bounds.isEmpty())
    return;

  m_dstImage.reset(new WithImage(image));
  m_bgcolor = doc->bgColor(cel->layer());
  m_boundsX = bounds.x;
  m_boundsY = bounds.y;

  m_copy.reset(crop_image(image,
      bounds.x, bounds.y, bounds.w, bounds.h, m_bgcolor));
}

void ClearMask::onExecute()
{
  m_seq.execute(context());
  if (m_dstImage)
    clear();
}

void ClearMask::onUndo()
{
  if (m_dstImage)
    restore();
  m_seq.undo();
}

void ClearMask::onRedo()
{
  m_seq.redo();
  if (m_dstImage)
    clear();
}

void ClearMask::clear()
{
  Cel* cel = this->cel();
  Image* image = m_dstImage->image();
  app::Document* doc = static_cast<app::Document*>(cel->document());
  Mask* mask = doc->mask();

  ASSERT(mask->bitmap());
  if (!mask->bitmap())
    return;

  const LockImageBits<BitmapTraits> maskBits(mask->bitmap());
  LockImageBits<BitmapTraits>::const_iterator it = maskBits.begin();

  // Clear the masked zones
  int u, v;
  for (v=0; v<mask->bounds().h; ++v) {
    for (u=0; u<mask->bounds().w; ++u, ++it) {
      ASSERT(it != maskBits.end());
      if (*it) {
        put_pixel(image,
          u + m_offsetX,
          v + m_offsetY, m_bgcolor);
      }
    }
  }

  ASSERT(it == maskBits.end());
}

void ClearMask::restore()
{
  copy_image(m_dstImage->image(), m_copy.get(), m_boundsX, m_boundsY);
}

} // namespace cmd
} // namespace app
