// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"
#include "app/script/values.h"

#include <cstring>

#include "json11.hpp"

namespace app {
namespace script {

namespace {

struct Json { };
using JsonObj = json11::Json;
using JsonArrayIterator = JsonObj::array::const_iterator;
using JsonObjectIterator = JsonObj::object::const_iterator;

void push_json_value(lua_State* L, const JsonObj& value)
{
  switch (value.type()) {
    case json11::Json::NUL:
      lua_pushnil(L);
      break;
    case json11::Json::NUMBER:
      lua_pushnumber(L, value.number_value());
      break;
    case json11::Json::BOOL:
      lua_pushboolean(L, value.bool_value());
      break;
    case json11::Json::STRING:
      lua_pushstring(L, value.string_value().c_str());
      break;
    case json11::Json::ARRAY:
    case json11::Json::OBJECT:
      push_obj(L, value);
      break;
  }
}

JsonObj get_json_value(lua_State* L, int index)
{
  switch (lua_type(L, index)) {

    case LUA_TNONE:
    case LUA_TNIL:
      return JsonObj();

    case LUA_TBOOLEAN:
      return JsonObj(lua_toboolean(L, index) ? true: false);

    case LUA_TNUMBER:
      return JsonObj(lua_tonumber(L, index));

    case LUA_TSTRING:
      return JsonObj(lua_tostring(L, index));

    case LUA_TTABLE:
      if (is_array_table(L, index)) {
        JsonObj::array items;
        if (index < 0)
          --index;
        lua_pushnil(L);
        while (lua_next(L, index) != 0) {
          items.push_back(get_json_value(L, -1));
          lua_pop(L, 1);          // pop the value lua_next(), leave the key in the stack
        }
        return JsonObj(items);
      }
      else {
        JsonObj::object items;
        lua_pushnil(L);
        if (index < 0)
          --index;
        while (lua_next(L, index) != 0) {
          if (const char* k = lua_tostring(L, -2)) {
            items[k] = get_json_value(L, -1);
          }
          lua_pop(L, 1);          // pop the value lua_next(), leave the key in the stack
        }
        return JsonObj(items);
      }
      break;

    case LUA_TUSERDATA:
      // TODO convert rectangles, point, size, uuids?
      break;

  }
  return JsonObj();
}

int JsonObj_gc(lua_State* L)
{
  get_obj<JsonObj>(L, 1)->~JsonObj();
  return 0;
}

int JsonObj_eq(lua_State* L)
{
  auto a = get_obj<JsonObj>(L, 1);
  auto b = get_obj<JsonObj>(L, 2);
  return (*a == *b);
}

int JsonObj_len(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  switch (obj->type()) {
    case json11::Json::STRING:
      lua_pushinteger(L, obj->string_value().size());
      break;
    case json11::Json::ARRAY:
      lua_pushinteger(L, obj->array_items().size());
      break;
    case json11::Json::OBJECT:
      lua_pushinteger(L, obj->object_items().size());
      break;
    case json11::Json::NUL:
      lua_pushnil(L);
      break;
    default:
    case json11::Json::NUMBER:
    case json11::Json::BOOL:
      lua_pushinteger(L, 1);
      break;
  }
  return 1;
}

int JsonObj_index(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  if (obj->type() == json11::Json::OBJECT) {
    if (auto key = lua_tostring(L, 2)) {
      push_json_value(L, (*obj)[key]);
      return 1;
    }
  }
  else if (obj->type() == json11::Json::ARRAY) {
    auto i = lua_tointeger(L, 2) - 1; // Adjust to 0-based index
    push_json_value(L, (*obj)[i]);
    return 1;
  }
  return 0;
}

int JsonObj_newindex(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  if (obj->type() == json11::Json::OBJECT) {
    if (auto key = lua_tostring(L, 2)) {
      obj->set_object_item(key, get_json_value(L, 3));
    }
  }
  else if (obj->type() == json11::Json::ARRAY) {
    auto i = lua_tointeger(L, 2);
    // Adjust to 0-based index
    if (i > 0)
      obj->set_array_item(i-1, get_json_value(L, 3));
  }
  return 0;
}

int JsonObj_pairs_next(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  auto& it = *get_obj<JsonObjectIterator>(L, lua_upvalueindex(1));
  if (it == obj->object_items().end())
    return 0;
  lua_pushstring(L, (*it).first.c_str());
  push_json_value(L, (*it).second);
  ++it;
  return 2;
}

int JsonObj_ipairs_next(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  auto& it = *get_obj<JsonArrayIterator>(L, lua_upvalueindex(1));
  if (it == obj->array_items().end())
    return 0;
  lua_pushinteger(L, (it - obj->array_items().begin() + 1));
  push_json_value(L, (*it));
  ++it;
  return 0;
}

int JsonObj_pairs(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  if (obj->type() == json11::Json::OBJECT) {
    push_obj(L, obj->object_items().begin());
    lua_pushcclosure(L, JsonObj_pairs_next, 1);
    lua_pushvalue(L, 1); // Copy the same obj as the second return value
    return 2;
  }
  else if (obj->type() == json11::Json::ARRAY) {
    push_obj(L, obj->array_items().begin());
    lua_pushcclosure(L, JsonObj_ipairs_next, 1);
    lua_pushvalue(L, 1); // Copy the same obj as the second return value
    return 2;
  }
  return 0;
}

int JsonObj_tostring(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  lua_pushstring(L, obj->dump().c_str());
  return 1;
}

int JsonObjectIterator_gc(lua_State* L)
{
  get_obj<JsonObjectIterator>(L, 1)->~JsonObjectIterator();
  return 0;
}

int JsonArrayIterator_gc(lua_State* L)
{
  get_obj<JsonArrayIterator>(L, 1)->~JsonArrayIterator();
  return 0;
}

int Json_decode(lua_State* L)
{
  if (const char* s = lua_tostring(L, 1)) {
    std::string err;
    auto json = json11::Json::parse(s, std::strlen(s), err);
    if (!err.empty())
      return luaL_error(L, err.c_str());
    push_obj(L, json);
    return 1;
  }
  return 0;
}

int Json_encode(lua_State* L)
{
  // Encode a JsonObj, we deep copy it (create a deep copy)
  if (auto obj = may_get_obj<JsonObj>(L, 1)) {
    lua_pushstring(L, obj->dump().c_str());
    return 1;
  }
  // Encode a Lua table
  else if (lua_istable(L, 1)) {
    lua_pushstring(L, get_json_value(L, 1).dump().c_str());
    return 1;
  }
  return 0;
}

const luaL_Reg JsonObj_methods[] = {
  { "__gc",       JsonObj_gc },
  { "__eq",       JsonObj_eq },
  { "__len",      JsonObj_len },
  { "__index",    JsonObj_index },
  { "__newindex", JsonObj_newindex },
  { "__pairs",    JsonObj_pairs },
  { "__tostring", JsonObj_tostring },
  { nullptr,      nullptr }
};

const luaL_Reg JsonObjectIterator_methods[] = {
  { "__gc",       JsonObjectIterator_gc },
  { nullptr,      nullptr }
};

const luaL_Reg JsonArrayIterator_methods[] = {
  { "__gc",       JsonArrayIterator_gc },
  { nullptr,      nullptr }
};

const luaL_Reg Json_methods[] = {
  { "decode",     Json_decode },
  { "encode",     Json_encode },
  { nullptr,      nullptr }
};

} // anonymous namespace

DEF_MTNAME(Json);
DEF_MTNAME(JsonObj);
DEF_MTNAME(JsonObjectIterator);
DEF_MTNAME(JsonArrayIterator);

void register_json_object(lua_State* L)
{
  REG_CLASS(L, JsonObj);
  REG_CLASS(L, JsonObjectIterator);
  REG_CLASS(L, JsonArrayIterator);
  REG_CLASS(L, Json);

  lua_newtable(L);              // Create a table which will be the "json" object
  lua_pushvalue(L, -1);
  luaL_getmetatable(L, get_mtname<Json>());
  lua_setmetatable(L, -2);
  lua_setglobal(L, "json");
  lua_pop(L, 1);                // Pop json table
}

} // namespace script
} // namespace app
