// Aseprite Document Library
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

namespace doc {

const gfx::Point SliceKey::NoPivot(std::numeric_limits<int32_t>::min(),
                                   std::numeric_limits<int32_t>::min());

SliceKey::SliceKey()
  : m_pivot(NoPivot)
{
}

SliceKey::SliceKey(const gfx::Rect& bounds,
                   const gfx::Rect& center,
                   const gfx::Point& pivot)
  : m_bounds(bounds)
  , m_center(center)
  , m_pivot(pivot)
{
}

Slice::Slice()
  : WithUserData(ObjectType::Slice)
  , m_owner(nullptr)
  , m_name("Slice")
{
  // TODO default color should be configurable
  userData().setColor(doc::rgba(0, 0, 255, 255));
}

Slice::~Slice()
{
  ASSERT(!m_owner);
}

int Slice::getMemSize() const
{
  return sizeof(*this) + sizeof(SliceKey)*m_keys.size();
}

void Slice::insert(const frame_t frame, const SliceKey& key)
{
  m_keys.insert(frame, new SliceKey(key));
}

void Slice::remove(const frame_t frame)
{
  delete m_keys.remove(frame);
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

void Slice::setOwner(Slices* owner)
{
  m_owner = owner;
}

void Slice::setName(const std::string& name)
{
  m_name = name;
}

} // namespace doc
