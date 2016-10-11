// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/extra_cel.h"

#include "doc/sprite.h"

namespace app {

ExtraCel::ExtraCel()
  : m_type(render::ExtraType::NONE)
  , m_blendMode(doc::BlendMode::NORMAL)
{
}

void ExtraCel::create(doc::Sprite* sprite,
                      const gfx::Rect& bounds,
                      doc::frame_t frame,
                      int opacity)
{
  ASSERT(sprite);

  if (!m_image ||
      m_image->pixelFormat() != sprite->pixelFormat() ||
      m_image->width() != bounds.w ||
      m_image->height() != bounds.h) {
    if (!m_imageBuffer)
      m_imageBuffer.reset(new doc::ImageBuffer(1));
    doc::Image* newImage = doc::Image::create(sprite->pixelFormat(),
                                              bounds.w, bounds.h,
                                              m_imageBuffer);
    m_image.reset(newImage);
  }

  if (!m_cel) {
    // Ignored fields for this cel (frame, and image index)
    m_cel.reset(new doc::Cel(doc::frame_t(0), doc::ImageRef(nullptr)));
  }

  m_cel->setBounds(bounds);
  m_cel->setOpacity(opacity);
  m_cel->setFrame(frame);
}

} // namespace app
