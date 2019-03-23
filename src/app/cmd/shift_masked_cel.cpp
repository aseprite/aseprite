// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/shift_masked_cel.h"

#include "app/doc.h"
#include "doc/algorithm/shift_image.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/mask.h"

namespace app {
namespace cmd {

ShiftMaskedCel::ShiftMaskedCel(Cel* cel, int dx, int dy)
  : WithCel(cel)
  , m_dx(dx)
  , m_dy(dy)
{
}

void ShiftMaskedCel::onExecute()
{
  shift(m_dx, m_dy);
}

void ShiftMaskedCel::onUndo()
{
  shift(-m_dx, -m_dy);
}

void ShiftMaskedCel::shift(int dx, int dy)
{
  Cel* cel = this->cel();
  Mask* mask = static_cast<Doc*>(cel->document())->mask();
  ASSERT(mask->bitmap());
  if (!mask->bitmap())
    return;

  gfx::Rect newBounds;
  ImageRef newImage =
    doc::algorithm::shift_image_with_mask(cel, mask, dx, dy, newBounds);

  ImageRef oldImage = cel->imageRef();
  if (!is_same_image(oldImage.get(), newImage.get())) {
    ObjectId id = oldImage->id();
    ObjectVersion ver = oldImage->version();
    oldImage->setId(NullId);

    newImage->setId(id);
    newImage->setVersion(ver);
    newImage->incrementVersion();
    cel->data()->setImage(newImage);
  }
  cel->data()->setBounds(newBounds);
  cel->data()->incrementVersion();
}

} // namespace cmd
} // namespace app
