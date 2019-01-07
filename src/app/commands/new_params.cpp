// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/new_params.h"

#include "app/script/luacpp.h"

namespace app {

template<>
void Param<bool>::fromString(const std::string& value)
{
  m_value = (value == "1" || value == "true");
}

#ifdef ENABLE_SCRIPTING

template<>
void Param<bool>::fromLua(lua_State* L, int index)
{
  m_value = lua_toboolean(L, index);
}

void CommandWithNewParamsBase::onLoadParams(const Params& params)
{
  if (m_skipLoadParams) {
    m_skipLoadParams = false;
    return;
  }
  onResetValues();
  for (const auto& pair : params) {
    if (ParamBase* p = onGetParam(pair.first))
      p->fromString(pair.second);
  }
}

void CommandWithNewParamsBase::loadParamsFromLuaTable(lua_State* L, int index)
{
  onResetValues();
  if (lua_istable(L, index)) {
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
      if (const char* k = lua_tostring(L, -2)) {
        if (ParamBase* p = onGetParam(k))
          p->fromLua(L, -1);
      }
      lua_pop(L, 1);            // Pop the value, leave the key
    }
  }
  m_skipLoadParams = true;
}

#endif

} // namespace app
