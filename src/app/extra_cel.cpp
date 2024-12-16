// Aseprite
// Copyright (C) 2020-2024  Igara Studio S.A.
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
  : m_purpose(Purpose::Unknown)
  , m_type(render::ExtraType::NONE)
  , m_blendMode(doc::BlendMode::NORMAL)
{
}

void ExtraCel::create(Purpose purpose,
                      const TilemapMode tilemapMode,
                      doc::Sprite* sprite,
                      const gfx::Rect& bounds,
                      const gfx::Size& imageSize,
                      const doc::frame_t frame,
                      const int opacity)
{
  ASSERT(sprite);

  m_purpose = purpose;

  doc::PixelFormat pixelFormat;
  if (tilemapMode == TilemapMode::Tiles)
    pixelFormat = doc::IMAGE_TILEMAP;
  else
    pixelFormat = sprite->pixelFormat();

  if (!m_image || m_image->pixelFormat() != pixelFormat || m_image->width() != imageSize.w ||
      m_image->height() != imageSize.h) {
    if (!m_imageBuffer)
      m_imageBuffer.reset(new doc::ImageBuffer(1));
    doc::Image* newImage = doc::Image::create(pixelFormat, imageSize.w, imageSize.h, m_imageBuffer);
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

void ExtraCel::reset()
{
  m_purpose = Purpose::Unknown;
  m_type = render::ExtraType::NONE;
  m_image.reset();
  m_cel.reset();
}

} // namespace app
