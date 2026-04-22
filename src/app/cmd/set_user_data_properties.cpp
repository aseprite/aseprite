// Aseprite
// Copyright (C) 2023-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_user_data_properties.h"

#include "doc/with_user_data.h"

namespace app { namespace cmd {

SetUserDataProperties::SetUserDataProperties(doc::WithUserData* obj,
                                             const std::string& group,
                                             doc::UserData::Properties&& newProperties)
  : m_objId(obj->id())
  , m_group(group)
  , m_properties(std::move(newProperties))
{
}

void SetUserDataProperties::onExecute()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  auto& properties = obj->userData().properties(m_group);

  auto old = properties;
  properties = m_properties;
  std::swap(m_properties, old);

  obj->incrementVersion();
}

void SetUserDataProperties::onUndo()
{
  onExecute();
}

}} // namespace app::cmd
