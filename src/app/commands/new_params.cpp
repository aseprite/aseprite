// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/new_params.h"

#include "app/doc_exporter.h"
#include "app/script/luacpp.h"
#include "app/sprite_sheet_type.h"
#include "base/convert_to.h"
#include "base/string.h"

namespace app {

template<>
void Param<bool>::fromString(const std::string& value)
{
  setValue(value == "1" || value == "true");
}

template<>
void Param<int>::fromString(const std::string& value)
{
  setValue(base::convert_to<int>(value));
}

template<>
void Param<std::string>::fromString(const std::string& value)
{
  setValue(value);
}

template<>
void Param<app::SpriteSheetType>::fromString(const std::string& value)
{
  if (value == "horizontal")
    setValue(app::SpriteSheetType::Horizontal);
  else if (value == "vertical")
    setValue(app::SpriteSheetType::Vertical);
  else if (value == "rows")
    setValue(app::SpriteSheetType::Rows);
  else if (value == "columns")
    setValue(app::SpriteSheetType::Columns);
  else if (value == "packed")
    setValue(app::SpriteSheetType::Packed);
  else
    setValue(app::SpriteSheetType::None);
}

template<>
void Param<app::DocExporter::DataFormat>::fromString(const std::string& value)
{
  // JsonArray, json-array, json_array, etc.
  if (base::utf8_icmp(value, "JsonArray") == 0 ||
      base::utf8_icmp(value, "json-array") == 0 ||
      base::utf8_icmp(value, "json_array") == 0)
    setValue(app::DocExporter::JsonArrayDataFormat);
  else
    setValue(app::DocExporter::JsonHashDataFormat);
}

#ifdef ENABLE_SCRIPTING

template<>
void Param<bool>::fromLua(lua_State* L, int index)
{
  setValue(lua_toboolean(L, index));
}

template<>
void Param<int>::fromLua(lua_State* L, int index)
{
  setValue(lua_tointeger(L, index));
}

template<>
void Param<std::string>::fromLua(lua_State* L, int index)
{
  if (const char* s = lua_tostring(L, index))
    setValue(s);
  else
    setValue(std::string());
}

template<>
void Param<app::SpriteSheetType>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((app::SpriteSheetType)lua_tointeger(L, index));
}

template<>
void Param<app::DocExporter::DataFormat>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((app::DocExporter::DataFormat)lua_tointeger(L, index));
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
