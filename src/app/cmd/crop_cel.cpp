// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/crop_cel.h"

#include "doc/cel.h"
#include "doc/primitives.h"

namespace app {
namespace cmd {

using namespace doc;

CropCel::CropCel(Cel* cel, const gfx::Rect& newBounds)
  : WithCel(cel)
  , m_oldOrigin(cel->position())
  , m_newOrigin(newBounds.origin())
  , m_oldBounds(cel->bounds())
  , m_newBounds(newBounds)
{
  m_oldBounds.offset(-m_newOrigin);
  m_newBounds.offset(-m_oldOrigin);

  ASSERT(m_newBounds != m_oldBounds);
}

void CropCel::onExecute()
{
  cropImage(m_newOrigin, m_newBounds);
}

void CropCel::onUndo()
{
  cropImage(m_oldOrigin, m_oldBounds);
}

// Crops the cel image leaving the same ID in the image.
void CropCel::cropImage(const gfx::Point& origin,
                        const gfx::Rect& bounds)
{
  Cel* cel = this->cel();

  if (bounds != cel->image()->bounds()) {
    ImageRef image(crop_image(cel->image(),
                              bounds.x, bounds.y,
                              bounds.w, bounds.h,
                              cel->image()->maskColor()));
    ObjectId id = cel->image()->id();
    ObjectVersion ver = cel->image()->version();

    cel->image()->setId(NullId);
    image->setId(id);
    image->setVersion(ver);
    image->incrementVersion();
    cel->data()->setImage(image);
    cel->data()->incrementVersion();
  }

  if (cel->data()->position() != origin) {
    cel->data()->setPosition(origin);
    cel->data()->incrementVersion();
  }
}

} // namespace cmd
} // namespace app
