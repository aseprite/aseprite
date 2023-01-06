// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_user_data_properties.h"

#include "doc/with_user_data.h"

namespace app {
namespace cmd {

SetUserDataProperties::SetUserDataProperties(
  doc::WithUserData* obj,
  const std::string& group,
  doc::UserData::Properties&& newProperties)
  : m_objId(obj->id())
  , m_group(group)
  , m_oldProperties(obj->userData().properties(group))
  , m_newProperties(std::move(newProperties))
{
}

void SetUserDataProperties::onExecute()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  obj->userData().properties(m_group) = m_newProperties;
  obj->incrementVersion();
}

void SetUserDataProperties::onUndo()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  obj->userData().properties(m_group) = m_oldProperties;
  obj->incrementVersion();
}

} // namespace cmd
} // namespace app
