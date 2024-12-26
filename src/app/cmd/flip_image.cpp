// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/flip_image.h"

#include "doc/algorithm/flip_image.h"
#include "doc/image.h"

namespace app { namespace cmd {

FlipImage::FlipImage(Image* image, const gfx::Rect& bounds, doc::algorithm::FlipType flipType)
  : WithImage(image)
  , m_bounds(bounds)
  , m_flipType(flipType)
{
}

void FlipImage::onExecute()
{
  swap();
}

void FlipImage::onUndo()
{
  swap();
}

void FlipImage::swap()
{
  Image* image = this->image();

  // Flip the portion of the bitmap.
  doc::algorithm::flip_image(image, m_bounds, m_flipType);

  image->incrementVersion();
}

}} // namespace app::cmd
