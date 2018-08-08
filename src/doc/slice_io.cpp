// Aseprite Document Library
// Copyright (c) 2017-2018 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/slice_io.h"

#include "base/serialization.h"
#include "doc/slice.h"
#include "doc/string_io.h"
#include "doc/user_data_io.h"

#include <iostream>
#include <memory>

namespace doc {

using namespace base::serialization;
using namespace base::serialization::little_endian;

void write_slice(std::ostream& os, const Slice* slice)
{
  write32(os, slice->id());
  write_string(os, slice->name());
  write_user_data(os, slice->userData());

  // Number of keys
  write32(os, slice->size());
  for (const auto& key : *slice) {
    write32(os, key.frame());
    write_slicekey(os, *key.value());
  }
}

Slice* read_slice(std::istream& is, bool setId)
{
  ObjectId id = read32(is);
  std::string name = read_string(is);
  UserData userData = read_user_data(is);
  size_t nkeys = read32(is);

  std::unique_ptr<Slice> slice(new Slice);
  slice->setName(name);
  slice->setUserData(userData);
  while (nkeys--) {
    frame_t fr = read32(is);
    slice->insert(fr, read_slicekey(is));
  }

  if (setId)
    slice->setId(id);
  return slice.release();
}

void write_slicekey(std::ostream& os, const SliceKey& sliceKey)
{
  write32(os, sliceKey.bounds().x);
  write32(os, sliceKey.bounds().y);
  write32(os, sliceKey.bounds().w);
  write32(os, sliceKey.bounds().h);
  write32(os, sliceKey.center().x);
  write32(os, sliceKey.center().y);
  write32(os, sliceKey.center().w);
  write32(os, sliceKey.center().h);
  write32(os, sliceKey.pivot().x);
  write32(os, sliceKey.pivot().y);
}

SliceKey read_slicekey(std::istream& is)
{
  gfx::Rect bounds, center;
  gfx::Point pivot;
  bounds.x = read32(is);
  bounds.y = read32(is);
  bounds.w = read32(is);
  bounds.h = read32(is);
  center.x = read32(is);
  center.y = read32(is);
  center.w = read32(is);
  center.h = read32(is);
  pivot.x = read32(is);
  pivot.y = read32(is);
  return SliceKey(bounds, center, pivot);
}

}
