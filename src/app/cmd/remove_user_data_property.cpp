// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/remove_user_data_property.h"

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "doc/object_id.h"
#include "doc/user_data.h"
#include "doc/with_user_data.h"

namespace app {
namespace cmd {

RemoveUserDataProperty::RemoveUserDataProperty(
  doc::WithUserData* obj,
  const std::string& group,
  const std::string& field)
  : m_objId(obj->id())
  , m_group(group)
  , m_field(field)
  , m_oldVariant(obj->userData().properties(m_group)[m_field])
{
}

void RemoveUserDataProperty::onExecute()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  auto& properties = obj->userData().properties(m_group);
  auto it = properties.find(m_field);
  ASSERT(it != properties.end());
  if (it != properties.end()) {
    properties.erase(it);
    obj->incrementVersion();
  }
}

void RemoveUserDataProperty::onUndo()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  obj->userData().properties(m_group)[m_field] = m_oldVariant;
  obj->incrementVersion();
}

} // namespace cmd
} // namespace app
