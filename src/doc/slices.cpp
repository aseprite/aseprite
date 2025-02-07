// Aseprite Document Library
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/slices.h"

#include "base/debug.h"
#include "doc/slice.h"

#include <algorithm>

namespace doc {

Slices::Slices(Sprite* sprite) : m_sprite(sprite)
{
}

Slices::~Slices()
{
  for (Slice* slice : m_slices) {
    slice->setOwner(nullptr);
    delete slice;
  }
}

void Slices::add(Slice* slice)
{
  m_slices.push_back(slice);
  slice->setOwner(this);
}

void Slices::remove(Slice* slice)
{
  auto it = std::find(m_slices.begin(), m_slices.end(), slice);
  ASSERT(it != m_slices.end());
  if (it != m_slices.end())
    m_slices.erase(it);

  slice->setOwner(nullptr);
}

Slice* Slices::getByName(const std::string& name) const
{
  for (Slice* slice : *this) {
    if (slice->name() == name)
      return slice;
  }
  return nullptr;
}

Slice* Slices::getById(ObjectId id) const
{
  for (Slice* slice : *this) {
    if (slice->id() == id)
      return slice;
  }
  return nullptr;
}

} // namespace doc
