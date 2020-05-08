// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_user_data.h"

#include "doc/with_user_data.h"

namespace app {
namespace cmd {

SetUserData::SetUserData(doc::WithUserData* obj, const doc::UserData& userData)
  : m_objId(obj->id())
  , m_oldUserData(obj->userData())
  , m_newUserData(userData)
{
}

void SetUserData::onExecute()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  obj->setUserData(m_newUserData);
  obj->incrementVersion();
}

void SetUserData::onUndo()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  obj->setUserData(m_oldUserData);
  obj->incrementVersion();
}

} // namespace cmd
} // namespace app
