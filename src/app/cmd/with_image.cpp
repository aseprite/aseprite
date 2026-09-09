// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/with_image.h"

#include "doc/image.h"

namespace app { namespace cmd {

using namespace doc;

WithImage::WithImage(Image* image) : m_imageId(image ? image->id() : NullId)
{
}

Image* WithImage::image()
{
  return get<Image>(m_imageId);
}

}} // namespace app::cmd
