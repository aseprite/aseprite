// Aseprite
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
#include "doc/mask.h"
#include "doc/primitives.h"

namespace app {
namespace cmd {

using namespace doc;

ClearMask::ClearMask(Cel* cel)
  : WithCel(cel)
{
  Doc* doc = static_cast<Doc*>(cel->document());

  // If the mask is empty or is not visible then we have to clear the
  // entire image in the cel.
  if (!doc->isMaskVisible()) {
    m_seq.add(new cmd::ClearCel(cel));
    return;
  }

  Image* image = cel->image();
  assert(image);
  if (!image)
    return;

  Mask* mask = doc->mask();
  m_offset = mask->bounds().origin() - cel->position();

  gfx::Rect bounds =
    image->bounds().createIntersection(
      gfx::Rect(
        m_offset.x, m_offset.y,
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
  Doc* doc = static_cast<Doc*>(cel->document());
  Mask* mask = doc->mask();

  doc::algorithm::fill_selection(image, m_offset, mask, m_bgcolor);
}

void ClearMask::restore()
{
  copy_image(m_dstImage->image(), m_copy.get(), m_boundsX, m_boundsY);
}

} // namespace cmd
} // namespace app
