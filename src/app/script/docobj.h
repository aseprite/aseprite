// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_DOCOBJ_H_INCLUDED
#define APP_SCRIPT_DOCOBJ_H_INCLUDED
#pragma once

#include "app/script/luacpp.h"
#include "doc/object.h"
#include "fmt/format.h"

namespace app { namespace script {

// Functions to push/get doc:: objects from Lua stack with doc::ObjectId.
// This can be used to avoid crashes accesing raw pointers.

template<typename T>
void push_docobj(lua_State* L, doc::ObjectId id)
{
  new (lua_newuserdata(L, sizeof(doc::ObjectId))) doc::ObjectId(id);
  luaL_getmetatable(L, get_mtname<T>());
  lua_setmetatable(L, -2);
}

template<typename T>
void push_docobj(lua_State* L, const T* obj)
{
  push_docobj<T>(L, obj->id());
}

template<typename T>
T* may_get_docobj(lua_State* L, int index)
{
  const doc::ObjectId* id = (doc::ObjectId*)luaL_testudata(L, index, get_mtname<T>());
  if (id) {
    T* obj = doc::get<T>(*id);
    if (obj)
      return obj;
  }
  return nullptr;
}

template<typename T>
T* check_docobj(lua_State* L, T* obj)
{
  if (obj)
    return obj;
  else {
    luaL_traceback(L, L, fmt::format("Tried to access a deleted '{}'", get_mtname<T>()).c_str(), 1);
    const char* traceback = lua_tostring(L, -1);
    luaL_error(L, traceback ? traceback : "");
    ASSERT(false); // unreachable code
    return nullptr;
  }
}

template<typename T>
T* get_docobj(lua_State* L, int index)
{
  return check_docobj<T>(L, may_get_docobj<T>(L, index));
}

}} // namespace app::script

#endif
