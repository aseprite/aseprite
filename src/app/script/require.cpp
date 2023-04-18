// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/require.h"

#include "app/extensions.h"

#include <cstring>

namespace app {
namespace script {

static void eval_code(lua_State* L, const char* code)
{
  if (luaL_loadbuffer(L, code, std::strlen(code), "internal") ||
      lua_pcall(L, 0, 0, 0)) {
    // Error case
    const char* s = lua_tostring(L, -1);
    if (s)
      std::puts(s);

    lua_pop(L, 1);
  }
}

void custom_require_function(lua_State* L)
{
  eval_code(L, R"(
_PACKAGE_PATH_STACK = {}

local origRequire = require
function require(name)
  if _PLUGIN then
    local module = origRequire(_PLUGIN.name .. '/' .. name)
    if module then return module end
  end
  return origRequire(name)
end

local origLuaSearcher = package.searchers[2]
package.searchers[2] = function(name)
  if _PLUGIN then
    name = name:sub(#_PLUGIN.name+2)
  end
  return origLuaSearcher(name)
end
)");
}

SetPluginForRequire::SetPluginForRequire(lua_State* L, int pluginRef) : L(L)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, pluginRef);
  lua_setglobal(L, "_PLUGIN");

  // We could use all elements of package.config, but "/?.lua;" should work:
  //
  //   local templateSep = package.config:sub(3, 3)
  //   local substitutionPoint = package.config:sub(5, 5)
  //
  eval_code(L, R"(
table.insert(_PACKAGE_PATH_STACK, package.path)

local pathSep = package.config:sub(1, 1)
package.path = _PLUGIN.path .. pathSep .. '?.lua;' .. package.path
)");
}

SetPluginForRequire::~SetPluginForRequire()
{
  eval_code(L, R"(
package.path = table.remove(_PACKAGE_PATH_STACK)
_PLUGIN_OLDPATH = nil
_PLUGIN = nil
)");
}

SetScriptForRequire::SetScriptForRequire(lua_State* L, const char* path) : L(L)
{
  lua_pushstring(L, path);
  lua_setglobal(L, "_SCRIPT_PATH");
  eval_code(L, R"(
table.insert(_PACKAGE_PATH_STACK, package.path)

local pathSep = package.config:sub(1, 1)
package.path = _SCRIPT_PATH .. pathSep .. '?.lua;' .. package.path
)");
}

SetScriptForRequire::~SetScriptForRequire()
{
  eval_code(L, R"(
package.path = table.remove(_PACKAGE_PATH_STACK)
_SCRIPT_PATH = nil
)");
}

} // namespace script
} // namespace app
