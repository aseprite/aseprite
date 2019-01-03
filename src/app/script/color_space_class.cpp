// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/script/luacpp.h"
#include "base/file_content.h"
#include "gfx/color_space.h"

namespace app {
namespace script {

namespace {

int ColorSpace_new(lua_State* L)
{
  // Copy color space
  if (auto cs2 = may_get_obj<gfx::ColorSpace>(L, 1)) {
    push_new<gfx::ColorSpace>(L, *cs2);
    return 1;
  }
  else if (lua_istable(L, 1)) {
    // Load ICC profile when ColorSpace{ filename="..." } is specified
    if (lua_getfield(L, 1, "filename") != LUA_TNIL) {
      const char* fn = lua_tostring(L, -1);
      if (fn) {
        auto buf = base::read_file_content(fn);
        lua_pop(L, 1);
        push_new<gfx::ColorSpace>(L, *gfx::ColorSpace::MakeICC(std::move(buf)));
        return 1;
      }
    }
    else
      lua_pop(L, 1);

    // Create sRGB profile with ColorSpace{ sRGB }
    if (lua_getfield(L, 1, "sRGB") != LUA_TNONE) {
      lua_pop(L, 1);
      push_new<gfx::ColorSpace>(L, *gfx::ColorSpace::MakeSRGB());
      return 1;
    }
    else
      lua_pop(L, 1);
  }
  push_new<gfx::ColorSpace>(L, *gfx::ColorSpace::MakeNone());
  return 1;
}

int ColorSpace_gc(lua_State* L)
{
  get_obj<gfx::ColorSpace>(L, 1)->~ColorSpace();
  return 0;
}

int ColorSpace_eq(lua_State* L)
{
  const auto a = get_obj<gfx::ColorSpace>(L, 1);
  const auto b = get_obj<gfx::ColorSpace>(L, 2);
  lua_pushboolean(L, a->nearlyEqual(*b));
  return 1;
}

int ColorSpace_get_name(lua_State* L)
{
  const auto cs = get_obj<gfx::ColorSpace>(L, 1);
  lua_pushstring(L, cs->name().c_str());
  return 1;
}

int ColorSpace_set_name(lua_State* L)
{
  auto cs = get_obj<gfx::ColorSpace>(L, 1);
  if (auto name = lua_tostring(L, 2))
    cs->setName(name);
  return 0;
}

const luaL_Reg ColorSpace_methods[] = {
  { "__gc", ColorSpace_gc },
  { "__eq", ColorSpace_eq },
  { nullptr, nullptr }
};

const Property ColorSpace_properties[] = {
  { "name", ColorSpace_get_name, ColorSpace_set_name },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(gfx::ColorSpace);

void register_color_space_class(lua_State* L)
{
  using ColorSpace = gfx::ColorSpace;
  REG_CLASS(L, ColorSpace);
  REG_CLASS_NEW(L, ColorSpace);
  REG_CLASS_PROPERTIES(L, ColorSpace);
}

void push_color_space(lua_State* L, const gfx::ColorSpace& cs)
{
  push_new<gfx::ColorSpace>(L, cs);
}

} // namespace script
} // namespace app
