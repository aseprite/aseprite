// Aseprite Document Library
// Copyright (c) 2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_OBJECT_H_INCLUDED
#define DOC_OBJECT_H_INCLUDED
#pragma once

#include "doc/object_id.h"

namespace doc {

  class Object {
  public:
    Object();
    virtual ~Object();

    const ObjectId id() const { return m_id; }

    void setId(ObjectId id) { m_id = id; }

  private:
    // Unique identifier for this object (it is assigned by
    // Objects class).
    ObjectId m_id;
  };

} // namespace doc

#endif
