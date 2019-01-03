// Aseprite
// Copyright (c) 2018-2019  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "doc/image_spec.h"

namespace app {
namespace script {

namespace {

doc::ImageSpec ImageSpec_new(lua_State* L, int index)
{
  doc::ImageSpec spec(doc::ColorMode::RGB, 1, 1, 0);
  // Copy other size
  if (auto spec2 = may_get_obj<doc::ImageSpec>(L, index)) {
    spec = *spec2;
  }
  // Convert { width, height } into a Size
  else if (lua_istable(L, index)) {
    if (lua_getfield(L, index, "colorMode") != LUA_TNIL)
      spec.setColorMode((doc::ColorMode)lua_tointeger(L, -1));
    lua_pop(L, 1);

    if (lua_getfield(L, index, "width") != LUA_TNIL)
      spec.setWidth(lua_tointeger(L, -1));
    lua_pop(L, 1);

    if (lua_getfield(L, index, "height") != LUA_TNIL)
      spec.setHeight(lua_tointeger(L, -1));
    lua_pop(L, 1);

    if (lua_getfield(L, index, "transparentColor") != LUA_TNIL)
      spec.setMaskColor(lua_tointeger(L, -1));
    lua_pop(L, 1);
  }
  return spec;
}

int ImageSpec_new(lua_State* L)
{
  push_obj(L, ImageSpec_new(L, 1));
  return 1;
}

int ImageSpec_gc(lua_State* L)
{
  get_obj<doc::ImageSpec>(L, 1)->~ImageSpec();
  return 0;
}

int ImageSpec_eq(lua_State* L)
{
  auto a = get_obj<doc::ImageSpec>(L, 1);
  auto b = get_obj<doc::ImageSpec>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

int ImageSpec_get_colorMode(lua_State* L)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  lua_pushinteger(L, (int)spec->colorMode());
  return 1;
}

int ImageSpec_get_colorSpace(lua_State* L)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  if (spec->colorSpace())
    push_color_space(L, *spec->colorSpace());
  else
    lua_pushnil(L);
  return 1;
}

int ImageSpec_get_width(lua_State* L)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  lua_pushinteger(L, spec->width());
  return 1;
}

int ImageSpec_get_height(lua_State* L)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  lua_pushinteger(L, spec->height());
  return 1;
}

int ImageSpec_get_transparentColor(lua_State* L)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  lua_pushinteger(L, spec->maskColor());
  return 1;
}

int ImageSpec_set_colorMode(lua_State* L)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  spec->setColorMode((doc::ColorMode)lua_tointeger(L, 2));
  return 0;
}

int ImageSpec_set_colorSpace(lua_State* L)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  auto cs = get_obj<gfx::ColorSpace>(L, 2);
  spec->setColorSpace(std::make_shared<gfx::ColorSpace>(*cs));
  return 0;
}

int ImageSpec_set_width(lua_State* L)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  spec->setWidth(lua_tointeger(L, 2));
  return 0;
}

int ImageSpec_set_height(lua_State* L)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  spec->setHeight(lua_tointeger(L, 2));
  return 0;
}

int ImageSpec_set_transparentColor(lua_State* L)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  spec->setMaskColor(lua_tointeger(L, 2));
  return 0;
}

const luaL_Reg ImageSpec_methods[] = {
  { "__gc", ImageSpec_gc },
  { "__eq", ImageSpec_eq },
  { nullptr, nullptr }
};

const Property ImageSpec_properties[] = {
  { "colorMode", ImageSpec_get_colorMode, ImageSpec_set_colorMode },
  { "colorSpace", ImageSpec_get_colorSpace, ImageSpec_set_colorSpace },
  { "width", ImageSpec_get_width, ImageSpec_set_width },
  { "height", ImageSpec_get_height, ImageSpec_set_height },
  { "transparentColor", ImageSpec_get_transparentColor, ImageSpec_set_transparentColor },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(doc::ImageSpec);

void register_image_spec_class(lua_State* L)
{
  using doc::ImageSpec;
  REG_CLASS(L, ImageSpec);
  REG_CLASS_NEW(L, ImageSpec);
  REG_CLASS_PROPERTIES(L, ImageSpec);
}

doc::ImageSpec convert_args_into_image_spec(lua_State* L, int index)
{
  return ImageSpec_new(L, index);
}

} // namespace script
} // namespace app
