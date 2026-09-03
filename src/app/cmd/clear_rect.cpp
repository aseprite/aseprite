// Aseprite
// Copyright (C) 2025-present  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/clear_rect.h"

#include "app/context.h"
#include "app/doc.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/primitives.h"

namespace app { namespace cmd {

using namespace doc;

ClearRect::ClearRect(Cel* cel, const gfx::Rect& bounds, color_t color)
{
  ASSERT(cel);
  initialize(cel, bounds, color);
}

ClearRect::ClearRect(Cel* cel, const gfx::Rect& bounds)
{
  ASSERT(cel);
  Doc* doc = static_cast<Doc*>(cel->document());
  initialize(cel, bounds, doc->bgColor(cel->layer()));
}

void ClearRect::initialize(Cel* cel, const gfx::Rect& bounds, color_t color)
{
  ASSERT(cel);

  Image* image = cel->image();
  if (!image)
    return;

  m_offset.x = bounds.x - cel->x();
  m_offset.y = bounds.y - cel->y();

  gfx::Rect bounds2 = image->bounds().createIntersection(gfx::Rect(m_offset, bounds.size()));
  if (bounds.isEmpty())
    return;

  m_dstImage = WithImage(image);
  m_bgcolor = color;

  m_copy.reset(crop_image(image, bounds2.x, bounds2.y, bounds2.w, bounds2.h, m_bgcolor));
}

void ClearRect::onExecute(Context* ctx)
{
  clear();
}

void ClearRect::onUndo(Context* ctx)
{
  restore();
}

void ClearRect::onRedo(Context* ctx)
{
  clear();
}

void ClearRect::onSerialize(CmdSerial& s)
{
  Cmd::onSerialize(s);
  m_dstImage.serializeImageId(s);
  s(m_copy);
  s(m_offset.x);
  s(m_offset.y);
  s(m_bgcolor);
}

void ClearRect::clear()
{
  fill_rect(m_dstImage.image(),
            m_offset.x,
            m_offset.y,
            m_offset.x + m_copy->width() - 1,
            m_offset.y + m_copy->height() - 1,
            m_bgcolor);
}

void ClearRect::restore()
{
  copy_image(m_dstImage.image(), m_copy.get(), m_offset.x, m_offset.y);
}

}} // namespace app::cmd
