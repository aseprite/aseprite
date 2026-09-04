// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/crop_cel.h"

#include "app/doc.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/layer_tilemap.h"
#include "doc/primitives.h"
#include "gfx/region.h"

namespace app { namespace cmd {

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

void CropCel::onExecute(Context* ctx)
{
  cropImage(m_newOrigin, m_newBounds);
}

void CropCel::onUndo(Context* ctx)
{
  cropImage(m_oldOrigin, m_oldBounds);
}

void CropCel::onFireNotifications(Context* ctx)
{
  Cel* cel = this->cel();
  if (!cel)
    return;

  Doc* doc = static_cast<Doc*>(cel->document());
  if (!doc)
    return;

  gfx::Region rgn(gfx::Rect(m_oldBounds).offset(m_newOrigin));
  rgn |= gfx::Region(gfx::Rect(m_newBounds).offset(m_oldOrigin));
  doc->notifySpritePixelsModified(cel->sprite(), rgn, cel->frame());
}

// Crops the cel image leaving the same ID in the image.
void CropCel::cropImage(const gfx::Point& origin, const gfx::Rect& bounds)
{
  Cel* cel = this->cel();
  if (!cel->image())
    return;

  gfx::Rect localBounds(bounds);
  if (cel->layer()->isTilemap()) {
    doc::Tileset* tileset = static_cast<LayerTilemap*>(cel->layer())->tileset();
    if (tileset) {
      doc::Grid grid = tileset->grid();
      localBounds = grid.canvasToTile(bounds);
    }
  }
  if (bounds != cel->image()->bounds()) {
    ImageRef image(crop_image(cel->image(),
                              localBounds.x,
                              localBounds.y,
                              localBounds.w,
                              localBounds.h,
                              cel->image()->maskColor()));
    ObjectId id = cel->image()->id();
    ObjectVersion ver = cel->image()->version();

    cel->image()->setId(NullId);
    image->setId(id);
    image->setVersion(ver);
    image->incrementVersion();
    cel->data()->setImage(image, cel->layer());
    cel->data()->incrementVersion();
  }

  if (cel->data()->position() != origin) {
    cel->data()->setPosition(origin);
    cel->data()->incrementVersion();
  }
}

}} // namespace app::cmd
