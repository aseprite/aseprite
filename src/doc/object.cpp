// Aseprite Document Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/object.h"

namespace doc {

static ObjectId newId = 0;

Object::Object(ObjectType type)
  : m_type(type)
  , m_id(++newId)
{
}

Object::~Object()
{
}

int Object::getMemSize() const
{
  return sizeof(Object);
}

} // namespace doc
