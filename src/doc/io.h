// Aseprite Document Library
// Copyright (c) 2026-present Igara Studio S.A.
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_IO_H_INCLUDED
#define DOC_IO_H_INCLUDED
#pragma once

#include "doc/cel_data.h"
#include "doc/image_ref.h"

#include <iosfwd>
#include <map>

namespace doc {
class Sprite;

class CancelIO {
public:
  virtual ~CancelIO() {}
  virtual bool isCanceled() = 0;
};

class IdMapperIO {
public:
  virtual ~IdMapperIO() {}
  virtual ObjectId mapId(const ObjectId id, const ObjectType type) const { return id; }
};

// Doesn't set the ID of the object.
class NullIdMapperIO : public IdMapperIO {
public:
  ObjectId mapId(const ObjectId id, const ObjectType type) const override { return NullId; }
};

// Sets the ID as it was read from the std::istream
class IdFromStreamMapperIO : public IdMapperIO {
public:
  ObjectId mapId(const ObjectId id, const ObjectType type) const override { return id; }
};

class SubObjectsIO {
public:
  virtual ~SubObjectsIO() {}
  virtual Sprite* sprite() const = 0;
  virtual void addImageRef(const ImageRef& image) = 0;
  virtual void addCelDataRef(const CelDataRef& celdata) = 0;
  virtual ImageRef getImageRef(ObjectId imageId) = 0;
  virtual CelDataRef getCelDataRef(ObjectId celdataId) = 0;
};

// Helper class used to read children-objects by layers and cels.
class SubObjectsFromSprite : public SubObjectsIO {
public:
  SubObjectsFromSprite(Sprite* sprite);

  // SubObjectsIO impl
  Sprite* sprite() const override { return m_sprite; }
  void addImageRef(const ImageRef& image) override;
  void addCelDataRef(const CelDataRef& celdata) override;
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
