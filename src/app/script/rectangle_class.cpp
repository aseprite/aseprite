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
#include "gfx/rect.h"

namespace app {
namespace script {

namespace {

gfx::Rect Rectangle_new(lua_State* L, int index)
{
  gfx::Rect rc(0, 0, 0, 0);
  // Copy other rectangle
  if (auto rc2 = may_get_obj<gfx::Rect>(L, index)) {
    rc = *rc2;
  }
  // Convert { x, y, width, height } into a Rectangle
  else if (lua_istable(L, index)) {
    const int type = lua_getfield(L, index, "x");
    if (type != LUA_TNONE &&
        type != LUA_TNIL) {
      lua_getfield(L, index, "y");
      lua_getfield(L, index, "width");
      lua_getfield(L, index, "height");
      rc.x = lua_tointeger(L, -4);
      rc.y = lua_tointeger(L, -3);
      rc.w = lua_tointeger(L, -2);
      rc.h = lua_tointeger(L, -1);
      lua_pop(L, 4);
    }
    else {
      lua_pop(L, 1);
      lua_geti(L, index, 1);
      lua_geti(L, index, 2);
      lua_geti(L, index, 3);
      lua_geti(L, index, 4);
      rc.x = lua_tointeger(L, -4);
      rc.y = lua_tointeger(L, -3);
      rc.w = lua_tointeger(L, -2);
      rc.h = lua_tointeger(L, -1);
      lua_pop(L, 4);
    }
  }
  else {
    rc.x = lua_tointeger(L, index);
    rc.y = lua_tointeger(L, index+1);
    rc.w = lua_tointeger(L, index+2);
    rc.h = lua_tointeger(L, index+3);
  }
  return rc;
}

int Rectangle_new(lua_State* L)
{
  push_obj(L, Rectangle_new(L, 1));
  return 1;
}

int Rectangle_gc(lua_State* L)
{
  get_obj<gfx::Rect>(L, 1)->~RectT();
  return 0;
}

int Rectangle_eq(lua_State* L)
{
  const auto a = get_obj<gfx::Rect>(L, 1);
  const auto b = get_obj<gfx::Rect>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

int Rectangle_tostring(lua_State* L)
{
  const auto rc = get_obj<gfx::Rect>(L, 1);
  lua_pushstring(L, fmt::format("Rectangle{{ x={}, y={}, width={}, height={} }}",
                                rc->x, rc->y, rc->w, rc->h).c_str());
  return 1;
}

int Rectangle_contains(lua_State* L)
{
  const auto a = get_obj<gfx::Rect>(L, 1);
  const auto b = get_obj<gfx::Rect>(L, 2);
  lua_pushboolean(L, a->contains(*b));
  return 1;
}

int Rectangle_intersects(lua_State* L)
{
  const auto a = get_obj<gfx::Rect>(L, 1);
  const auto b = get_obj<gfx::Rect>(L, 2);
  lua_pushboolean(L, a->intersects(*b));
  return 1;
}

int Rectangle_union(lua_State* L)
{
  const auto a = get_obj<gfx::Rect>(L, 1);
  const auto b = get_obj<gfx::Rect>(L, 2);
  push_obj(L, a->createUnion(*b));
  return 1;
}

int Rectangle_intersect(lua_State* L)
{
  const auto a = get_obj<gfx::Rect>(L, 1);
  const auto b = get_obj<gfx::Rect>(L, 2);
  push_obj(L, a->createIntersection(*b));
  return 1;
}

int Rectangle_get_x(lua_State* L)
{
  const auto rc = get_obj<gfx::Rect>(L, 1);
  lua_pushinteger(L, rc->x);
  return 1;
}

int Rectangle_get_y(lua_State* L)
{
  const auto rc = get_obj<gfx::Rect>(L, 1);
  lua_pushinteger(L, rc->y);
  return 1;
}

int Rectangle_get_width(lua_State* L)
{
  const auto rc = get_obj<gfx::Rect>(L, 1);
  lua_pushinteger(L, rc->w);
  return 1;
}

int Rectangle_get_height(lua_State* L)
{
  const auto rc = get_obj<gfx::Rect>(L, 1);
  lua_pushinteger(L, rc->h);
  return 1;
}

int Rectangle_set_x(lua_State* L)
{
  auto rc = get_obj<gfx::Rect>(L, 1);
  rc->x = lua_tointeger(L, 2);
  return 0;
}

int Rectangle_set_y(lua_State* L)
{
  auto rc = get_obj<gfx::Rect>(L, 1);
  rc->y = lua_tointeger(L, 2);
  return 0;
}

int Rectangle_set_width(lua_State* L)
{
  auto rc = get_obj<gfx::Rect>(L, 1);
  rc->w = lua_tointeger(L, 2);
  return 0;
}

int Rectangle_set_height(lua_State* L)
{
  auto rc = get_obj<gfx::Rect>(L, 1);
  rc->h = lua_tointeger(L, 2);
  return 0;
}

int Rectangle_get_isEmpty(lua_State* L)
{
  const auto rc = get_obj<gfx::Rect>(L, 1);
  lua_pushboolean(L, rc->isEmpty());
  return 1;
}

const luaL_Reg Rectangle_methods[] = {
  { "__gc", Rectangle_gc },
  { "__eq", Rectangle_eq },
  { "__tostring", Rectangle_tostring },
  { "__band", Rectangle_intersect },
  { "__bor", Rectangle_union },
  { "contains", Rectangle_contains },
  { "intersects", Rectangle_intersects },
  { "union", Rectangle_union },
  { "intersect", Rectangle_intersect },
  { nullptr, nullptr }
};

const Property Rectangle_properties[] = {
  { "x", Rectangle_get_x, Rectangle_set_x },
  { "y", Rectangle_get_y, Rectangle_set_y },
  { "width", Rectangle_get_width, Rectangle_set_width },
  { "height", Rectangle_get_height, Rectangle_set_height },
  { "isEmpty", Rectangle_get_isEmpty, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(gfx::Rect);

void register_rect_class(lua_State* L)
{
  using Rectangle = gfx::Rect;
  REG_CLASS(L, Rectangle);
  REG_CLASS_NEW(L, Rectangle);
  REG_CLASS_PROPERTIES(L, Rectangle);
}

gfx::Rect convert_args_into_rect(lua_State* L, int index)
{
  return Rectangle_new(L, index);
}

} // namespace script
} // namespace app
