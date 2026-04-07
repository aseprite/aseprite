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

#include "nlohmann/json.hpp"
#include <memory>

using namespace nlohmann;

namespace app { namespace script {

namespace {

struct Json {};

struct JsonObj {
  std::shared_ptr<json> root;
  json::json_pointer ptr;

  JsonObj(std::shared_ptr<json> root, json::json_pointer ptr)
    : root(std::move(root))
    , ptr(std::move(ptr))
  {
  }

  json& get() { return root->at(ptr); }
  const json& get() const { return root->at(ptr); }
};

struct JsonIterator {
  JsonObj parent;
  json::iterator it;

  JsonIterator(const JsonObj& parent, const json::iterator& it) : parent(parent), it(it) {}
};

void push_json_value(lua_State* L, const JsonObj& obj)
{
  const auto& value = obj.get();
  switch (value.type()) {
    case json::value_t::null:            lua_pushnil(L); break;
    case json::value_t::number_integer:  lua_pushinteger(L, value.get<lua_Integer>()); break;
    case json::value_t::number_unsigned:
    case json::value_t::number_float:    lua_pushnumber(L, value.get<lua_Number>()); break;
    case json::value_t::boolean:         lua_pushboolean(L, value.get<bool>()); break;
    case json::value_t::string:          lua_pushstring(L, value.get<std::string>().c_str()); break;
    case json::value_t::array:
    case json::value_t::object:          push_obj(L, obj); break;
    default:                             ASSERT(false);
  }
}

json get_json_value(lua_State* L, int index)
{
  switch (lua_type(L, index)) {
    case LUA_TNONE:
    case LUA_TNIL:     return json();

    case LUA_TBOOLEAN: return json(lua_toboolean(L, index) ? true : false);

    case LUA_TNUMBER:  {
      if (lua_isinteger(L, index)) {
        return json(lua_tointeger(L, index));
      }
      return json(lua_tonumber(L, index));
    }

    case LUA_TSTRING: return json(lua_tostring(L, index));

    case LUA_TTABLE:
      if (is_array_table(L, index)) {
        json items = json::array();
        if (index < 0)
          --index;
        lua_pushnil(L);
        while (lua_next(L, index) != 0) {
          items.push_back(get_json_value(L, -1));
          lua_pop(L, 1); // pop the value lua_next(), leave the key in the stack
        }
        return items;
      }
      else {
        json items = json::object();
        lua_pushnil(L);
        if (index < 0)
          --index;
        while (lua_next(L, index) != 0) {
          if (const char* k = lua_tostring(L, -2)) {
            items[k] = get_json_value(L, -1);
          }
          lua_pop(L, 1); // pop the value lua_next(), leave the key in the stack
        }
        return items;
      }
      break;

    case LUA_TUSERDATA:
      // TODO convert rectangles, point, size, uuids?
      break;
  }
  return json();
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
  return (a->get() == b->get());
}

int JsonObj_len(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  auto& value = obj->get();

  switch (value.type()) {
    case json::value_t::string:
    case json::value_t::array:
    case json::value_t::object: lua_pushinteger(L, value.size()); break;
    case json::value_t::null:   lua_pushnil(L); break;
    default:                    lua_pushinteger(L, 1); break;
  }
  return 1;
}

int JsonObj_index(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  auto& value = obj->get();
  if (value.type() == json::value_t::object) {
    if (const auto* key = lua_tostring(L, 2); value.contains(key)) {
      push_json_value(L, JsonObj(obj->root, obj->ptr / key));
      return 1;
    }
  }
  else if (value.type() == json::value_t::array) {
    auto i = lua_tointeger(L, 2) - 1; // Adjust to 0-based index
    if (i >= 0 && i < value.size()) {
      push_json_value(L, JsonObj(obj->root, obj->ptr / i));
      return 1;
    }
  }
  return 0;
}

int JsonObj_newindex(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  auto& value = obj->get();
  if (value.type() == json::value_t::object) {
    if (auto key = lua_tostring(L, 2)) {
      value[key] = get_json_value(L, 3);
    }
  }
  else if (value.type() == json::value_t::array) {
    auto i = lua_tointeger(L, 2);
    // Adjust to 0-based index
    if (i > 0)
      value[i - 1] = get_json_value(L, 3);
  }
  else {
    ASSERT(false);
  }
  return 0;
}

int JsonObj_pairs_next(lua_State* L)
{
  auto& itObj = *get_obj<JsonIterator>(L, lua_upvalueindex(1));
  auto& j = itObj.parent.get();
  if (itObj.it == j.end())
    return 0;
  lua_pushstring(L, itObj.it.key().c_str());
  const JsonObj child(itObj.parent.root, itObj.parent.ptr / itObj.it.key());
  push_json_value(L, child);
  ++itObj.it;
  return 2;
}

int JsonObj_ipairs_next(lua_State* L)
{
  auto& itObj = *get_obj<JsonIterator>(L, lua_upvalueindex(1));
  auto& j = itObj.parent.get();
  if (itObj.it == j.end())
    return 0;
  int index = std::distance(j.begin(), itObj.it);
  lua_pushinteger(L, index + 1);
  const JsonObj child(itObj.parent.root, itObj.parent.ptr / index);
  push_json_value(L, child);
  ++itObj.it;
  return 2;
}

int JsonObj_pairs(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  auto& value = obj->get();
  if (value.type() == json::value_t::object) {
    push_obj(L, JsonIterator(*obj, value.begin()));
    lua_pushcclosure(L, JsonObj_pairs_next, 1);
    lua_pushvalue(L, 1); // Copy the same obj as the second return value
    return 2;
  }
  else if (value.type() == json::value_t::array) {
    push_obj(L, JsonIterator(*obj, value.begin()));
    lua_pushcclosure(L, JsonObj_ipairs_next, 1);
    lua_pushvalue(L, 1); // Copy the same obj as the second return value
    return 2;
  }
  return 0;
}

int JsonObj_tostring(lua_State* L)
{
  auto obj = get_obj<JsonObj>(L, 1);
  lua_pushstring(L, obj->get().dump().c_str());
  return 1;
}

int JsonIterator_gc(lua_State* L)
{
  get_obj<JsonIterator>(L, 1)->~JsonIterator();
  return 0;
}

int Json_decode(lua_State* L)
{
  if (const char* s = lua_tostring(L, 1)) {
    try {
      auto j = std::make_shared<json>(json::parse(s));
      push_obj(L, JsonObj(j, json::json_pointer()));
      return 1;
    }
    catch (std::exception& e) {
      return luaL_error(L, e.what());
    }
  }
  return 0;
}

int Json_encode(lua_State* L)
{
  // Encode a JsonObj, we deep copy it (create a deep copy)
  if (auto obj = may_get_obj<JsonObj>(L, 1)) {
    lua_pushstring(L, obj->get().dump().c_str());
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
  { "__gc",       JsonObj_gc       },
  { "__eq",       JsonObj_eq       },
  { "__len",      JsonObj_len      },
  { "__index",    JsonObj_index    },
  { "__newindex", JsonObj_newindex },
  { "__pairs",    JsonObj_pairs    },
  { "__tostring", JsonObj_tostring },
  { nullptr,      nullptr          }
};

const luaL_Reg JsonIterator_methods[] = {
  { "__gc",  JsonIterator_gc },
  { nullptr, nullptr         }
};

const luaL_Reg Json_methods[] = {
  { "decode", Json_decode },
  { "encode", Json_encode },
  { nullptr,  nullptr     }
};

} // anonymous namespace

DEF_MTNAME(Json);
DEF_MTNAME(JsonObj);
DEF_MTNAME(JsonIterator);

void register_json_object(lua_State* L)
{
  REG_CLASS(L, JsonObj);
  REG_CLASS(L, JsonIterator);
  REG_CLASS(L, Json);

  lua_newtable(L); // Create a table which will be the "json" object
  lua_pushvalue(L, -1);
  luaL_getmetatable(L, get_mtname<Json>());
  lua_setmetatable(L, -2);
  lua_setglobal(L, "json");
  lua_pop(L, 1); // Pop json table
}

}} // namespace app::script
