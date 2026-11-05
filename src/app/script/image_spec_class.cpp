// Aseprite
// Copyright (c) 2018-present  Igara Studio S.A.
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

namespace app { namespace script {

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

LUA_FUNCTION(ImageSpec_new)
{
  push_obj(L, ImageSpec_new(L, 1));
  return 1;
}

LUA_FUNCTION(ImageSpec_gc)
{
  get_obj<doc::ImageSpec>(L, 1)->~ImageSpec();
  return 0;
}

LUA_FUNCTION(ImageSpec_eq)
{
  auto a = get_obj<doc::ImageSpec>(L, 1);
  auto b = get_obj<doc::ImageSpec>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

LUA_FUNCTION(ImageSpec_get_colorMode)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  lua_pushinteger(L, (int)spec->colorMode());
  return 1;
}

LUA_FUNCTION(ImageSpec_get_colorSpace)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  if (spec->colorSpace())
    push_color_space(L, *spec->colorSpace());
  else
    lua_pushnil(L);
  return 1;
}

LUA_FUNCTION(ImageSpec_get_width)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  lua_pushinteger(L, spec->width());
  return 1;
}

LUA_FUNCTION(ImageSpec_get_height)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  lua_pushinteger(L, spec->height());
  return 1;
}

LUA_FUNCTION(ImageSpec_get_transparentColor)
{
  const auto spec = get_obj<doc::ImageSpec>(L, 1);
  lua_pushinteger(L, spec->maskColor());
  return 1;
}

LUA_FUNCTION(ImageSpec_set_colorMode)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  spec->setColorMode((doc::ColorMode)lua_tointeger(L, 2));
  return 0;
}

LUA_FUNCTION(ImageSpec_set_colorSpace)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  auto cs = get_obj<gfx::ColorSpace>(L, 2);
  spec->setColorSpace(base::make_ref<gfx::ColorSpace>(*cs));
  return 0;
}

LUA_FUNCTION(ImageSpec_set_width)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  spec->setWidth(lua_tointeger(L, 2));
  return 0;
}

LUA_FUNCTION(ImageSpec_set_height)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  spec->setHeight(lua_tointeger(L, 2));
  return 0;
}

LUA_FUNCTION(ImageSpec_set_transparentColor)
{
  auto spec = get_obj<doc::ImageSpec>(L, 1);
  spec->setMaskColor(lua_tointeger(L, 2));
  return 0;
}

const luaL_Reg ImageSpec_methods[] = {
  { "__gc",  ImageSpec_gc },
  { "__eq",  ImageSpec_eq },
  { nullptr, nullptr      }
};

const Property ImageSpec_properties[] = {
  { "colorMode",        ImageSpec_get_colorMode,        ImageSpec_set_colorMode        },
  { "colorSpace",       ImageSpec_get_colorSpace,       ImageSpec_set_colorSpace       },
  { "width",            ImageSpec_get_width,            ImageSpec_set_width            },
  { "height",           ImageSpec_get_height,           ImageSpec_set_height           },
  { "transparentColor", ImageSpec_get_transparentColor, ImageSpec_set_transparentColor },
  { nullptr,            nullptr,                        nullptr                        }
};
DEF_ITERATOR_PAIRS(ImageSpec);
} // anonymous namespace

DEF_MTNAME(doc::ImageSpec);

LUA_FUNCTION(register_image_spec_class)
{
  using doc::ImageSpec;
  REG_CLASS(L, ImageSpec);
  REG_CLASS_NEW(L, ImageSpec);
  REG_CLASS_PROPERTIES(L, ImageSpec);
  return 0;
}

doc::ImageSpec convert_args_into_image_spec(lua_State* L, int index)
{
  return ImageSpec_new(L, index);
}

}} // namespace app::script
