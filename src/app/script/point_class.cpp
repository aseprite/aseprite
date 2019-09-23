// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"
#include "fmt/format.h"
#include "gfx/point.h"

#include <cmath>

namespace app {
namespace script {

namespace {

gfx::Point Point_new(lua_State* L, int index)
{
  gfx::Point pt(0, 0);
  // Copy other rectangle
  if (auto pt2 = may_get_obj<gfx::Point>(L, index)) {
    pt = *pt2;
  }
  // Convert {x=int,y=int} or {int,int} into a Point
  else if (lua_istable(L, index)) {
    const int type = lua_getfield(L, index, "x");
    if (type != LUA_TNONE &&
        type != LUA_TNIL) {
      lua_getfield(L, index, "y");
      pt.x = lua_tointeger(L, -2);
      pt.y = lua_tointeger(L, -1);
      lua_pop(L, 2);
    }
    else {
      lua_pop(L, 1);

      // TODO Investigate this further: why we cannot use two
      // lua_geti() calls and then lua_pop(L, 2) when we are iterating
      // points in a table defined like {{0,0},{32,32}}

      lua_geti(L, index, 1);
      pt.x = lua_tointeger(L, -1);
      lua_pop(L, 1);

      lua_geti(L, index, 2);
      pt.y = lua_tointeger(L, -1);
      lua_pop(L, 1);
    }
  }
  else {
    pt.x = lua_tointeger(L, index);
    pt.y = lua_tointeger(L, index+1);
  }
  return pt;
}

int Point_new(lua_State* L)
{
  push_obj(L, Point_new(L, 1));
  return 1;
}

int Point_gc(lua_State* L)
{
  get_obj<gfx::Point>(L, 1)->~PointT();
  return 0;
}

int Point_eq(lua_State* L)
{
  const auto a = get_obj<gfx::Point>(L, 1);
  const auto b = get_obj<gfx::Point>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

int Point_tostring(lua_State* L)
{
  const auto pt = get_obj<gfx::Point>(L, 1);
  lua_pushstring(L, fmt::format("Point{{ x={}, y={} }}",
                                pt->x, pt->y).c_str());
  return 1;
}

int Point_unm(lua_State* L)
{
  const auto pt = get_obj<gfx::Point>(L, 1);
  push_obj(L, -(*pt));
  return 1;
}

int Point_add(lua_State* L)
{
  gfx::Point result(0, 0);
  if (lua_isuserdata(L, 1))
    result += *get_obj<gfx::Point>(L, 1);
  else
    result += lua_tointeger(L, 1);
  if (lua_isuserdata(L, 2))
    result += *get_obj<gfx::Point>(L, 2);
  else
    result += lua_tointeger(L, 2);
  push_obj(L, result);
  return 1;
}

int Point_sub(lua_State* L)
{
  gfx::Point result = *get_obj<gfx::Point>(L, 1);
  if (lua_isuserdata(L, 2))
    result -= *get_obj<gfx::Point>(L, 2);
  else
    result -= lua_tointeger(L, 2);
  push_obj(L, result);
  return 1;
}

int Point_mul(lua_State* L)
{
  gfx::Point result = *get_obj<gfx::Point>(L, 1);
  result *= lua_tointeger(L, 2);
  push_obj(L, result);
  return 1;
}

int Point_div(lua_State* L)
{
  gfx::Point result = *get_obj<gfx::Point>(L, 1);
  const int value = lua_tointeger(L, 2);
  if (value == 0)
    return luaL_error(L, "attempt to divide by zero");
  result /= value;
  push_obj(L, result);
  return 1;
}

int Point_mod(lua_State* L)
{
  gfx::Point result = *get_obj<gfx::Point>(L, 1);
  const int value = lua_tointeger(L, 2);
  if (value == 0)
    return luaL_error(L, "attempt to divide by zero");
  result.x %= value;
  result.y %= value;
  push_obj(L, result);
  return 1;
}

int Point_pow(lua_State* L)
{
  gfx::Point result = *get_obj<gfx::Point>(L, 1);
  const int value = lua_tointeger(L, 2);
  result.x = std::pow(result.x, value);
  result.y = std::pow(result.y, value);
  push_obj(L, result);
  return 1;
}

int Point_get_x(lua_State* L)
{
  const auto pt = get_obj<gfx::Point>(L, 1);
  lua_pushinteger(L, pt->x);
  return 1;
}

int Point_get_y(lua_State* L)
{
  const auto pt = get_obj<gfx::Point>(L, 1);
  lua_pushinteger(L, pt->y);
  return 1;
}

int Point_set_x(lua_State* L)
{
  auto pt = get_obj<gfx::Point>(L, 1);
  pt->x = lua_tointeger(L, 2);
  return 0;
}

int Point_set_y(lua_State* L)
{
  auto pt = get_obj<gfx::Point>(L, 1);
  pt->y = lua_tointeger(L, 2);
  return 0;
}

const luaL_Reg Point_methods[] = {
  { "__gc", Point_gc },
  { "__eq", Point_eq },
  { "__tostring", Point_tostring },
  { "__unm", Point_unm },
  { "__add", Point_add },
  { "__sub", Point_sub },
  { "__mul", Point_mul },
  { "__div", Point_div },
  { "__mod", Point_mod },
  { "__pow", Point_pow },
  { "__idiv", Point_div },
  { nullptr, nullptr }
};

const Property Point_properties[] = {
  { "x", Point_get_x, Point_set_x },
  { "y", Point_get_y, Point_set_y },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(gfx::Point);

void register_point_class(lua_State* L)
{
  using gfx::Point;
  REG_CLASS(L, Point);
  REG_CLASS_NEW(L, Point);
  REG_CLASS_PROPERTIES(L, Point);
}

gfx::Point convert_args_into_point(lua_State* L, int index)
{
  return Point_new(L, index);
}

} // namespace script
} // namespace app
