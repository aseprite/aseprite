// Aseprite
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"
#include "doc/sprite.h"

namespace app {
namespace script {

namespace {

struct ImageObj {
  doc::ImageRef image;
  doc::Sprite* sprite;
  ImageObj(const doc::ImageRef& image, doc::Sprite* sprite)
    : image(image)
    , sprite(sprite) {
  }
  ImageObj(ImageObj&& that) {
    std::swap(image, that.image);
    std::swap(sprite, that.sprite);
  }
  ImageObj(const ImageObj&) = delete;
  ImageObj& operator=(const ImageObj&) = delete;
};

int Image_new(lua_State* L)
{
  const int w = lua_tointeger(L, 1);
  const int h = lua_tointeger(L, 2);
  const int colorMode = (lua_isnone(L, 3) ? doc::IMAGE_RGB:
                                            lua_tointeger(L, 3));
  doc::ImageRef image(doc::Image::create((doc::PixelFormat)colorMode, w, h));
  push_obj(L, ImageObj(image, nullptr));
  return 1;
}

int Image_clone(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  doc::ImageRef cloned(doc::Image::createCopy(obj->image.get()));
  push_obj(L, ImageObj(cloned, nullptr));
  return 1;
}

int Image_gc(lua_State* L)
{
  get_obj<ImageObj>(L, 1)->~ImageObj();
  return 0;
}

int Image_putPixel(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  const int x = lua_tointeger(L, 2);
  const int y = lua_tointeger(L, 3);
  const doc::color_t color = lua_tointeger(L, 4);
  doc::put_pixel(obj->image.get(), x, y, color);
  return 0;
}

int Image_getPixel(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  const int x = lua_tointeger(L, 2);
  const int y = lua_tointeger(L, 3);
  const doc::color_t color = doc::get_pixel(obj->image.get(), x, y);
  lua_pushinteger(L, color);
  return 1;
}

int Image_get_width(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  lua_pushinteger(L, obj->image->width());
  return 1;
}

int Image_get_height(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  lua_pushinteger(L, obj->image->height());
  return 1;
}

int Image_get_colorMode(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  lua_pushinteger(L, obj->image->pixelFormat());
  return 1;
}

const luaL_Reg Image_methods[] = {
  { "clone", Image_clone },
  { "getPixel", Image_getPixel },
  { "putPixel", Image_putPixel },
  { "__gc",     Image_gc },
  { nullptr, nullptr }
};

const Property Image_properties[] = {
  { "width", Image_get_width, nullptr },
  { "height", Image_get_height, nullptr },
  { "colorMode", Image_get_colorMode, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(ImageObj);

void register_image_class(lua_State* L)
{
  using Image = ImageObj;
  REG_CLASS(L, Image);
  REG_CLASS_NEW(L, Image);
  REG_CLASS_PROPERTIES(L, Image);
}

void push_cel_image(lua_State* L, doc::Cel* cel)
{
  push_obj(L, ImageObj(cel->imageRef(), cel->sprite()));
}

} // namespace script
} // namespace app
