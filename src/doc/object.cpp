// Aseprite Document Library
// Copyright (C) 2019-present  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "doc/object.h"

#include "base/debug.h"
#include "base/exception.h"

#include <map>
#include <mutex>

namespace doc {

struct ObjectsStore {
  std::mutex mutex;
  ObjectId newId = 0;
  // TODO Profile this and see if an unordered_map is better
  std::map<ObjectId, Object*> objects;

  void add(ObjectId id, Object* obj)
  {
    auto it = objects.find(id);
    if (it != objects.end()) {
      if (it->second == obj)
        return; // Do nothing, already in store

      throw base::Exception("Trying to re-add an existing object in the store with ID %d", id);
    }
    objects.insert(std::make_pair(id, obj));
  }

  void remove(ObjectId id, Object* obj)
  {
    ASSERT(id != NullId);
    auto it = objects.find(id);
    if (it != objects.end())
      objects.erase(it);
  }
};

static ObjectsStore g_store;

ObjectId new_id()
{
  const std::lock_guard lock(g_store.mutex);
  return ++g_store.newId;
}

Object::Object(ObjectType type) : m_type(type)
{
}

Object::Object(const Object& other)
  : m_type(other.m_type)
  , m_id(NullId) // We don't copy the ID
  , m_version(0) // We don't copy the version
  , m_suspended(false)
{
}

Object::~Object()
{
  if (m_id)
    setId(NullId);
}

int Object::getMemSize() const
{
  return sizeof(Object);
}

const ObjectId Object::id() const
{
  // The first time the ID is requested, we generate it.
  if (!m_id) {
    const std::lock_guard lock(g_store.mutex);
    m_id = ++g_store.newId;

    // For non-suspended objects, we add the object in the store.  But
    // it can happen than a specific object (e.g. a CelData) requires
    // an ID, but it's suspended, because we're just serializing it to
    // be stored in the undo history.
    if (!m_suspended)
      g_store.add(m_id, const_cast<Object*>(this));
  }
  return m_id; // This can be NullId for suspended objects.
}

void Object::setIdInternal(const ObjectId id, const bool removeFromStore, const bool addToStore)
{
  const std::lock_guard lock(g_store.mutex);

  if (removeFromStore) {
    ASSERT(m_id != NullId);
    g_store.remove(m_id, this);
  }

  m_id = id;

  if (addToStore) {
    ASSERT(m_id != NullId);
    g_store.add(m_id, this);
  }
}

void Object::setId(const ObjectId id)
{
  setIdInternal(id, m_id != NullId, id != NullId && !m_suspended);
}

void Object::setVersion(ObjectVersion version)
{
  m_version = version;
}

void Object::suspendObject()
{
  if (m_suspended)
    return;

  setIdInternal(m_id, m_id != NullId, false);
  m_suspended = true;
}

void Object::restoreObject()
{
  if (!m_suspended)
    return;

  setIdInternal(m_id, false, m_id != NullId);
  m_suspended = false;
}

Object* get_object(ObjectId id)
{
  const std::lock_guard lock(g_store.mutex);
  auto it = g_store.objects.find(id);
  if (it != g_store.objects.end())
    return it->second;
  else
    return nullptr;
}

} // namespace doc
