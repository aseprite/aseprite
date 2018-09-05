// Aseprite
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"
#include "gfx/point.h"

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
  // Convert { x, y } into a Point
  else if (lua_istable(L, index)) {
    lua_getfield(L, index, "x");
    lua_getfield(L, index, "y");
    pt.x = lua_tointeger(L, -2);
    pt.y = lua_tointeger(L, -1);
    lua_pop(L, 2);
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
