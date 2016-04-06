// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_SCRIPT_IMAGE_WRAP_H_INCLUDED
#define APP_SCRIPT_IMAGE_WRAP_H_INCLUDED
#pragma once

#include "doc/image.h"
#include "doc/image_ref.h"
#include "gfx/region.h"

namespace app {
  class SpriteWrap;

  class ImageWrap {
  public:
    ImageWrap(SpriteWrap* sprite, doc::Image* img);
    ~ImageWrap();

    void commit();

    SpriteWrap* sprite() const { return m_sprite; }
    doc::Image* image() const { return m_image; }

    void modifyRegion(const gfx::Region& rgn);
  private:
    SpriteWrap* m_sprite;
    doc::Image* m_image;
    doc::ImageRef m_backup;
    gfx::Region m_modifiedRegion;
  };

} // namespace app

#endif
