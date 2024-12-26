// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
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

class SubObjectsIO {
public:
  virtual ~SubObjectsIO() {}
  virtual ImageRef getImageRef(ObjectId imageId) = 0;
  virtual CelDataRef getCelDataRef(ObjectId celdataId) = 0;
};

// Helper class used to read children-objects by layers and cels.
class SubObjectsFromSprite : public SubObjectsIO {
public:
  SubObjectsFromSprite(Sprite* sprite);

  Sprite* sprite() const { return m_sprite; }

  void addImageRef(const ImageRef& image);
  void addCelDataRef(const CelDataRef& celdata);

  ImageRef getImageRef(ObjectId imageId) override;
  CelDataRef getCelDataRef(ObjectId celdataId) override;

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
