// Aseprite Document Library
// Copyright (c) 2001-2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_OBJECT_H_INCLUDED
#define DOC_OBJECT_H_INCLUDED
#pragma once

#include "doc/object_id.h"
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

    void incrementVersion() {
      ++m_version;
    }

    // Returns the approximate amount of memory (in bytes) which this
    // object use.
    virtual int getMemSize() const;

  private:
    ObjectType m_type;

    // Unique identifier for this object (it is assigned by
    // Objects class).
    mutable ObjectId m_id;

    ObjectVersion m_version;

    // Disable copy assignment
    Object& operator=(const Object&);
  };

  Object* get_object(ObjectId id);

  template<typename T>
  inline T* get(ObjectId id) {
    return static_cast<T*>(get_object(id));
  }

} // namespace doc

#endif
