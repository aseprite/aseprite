// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_OBJECT_H_INCLUDED
#define DOC_OBJECT_H_INCLUDED
#pragma once

#include "doc/object_id.h"
#include "doc/object_type.h"

namespace doc {

  class Object {
  public:
    Object(ObjectType type);
    virtual ~Object();

    const ObjectType type() const { return m_type; }
    const ObjectId id() const { return m_id; }

    void setId(ObjectId id) { m_id = id; }

    // Returns the approximate amount of memory (in bytes) which this
    // object use.
    virtual int getMemSize() const;

  private:
    ObjectType m_type;

    // Unique identifier for this object (it is assigned by
    // Objects class).
    ObjectId m_id;
  };

} // namespace doc

#endif
