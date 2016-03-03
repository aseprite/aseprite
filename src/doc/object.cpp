// Aseprite Document Library
// Copyright (c) 2001-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/object.h"

#include "base/debug.h"
#include "base/mutex.h"
#include "base/scoped_lock.h"

#include <map>

namespace doc {

static base::mutex mutex;
static ObjectId newId = 0;
// TODO Profile this and see if an unordered_map is better
static std::map<ObjectId, Object*> objects;

Object::Object(ObjectType type)
  : m_type(type)
  , m_id(0)
  , m_version(0)
{
}

Object::Object(const Object& other)
  : m_type(other.m_type)
  , m_id(0) // We don't copy the ID
  , m_version(0) // We don't copy the version
{
}

Object::~Object()
{
  if (m_id)
    setId(0);
}

int Object::getMemSize() const
{
  return sizeof(Object);
}

const ObjectId Object::id() const
{
  // The first time the ID is request, we store the object in the
  // "objects" hash table.
  if (!m_id) {
    base::scoped_unlock hold(mutex);
    m_id = ++newId;
    objects.insert(std::make_pair(m_id, const_cast<Object*>(this)));
  }
  return m_id;
}

void Object::setId(ObjectId id)
{
  base::scoped_unlock hold(mutex);

  if (m_id) {
    auto it = objects.find(m_id);
    ASSERT(it != objects.end());
    ASSERT(it->second == this);
    if (it != objects.end())
      objects.erase(it);
  }

  m_id = id;

  if (m_id) {
    ASSERT(objects.find(m_id) == objects.end());
    objects.insert(std::make_pair(m_id, this));
  }
}

void Object::setVersion(ObjectVersion version)
{
  m_version = version;
}

Object* get_object(ObjectId id)
{
  base::scoped_unlock hold(mutex);
  auto it = objects.find(id);
  if (it != objects.end())
    return it->second;
  else
    return nullptr;
}

} // namespace doc
