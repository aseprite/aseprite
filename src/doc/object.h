// Aseprite Document Library
// Copyright (c) 2025  Igara Studio S.A.
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_OBJECT_H_INCLUDED
#define DOC_OBJECT_H_INCLUDED
#pragma once

#include "doc/object_id.h"
#include "doc/object_ids.h"
#include "doc/object_type.h"
#include "doc/object_version.h"

namespace doc {

class Object {
public:
  Object(ObjectType type);
  Object(const Object& other);
  virtual ~Object();

  const ObjectType type() const { return m_type; }
  const ObjectId id() const;
  const ObjectVersion version() const { return m_version; }

  void setId(ObjectId id);
  void setVersion(ObjectVersion version);

  void incrementVersion() { ++m_version; }

  // Returns the approximate amount of memory (in bytes) which this
  // object use.
  virtual int getMemSize() const;

  // Removes or restore the ID of this object (and all its children)
  // to keep this object in a "suspended" state (e.g. in the undo
  // history) or to recover the object from a suspended state.
  virtual void suspendObject();
  virtual void restoreObject();

private:
  ObjectType m_type;

  // Unique identifier for this object (it is assigned by
  // Objects class).
  mutable ObjectId m_id = NullId;

  // ID saved when the objects is "deleted" but stored in the undo
  // history. It's a way to save the previous ID and restore it.
  ObjectId m_suspendedId = NullId;

  ObjectVersion m_version = 0;

  // Disable copy assignment
  Object& operator=(const Object&);
};

Object* get_object(ObjectId id);

template<typename T>
inline T* get(ObjectId id)
{
  return static_cast<T*>(get_object(id));
}

} // namespace doc

#endif
