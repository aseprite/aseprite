// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"
#include "fmt/format.h"
#include "gfx/size.h"

#include <cmath>

namespace app {
namespace script {

namespace {

gfx::Size Size_new(lua_State* L, int index)
{
  gfx::Size sz(0, 0);
  // Copy other size
  if (auto sz2 = may_get_obj<gfx::Size>(L, index)) {
    sz = *sz2;
  }
  // Convert {x=int,y=int} or {int,int} into a Size
  else if (lua_istable(L, index)) {
    int type = lua_getfield(L, index, "width");
    if (VALID_LUATYPE(type)) {
      lua_getfield(L, index, "height");
      sz.w = lua_tointeger(L, -2);
      sz.h = lua_tointeger(L, -1);
      lua_pop(L, 2);
    }
    else {
      lua_pop(L, 1);
      type = lua_getfield(L, index, "w");
      if (VALID_LUATYPE(type)) {
        lua_getfield(L, index, "h");
        sz.w = lua_tointeger(L, -2);
        sz.h = lua_tointeger(L, -1);
        lua_pop(L, 2);
      }
      else {
        lua_pop(L, 1);
        lua_geti(L, index, 1);
        lua_geti(L, index, 2);
        sz.w = lua_tointeger(L, -2);
        sz.h = lua_tointeger(L, -1);
        lua_pop(L, 2);
      }
    }
  }
  else {
    sz.w = lua_tointeger(L, index);
    sz.h = lua_tointeger(L, index+1);
  }
  return sz;
}

int Size_new(lua_State* L)
{
  push_obj(L, Size_new(L, 1));
  return 1;
}

int Size_gc(lua_State* L)
{
  get_obj<gfx::Size>(L, 1)->~SizeT();
  return 0;
}

int Size_eq(lua_State* L)
{
  const auto a = get_obj<gfx::Size>(L, 1);
  const auto b = get_obj<gfx::Size>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

int Size_tostring(lua_State* L)
{
  const auto sz = get_obj<gfx::Size>(L, 1);
  lua_pushstring(L, fmt::format("Size{{ width={}, height={} }}",
                                sz->w, sz->h).c_str());
  return 1;
}

int Size_unm(lua_State* L)
{
  const auto sz = get_obj<gfx::Size>(L, 1);
  push_obj(L, -(*sz));
  return 1;
}

int Size_add(lua_State* L)
{
  gfx::Size result(0, 0);
  if (lua_isuserdata(L, 1))
    result += *get_obj<gfx::Size>(L, 1);
  else
    result += lua_tointeger(L, 1);
  if (lua_isuserdata(L, 2))
    result += *get_obj<gfx::Size>(L, 2);
  else
    result += lua_tointeger(L, 2);
  push_obj(L, result);
  return 1;
}

int Size_sub(lua_State* L)
{
  gfx::Size result = *get_obj<gfx::Size>(L, 1);
  if (lua_isuserdata(L, 2))
    result -= *get_obj<gfx::Size>(L, 2);
  else
    result -= lua_tointeger(L, 2);
  push_obj(L, result);
  return 1;
}

int Size_mul(lua_State* L)
{
  gfx::Size result = *get_obj<gfx::Size>(L, 1);
  result *= lua_tointeger(L, 2);
  push_obj(L, result);
  return 1;
}

int Size_div(lua_State* L)
{
  gfx::Size result = *get_obj<gfx::Size>(L, 1);
  const int value = lua_tointeger(L, 2);
  if (value == 0)
    return luaL_error(L, "attempt to divide by zero");
  result /= value;
  push_obj(L, result);
  return 1;
}

int Size_mod(lua_State* L)
{
  gfx::Size result = *get_obj<gfx::Size>(L, 1);
  const int value = lua_tointeger(L, 2);
  if (value == 0)
    return luaL_error(L, "attempt to divide by zero");
  result.w %= value;
  result.h %= value;
  push_obj(L, result);
  return 1;
}

int Size_pow(lua_State* L)
{
  gfx::Size result = *get_obj<gfx::Size>(L, 1);
  const int value = lua_tointeger(L, 2);
  result.w = std::pow(result.w, value);
  result.h = std::pow(result.h, value);
  push_obj(L, result);
  return 1;
}

int Size_union(lua_State* L)
{
  const auto a = get_obj<gfx::Size>(L, 1);
  const auto b = get_obj<gfx::Size>(L, 2);
  push_obj(L, a->createUnion(*b));
  return 1;
}

int Size_get_width(lua_State* L)
{
  const auto sz = get_obj<gfx::Size>(L, 1);
  ASSERT(sz);
  lua_pushinteger(L, sz->w);
  return 1;
}

int Size_get_height(lua_State* L)
{
  const auto sz = get_obj<gfx::Size>(L, 1);
  ASSERT(sz);
  lua_pushinteger(L, sz->h);
  return 1;
}

int Size_set_width(lua_State* L)
{
  auto sz = get_obj<gfx::Size>(L, 1);
  ASSERT(sz);
  sz->w = lua_tointeger(L, 2);
  return 0;
}

int Size_set_height(lua_State* L)
{
  auto sz = get_obj<gfx::Size>(L, 1);
  ASSERT(sz);
  sz->h = lua_tointeger(L, 2);
  return 0;
}

const luaL_Reg Size_methods[] = {
  { "__gc", Size_gc },
  { "__eq", Size_eq },
  { "__tostring", Size_tostring },
  { "__unm", Size_unm },
  { "__add", Size_add },
  { "__sub", Size_sub },
  { "__mul", Size_mul },
  { "__div", Size_div },
  { "__mod", Size_mod },
  { "__pow", Size_pow },
  { "__idiv", Size_div },
  { "union", Size_union },
  { nullptr, nullptr }
};

const Property Size_properties[] = {
  { "w", Size_get_width, Size_set_width },
  { "h", Size_get_height, Size_set_height },
  { "width", Size_get_width, Size_set_width },
  { "height", Size_get_height, Size_set_height },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(gfx::Size);

void register_size_class(lua_State* L)
{
  using gfx::Size;
  REG_CLASS(L, Size);
  REG_CLASS_NEW(L, Size);
  REG_CLASS_PROPERTIES(L, Size);
}

gfx::Size convert_args_into_size(lua_State* L, int index)
{
  return Size_new(L, index);
}

} // namespace script
} // namespace app
