// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/engine.h"

#include "app/app.h"
#include "app/console.h"
#include "app/script/luacpp.h"
#include "base/fstream_path.h"
#include "doc/color_mode.h"

#include <fstream>
#include <sstream>

namespace app {
namespace script {

namespace {

int print(lua_State* L)
{
  std::string output;
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    lua_pushvalue(L, -1);  // function to be called
    lua_pushvalue(L, i);   // value to print
    lua_call(L, 1, 1);
    size_t l;
    const char* s = lua_tolstring(L, -1, &l);  // get result
    if (s == nullptr)
      return luaL_error(L, "'tostring' must return a string to 'print'");
    if (i > 1)
      output.push_back('\t');
    output.insert(output.size(), s, l);
    lua_pop(L, 1);  // pop result
  }
  if (!output.empty()) {
    std::printf("%s\n", output.c_str());
    std::fflush(stdout);
    if (App::instance()->isGui())
      Console().printf("%s\n", output.c_str());
  }
  return 0;
}

int unsupported(lua_State* L)
{
  // debug.getinfo(1, "n").name
  lua_getglobal(L, "debug");
  lua_getfield(L, -1, "getinfo");
  lua_remove(L, -2);
  lua_pushinteger(L, 1);
  lua_pushstring(L, "n");
  lua_call(L, 2, 1);
  lua_getfield(L, -1, "name");
  return luaL_error(L, "unsupported function '%s'",
                    lua_tostring(L, -1));
}

} // anonymous namespace

void register_app_object(lua_State* L);
void register_app_pixel_color_object(lua_State* L);

void register_image_class(lua_State* L);
void register_point_class(lua_State* L);
void register_rect_class(lua_State* L);
void register_selection_class(lua_State* L);
void register_site_class(lua_State* L);
void register_size_class(lua_State* L);
void register_sprite_class(lua_State* L);

Engine::Engine()
  : L(luaL_newstate())
  , m_delegate(nullptr)
  , m_printLastResult(false)
{
  int top = lua_gettop(L);

  // Standard Lua libraries
  luaL_requiref(L, LUA_GNAME, luaopen_base, 1);
  luaL_requiref(L, LUA_COLIBNAME, luaopen_coroutine, 1);
  luaL_requiref(L, LUA_TABLIBNAME, luaopen_table, 1);
  luaL_requiref(L, LUA_OSLIBNAME, luaopen_os, 1);
  luaL_requiref(L, LUA_STRLIBNAME, luaopen_string, 1);
  luaL_requiref(L, LUA_MATHLIBNAME, luaopen_math, 1);
  luaL_requiref(L, LUA_UTF8LIBNAME, luaopen_utf8, 1);
  luaL_requiref(L, LUA_DBLIBNAME, luaopen_debug, 1);
  lua_pop(L, 8);

  // Overwrite Lua functions
  lua_register(L, "print", print);

  lua_getglobal(L, "os");
  for (const char* name : { "execute", "remove", "rename", "exit", "tmpname" }) {
    lua_pushcfunction(L, unsupported);
    lua_setfield(L, -2, name);
  }
  lua_pop(L, 1);

  // Generic code used by metatables
  run_mt_index_code(L);

  // Register global app object
  register_app_object(L);
  register_app_pixel_color_object(L);

  // Register constants
  lua_newtable(L);
  lua_pushvalue(L, -1);
  lua_setglobal(L, "ColorMode");
  setfield_integer(L, "RGB", doc::ColorMode::RGB);
  setfield_integer(L, "GRAYSCALE", doc::ColorMode::GRAYSCALE);
  setfield_integer(L, "INDEXED", doc::ColorMode::INDEXED);
  lua_pop(L, 1);

  // Register classes/prototypes
  register_image_class(L);
  register_point_class(L);
  register_rect_class(L);
  register_selection_class(L);
  register_site_class(L);
  register_size_class(L);
  register_sprite_class(L);

  // Check that we have a clean start (without dirty in the stack)
  ASSERT(lua_gettop(L) == top);
}

Engine::~Engine()
{
  lua_close(L);
}

void Engine::printLastResult()
{
  m_printLastResult = true;
}

bool Engine::evalCode(const std::string& code,
                      const std::string& filename)
{
  bool ok = true;
  if (luaL_loadbuffer(L, code.c_str(), code.size(), filename.c_str()) ||
      lua_pcall(L, 0, 1, 0)) {
    // Error case
    std::string err;
    const char* s = lua_tostring(L, -1);
    if (s)
      onConsolePrint(s);
    ok = false;
  }
  else {
    // Code was executed correctly
    if (m_printLastResult) {
      if (!lua_isnone(L, -1)) {
        const char* result = lua_tostring(L, -1);
        if (result)
          onConsolePrint(result);
      }
    }
  }
  lua_pop(L, 1);
  return ok;
}

bool Engine::evalFile(const std::string& filename)
{
  std::stringstream buf;
  {
    std::ifstream s(FSTREAM_PATH(filename));
    buf << s.rdbuf();
  }
  return evalCode(buf.str(), filename);
}

void Engine::onConsolePrint(const char* text)
{
  if (m_delegate)
    m_delegate->onConsolePrint(text);
  else
    std::printf("%s\n", text);
}

} // namespace script
} // namespace app
