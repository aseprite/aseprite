// Aseprite Document Library
// Copyright (c) 2020-2022 Igara Studio S.A.
// Copyright (c) 2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/slice.h"

#include "base/debug.h"
#include "doc/slices.h"

#include <limits>

namespace doc {

const gfx::Point SliceKey::NoPivot(std::numeric_limits<int32_t>::min(),
                                   std::numeric_limits<int32_t>::min());

SliceKey::SliceKey() : m_pivot(NoPivot)
{
}

SliceKey::SliceKey(const gfx::Rect& bounds, const gfx::Rect& center, const gfx::Point& pivot)
  : m_bounds(bounds)
  , m_center(center)
  , m_pivot(pivot)
{
}

Slice::Slice() : WithUserData(ObjectType::Slice), m_owner(nullptr), m_name("Slice")
{
}

Slice::Slice(const Slice& other)
  : WithUserData(other)
  , m_owner(nullptr)
  , m_name(other.m_name)
  , m_keys(other.m_keys)
{
}

Slice::~Slice()
{
  ASSERT(!m_owner);
}

int Slice::getMemSize() const
{
  return sizeof(*this) + sizeof(SliceKey) * m_keys.size();
}

void Slice::insert(const frame_t frame, const SliceKey& key)
{
  m_keys.insert(frame, std::make_unique<SliceKey>(key));
}

void Slice::remove(const frame_t frame)
{
  m_keys.remove(frame);
}

const SliceKey* Slice::getByFrame(const frame_t frame) const
{
  auto it = const_cast<Slice*>(this)->m_keys.getIterator(frame);
  if (it != m_keys.end())
    return it->value();
  else
    return nullptr;
}

Slice::iterator Slice::getIteratorByFrame(const frame_t frame) const
{
  return const_cast<Slice*>(this)->m_keys.getIterator(frame);
}

Sprite* Slice::sprite() const
{
  if (m_owner)
    return m_owner->sprite();
  else
    return nullptr;
}

void Slice::setOwner(Slices* owner)
{
  m_owner = owner;
}

void Slice::setName(const std::string& name)
{
  m_name = name;
}

} // namespace doc
