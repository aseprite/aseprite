// Aseprite
// Copyright (C) 2023-2025  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/cmd/set_user_data_property.h"

#include "doc/with_user_data.h"

namespace app { namespace cmd {

SetUserDataProperty::SetUserDataProperty(doc::WithUserData* obj,
                                         const std::string& group,
                                         const std::string& field,
                                         doc::UserData::Variant&& newValue)
  : m_objId(obj->id())
  , m_group(group)
  , m_field(field)
  , m_value(std::move(newValue))
{
}

void SetUserDataProperty::onExecute()
{
  auto obj = doc::get<doc::WithUserData>(m_objId);
  auto& properties = obj->userData().properties(m_group);

  auto old = properties[m_field];
  doc::set_property_value(properties, m_field, std::move(m_value));
  std::swap(m_value, old);

  obj->incrementVersion();
}

void SetUserDataProperty::onUndo()
{
  onExecute();
}

}} // namespace app::cmd
