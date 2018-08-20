// Aseprite
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"
#include "doc/image.h"
#include "doc/primitives.h"

namespace app {
namespace script {

namespace {

int Image_new(lua_State* L)
{
  lua_pushnil(L);          // TODO
  return 1;
}

int Image_putPixel(lua_State* L)
{
  auto image = get_ptr<doc::Image>(L, 1);
  const int x = lua_tointeger(L, 2);
  const int y = lua_tointeger(L, 3);
  const doc::color_t color = lua_tointeger(L, 4);
  doc::put_pixel(image, x, y, color);
  return 0;
}

int Image_getPixel(lua_State* L)
{
  const auto image = get_ptr<doc::Image>(L, 1);
  const int x = lua_tointeger(L, 2);
  const int y = lua_tointeger(L, 3);
  const doc::color_t color = doc::get_pixel(image, x, y);
  lua_pushinteger(L, color);
  return 1;
}

int Image_get_width(lua_State* L)
{
  const auto image = get_ptr<doc::Image>(L, 1);
  lua_pushinteger(L, image->width());
  return 1;
}

int Image_get_height(lua_State* L)
{
  const auto image = get_ptr<doc::Image>(L, 1);
  lua_pushinteger(L, image->height());
  return 1;
}

const luaL_Reg Image_methods[] = {
  { "getPixel", Image_getPixel },
  { "putPixel", Image_putPixel },
  { nullptr, nullptr }
};

const Property Image_properties[] = {
  { "width", Image_get_width, nullptr },
  { "height", Image_get_height, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(doc::Image);

void register_image_class(lua_State* L)
{
  using doc::Image;
  REG_CLASS(L, Image);
  REG_CLASS_NEW(L, Image);
  REG_CLASS_PROPERTIES(L, Image);
}

} // namespace script
} // namespace app
