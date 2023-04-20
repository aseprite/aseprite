// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_REGISTRY_H_INCLUDED
#define APP_SCRIPT_REGISTRY_H_INCLUDED
#pragma once

#include "app/script/luacpp.h"

namespace app {
namespace script {

// Saves a value from the Lua stack into the registry (LUA_REGISTRYINDEX)
class RegistryRef {
public:
  RegistryRef(int ref = LUA_REFNIL) : m_ref(ref) { }
  ~RegistryRef() { ASSERT(m_ref == LUA_REFNIL); }

  RegistryRef(const RegistryRef&) = delete;
  RegistryRef& operator=(const RegistryRef&) = delete;

  RegistryRef(RegistryRef&& other) {
    std::swap(m_ref, other.m_ref);
  }
  RegistryRef& operator=(RegistryRef&& other) {
    std::swap(m_ref, other.m_ref);
    return *this;
  }

  void ref(lua_State* L) {
    unref(L);
    m_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  void unref(lua_State* L) {
    if (m_ref != LUA_REFNIL) {
      luaL_unref(L, LUA_REGISTRYINDEX, m_ref);
      m_ref = LUA_REFNIL;
    }
  }

  bool get(lua_State* L) {
    if (m_ref != LUA_REFNIL) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, m_ref);
      return true;
    }
    else
      return false;
  }

private:
  int m_ref = LUA_REFNIL;
};

} // namespace script
} // namespace app

#endif
