// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_EXTRA_CEL_H_INCLUDED
#define APP_EXTRA_CEL_H_INCLUDED
#pragma once

#include "app/tilemap_mode.h"
#include "base/disable_copying.h"
#include "doc/blend_mode.h"
#include "doc/cel.h"
#include "doc/frame.h"
#include "doc/image_buffer.h"
#include "doc/image_ref.h"
#include "gfx/rect.h"
#include "render/extra_type.h"

#include <memory>

namespace doc {
class Sprite;
}

namespace app {

class ExtraCel {
public:
  enum class Purpose {
    Unknown,
    BrushPreview,
    TransformationPreview,
  };

  ExtraCel();

  void create(Purpose purpose,
              const TilemapMode tilemapMode,
              doc::Sprite* sprite,
              const gfx::Rect& bounds,
              const gfx::Size& imageSize,
              const doc::frame_t frame,
              const int opacity);
  void reset();

  Purpose purpose() const { return m_purpose; }

  render::ExtraType type() const { return m_type; }
  void setType(render::ExtraType type) { m_type = type; }

  doc::Cel* cel() const { return m_cel.get(); }
  doc::Image* image() const { return m_image.get(); }

  doc::BlendMode blendMode() const { return m_blendMode; }
  void setBlendMode(doc::BlendMode mode) { m_blendMode = mode; }

private:
  Purpose m_purpose;
  render::ExtraType m_type;
  std::unique_ptr<doc::Cel> m_cel;
  doc::ImageRef m_image;
  doc::ImageBufferPtr m_imageBuffer;
  doc::BlendMode m_blendMode;

  DISABLE_COPYING(ExtraCel);
};

typedef std::shared_ptr<ExtraCel> ExtraCelRef;

} // namespace app

#endif
