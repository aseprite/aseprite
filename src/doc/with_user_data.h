// Aseprite Document Library
// Copyright (c) 2001-2017 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef DOC_WITH_USER_DATA_H_INCLUDED
#define DOC_WITH_USER_DATA_H_INCLUDED
#pragma once

#include "doc/object.h"
#include "doc/user_data.h"

namespace doc {

class WithUserData : public Object {
public:
  WithUserData(ObjectType type) : Object(type) {}

  const UserData& userData() const { return m_userData; }
  UserData& userData() { return m_userData; }

  void setUserData(const UserData& userData) { m_userData = userData; }

private:
  UserData m_userData;
};

} // namespace doc

#endif
