// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"
#include "doc/color.h"

namespace app {
namespace script {

namespace {

int PixelColor_rgba(lua_State* L)
{
  const int r = lua_tointeger(L, 1);
  const int g = lua_tointeger(L, 2);
  const int b = lua_tointeger(L, 3);
  const int a = (lua_isnoneornil(L, 4) ? 255: lua_tointeger(L, 4));
  lua_pushinteger(L, doc::rgba(r, g, b, a));
  return 1;
}

int PixelColor_rgbaR(lua_State* L)
{
  lua_pushinteger(L, doc::rgba_getr(lua_tointeger(L, 1)));
  return 1;
}

int PixelColor_rgbaG(lua_State* L)
{
  lua_pushinteger(L, doc::rgba_getg(lua_tointeger(L, 1)));
  return 1;
}

int PixelColor_rgbaB(lua_State* L)
{
  lua_pushinteger(L, doc::rgba_getb(lua_tointeger(L, 1)));
  return 1;
}

int PixelColor_rgbaA(lua_State* L)
{
  lua_pushinteger(L, doc::rgba_geta(lua_tointeger(L, 1)));
  return 1;
}

int PixelColor_graya(lua_State* L)
{
  int v = lua_tointeger(L, 1);
  int a = (lua_isnoneornil(L, 2) ? 255: lua_tointeger(L, 2));
  lua_pushinteger(L, doc::graya(v, a));
  return 1;
}

int PixelColor_grayaV(lua_State* L)
{
  lua_pushinteger(L, doc::graya_getv(lua_tointeger(L, 1)));
  return 1;
}

int PixelColor_grayaA(lua_State* L)
{
  lua_pushinteger(L, doc::graya_geta(lua_tointeger(L, 1)));
  return 1;
}

const luaL_Reg PixelColor_methods[] = {
  { "rgba", PixelColor_rgba },
  { "rgbaR", PixelColor_rgbaR },
  { "rgbaG", PixelColor_rgbaG },
  { "rgbaB", PixelColor_rgbaB },
  { "rgbaA", PixelColor_rgbaA },
  { "graya", PixelColor_graya },
  { "grayaV", PixelColor_grayaV },
  { "grayaA", PixelColor_grayaA },
  { nullptr, nullptr }
};

} // anonymous namespace

void register_app_pixel_color_object(lua_State* L)
{
  lua_getglobal(L, "app");
  lua_newtable(L);              // New table for pixelColor
  lua_pushstring(L, "pixelColor");
  lua_pushvalue(L, -2);         // Copy table
  lua_rawset(L, -4);
  luaL_setfuncs(L, PixelColor_methods, 0);
  lua_pop(L, 2);                // Pop table & app global
}

} // namespace script
} // namespace app
