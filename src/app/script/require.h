// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_REQUIRE_H_INCLUDED
#define APP_SCRIPT_REQUIRE_H_INCLUDED
#pragma once

#include "app/script/luacpp.h"

#include <string>

namespace app {
namespace script {

class SetPluginForRequire {
public:
  SetPluginForRequire(lua_State* L, int pluginRef);
  ~SetPluginForRequire();
private:
  lua_State* L;
};

class SetScriptForRequire {
public:
  SetScriptForRequire(lua_State* L, const char* path);
  ~SetScriptForRequire();
private:
  lua_State* L;
};

void custom_require_function(lua_State* L);

} // namespace script
} // namespace app

#endif
