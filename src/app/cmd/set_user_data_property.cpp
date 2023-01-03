// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_user_data_property.h"

#include "app/cmd.h"
#include "app/cmd/with_document.h"
#include "doc/object_id.h"
#include "doc/user_data.h"
#include "doc/with_user_data.h"

namespace app {
namespace cmd {

SetUserDataProperty::SetUserDataProperty(
  doc::WithUserData* obj,
  const std::string& group,
  const std::string& field,
  doc::UserData::Variant&& newValue)
  : m_objId(obj->id())
  , m_group(group)
  , m_field(field)
  , m_oldValue(obj->userData().properties(group)[m_field])
  , m_newValue(std::move(newValue))
{
}

void SetUserDataProperty::onExecute()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  obj->userData().properties(m_group)[m_field] = m_newValue;
  obj->incrementVersion();
}

void SetUserDataProperty::onUndo()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  obj->userData().properties(m_group)[m_field] = m_oldValue;
  obj->incrementVersion();
}

} // namespace cmd
} // namespace app
