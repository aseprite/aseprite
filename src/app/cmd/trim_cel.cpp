// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/trim_cel.h"

#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/primitives.h"

namespace app {
namespace cmd {

using namespace doc;

TrimCel::TrimCel(Cel* cel)
  : WithCel(cel)
  , m_originalBounds(cel->image()->bounds())
  , m_color(cel->image()->maskColor())
  , m_removeCel(nullptr)
{
  if (algorithm::shrink_bounds(cel->image(), m_bounds, m_color)) {
    m_originalBounds.x = -m_bounds.x;
    m_originalBounds.y = -m_bounds.y;
  }
  else
    m_removeCel = new cmd::RemoveCel(cel);
}

void TrimCel::onExecute()
{
  if (m_removeCel)
    m_removeCel->execute(context());
  else
    cropImage(m_bounds);
}

void TrimCel::onUndo()
{
  if (m_removeCel)
    m_removeCel->undo();
  else
    cropImage(m_originalBounds);
}

void TrimCel::onRedo()
{
  if (m_removeCel)
    m_removeCel->redo();
  else
    cropImage(m_bounds);
}

// Crops the cel image leaving the same ID in the image.
void TrimCel::cropImage(const gfx::Rect& bounds)
{
  Cel* cel = this->cel();

  if (bounds == cel->image()->bounds())
    return;

  ImageRef image(crop_image(cel->image(),
                            bounds.x, bounds.y,
                            bounds.w, bounds.h,
                            m_color));

  ObjectId id = cel->image()->id();
  ObjectVersion ver = cel->image()->version();
  gfx::Point newPos = cel->position() + bounds.origin();

  cel->image()->setId(NullId);
  image->setId(id);
  image->setVersion(ver);
  image->incrementVersion();

  cel->data()->setImage(image);
  cel->data()->setPosition(newPos);
  cel->incrementVersion();
}

} // namespace cmd
} // namespace app
