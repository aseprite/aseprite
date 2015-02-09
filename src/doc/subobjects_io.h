// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_SUBOBJECTS_IO_H_INCLUDED
#define DOC_SUBOBJECTS_IO_H_INCLUDED
#pragma once

#include "doc/cel_data.h"
#include "doc/image_ref.h"

#include <iosfwd>
#include <map>

namespace doc {
  class Sprite;

  // Helper class used to read children-objects by layers and cels.
  class SubObjectsIO {
  public:
    SubObjectsIO(Sprite* sprite);

    Sprite* sprite() const { return m_sprite; }

    void addImageRef(const ImageRef& image);
    void addCelDataRef(const CelDataRef& celdata);

    ImageRef getImageRef(ObjectId imageId);
    CelDataRef getCelDataRef(ObjectId celdataId);

  private:
    Sprite* m_sprite;

    // Images list that can be queried from doc::read_celdata() using
    // getImageRef().
    std::map<ObjectId, ImageRef> m_images;

    // CelData list that can be queried from doc::read_cel() using
    // getCelDataRef().
    std::map<ObjectId, CelDataRef> m_celdatas;
  };

} // namespace doc

#endif
