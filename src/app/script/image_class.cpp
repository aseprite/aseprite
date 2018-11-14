// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/copy_rect.h"
#include "app/cmd/copy_region.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/tx.h"
#include "app/util/autocrop.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "render/render.h"

namespace app {
namespace script {

namespace {

struct ImageObj {
  doc::ImageRef image;
  doc::Cel* cel;
  ImageObj(const doc::ImageRef& image, doc::Cel* cel)
    : image(image)
    , cel(cel) {
  }
  ImageObj(const ImageObj&) = delete;
  ImageObj& operator=(const ImageObj&) = delete;
};

int Image_new(lua_State* L)
{
  doc::ImageSpec spec(doc::ColorMode::RGB, 1, 1, 0);
  if (auto spec2 = may_get_obj<doc::ImageSpec>(L, 1)) {
    spec = *spec2;
  }
  else {
    const int w = lua_tointeger(L, 1);
    const int h = lua_tointeger(L, 2);
    const int colorMode = (lua_isnone(L, 3) ? doc::IMAGE_RGB:
                                              lua_tointeger(L, 3));
    spec.setWidth(w);
    spec.setHeight(h);
    spec.setColorMode((doc::ColorMode)colorMode);
  }
  doc::ImageRef image(doc::Image::create(spec));
  doc::clear_image(image.get(), spec.maskColor());
  push_new<ImageObj>(L, image, nullptr);
  return 1;
}

int Image_clone(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  doc::ImageRef cloned(doc::Image::createCopy(obj->image.get()));
  push_new<ImageObj>(L, cloned, nullptr);
  return 1;
}

int Image_gc(lua_State* L)
{
  get_obj<ImageObj>(L, 1)->~ImageObj();
  return 0;
}

int Image_clear(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  doc::color_t color;
  if (lua_isnone(L, 2))
    color = obj->image.get()->maskColor();
  else if (lua_isinteger(L, 2))
    color = lua_tointeger(L, 2);
  else
    color = convert_args_into_pixel_color(L, 2);
  doc::clear_image(obj->image.get(), color);
  return 0;
}

int Image_drawPixel(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  const int x = lua_tointeger(L, 2);
  const int y = lua_tointeger(L, 3);
  doc::color_t color;
  if (lua_isinteger(L, 4))
    color = lua_tointeger(L, 4);
  else
    color = convert_args_into_pixel_color(L, 4);
  doc::put_pixel(obj->image.get(), x, y, color);
  return 0;
}

int Image_drawImage(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  auto sprite = get_obj<ImageObj>(L, 2);
  gfx::Point pos = convert_args_into_point(L, 3);
  Image* dst = obj->image.get();
  const Image* src = sprite->image.get();

  // If the destination image is not related to a sprite, we just draw
  // the source image without undo information.
  if (obj->cel == nullptr) {
    doc::copy_image(dst, src, pos.x, pos.y);
  }
  else {
    gfx::Rect bounds(pos, src->size());
    gfx::Rect output;
    if (doc::algorithm::shrink_bounds2(src, dst, bounds, output)) {
      Tx tx;
      tx(new cmd::CopyRegion(
           dst, src, gfx::Region(output),
           gfx::Point(0, 0)));
      tx.commit();
    }
  }
  return 0;
}

int Image_drawSprite(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  const auto sprite = get_ptr<Sprite>(L, 2);
  doc::frame_t frame = lua_tointeger(L, 3)-1;
  gfx::Point pos = convert_args_into_point(L, 4);
  doc::Image* dst = obj->image.get();

  ASSERT(dst);
  ASSERT(sprite);

  // If the destination image is not related to a sprite, we just draw
  // the source image without undo information.
  if (obj->cel == nullptr) {
    render::Render render;
    render.renderSprite(
      dst, sprite, frame,
      gfx::Clip(pos.x, pos.y,
                0, 0,
                sprite->width(),
                sprite->height()));
  }
  else {
    Tx tx;

    ImageRef tmp(Image::createCopy(dst));
    render::Render render;
    render.renderSprite(
      tmp.get(), sprite, frame,
      gfx::Clip(pos.x, pos.y,
                0, 0,
                sprite->width(),
                sprite->height()));

    int x1, y1, x2, y2;
    if (get_shrink_rect2(&x1, &y1, &x2, &y2, dst, tmp.get())) {
      tx(new cmd::CopyRect(
           dst, tmp.get(),
           gfx::Clip(x1, y1, x1, y1, x2-x1+1, y2-y1+1)));
    }

    tx.commit();
  }
  return 0;
}

int Image_pixels(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  push_image_iterator_function(L, obj->image, 2);
  return 1;
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

int Image_isEqual(lua_State* L)
{
  auto objA = get_obj<ImageObj>(L, 1);
  auto objB = get_obj<ImageObj>(L, 2);
  bool res = doc::is_same_image(objA->image.get(),
                                objB->image.get());
  lua_pushboolean(L, res);
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

int Image_get_spec(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  push_obj(L, obj->image->spec());
  return 1;
}

const luaL_Reg Image_methods[] = {
  { "clone", Image_clone },
  { "clear", Image_clear },
  { "getPixel", Image_getPixel },
  { "drawPixel", Image_drawPixel }, { "putPixel", Image_drawPixel },
  { "drawImage", Image_drawImage }, { "putImage", Image_drawImage }, // TODO putImage is deprecated
  { "drawSprite", Image_drawSprite }, { "putSprite", Image_drawSprite }, // TODO putSprite is deprecated
  { "pixels", Image_pixels },
  { "isEqual", Image_isEqual },
  { "__gc", Image_gc },
  { nullptr, nullptr }
};

const Property Image_properties[] = {
  { "width", Image_get_width, nullptr },
  { "height", Image_get_height, nullptr },
  { "colorMode", Image_get_colorMode, nullptr },
  { "spec", Image_get_spec, nullptr },
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
  push_new<ImageObj>(L, cel->imageRef(), cel);
}

doc::Image* may_get_image_from_arg(lua_State* L, int index)
{
  auto obj = may_get_obj<ImageObj>(L, index);
  if (obj)
    return obj->image.get();
  else
    return nullptr;
}

doc::Image* get_image_from_arg(lua_State* L, int index)
{
  return get_obj<ImageObj>(L, index)->image.get();
}

doc::Cel* get_image_cel_from_arg(lua_State* L, int index)
{
  return get_obj<ImageObj>(L, index)->cel;
}

} // namespace script
} // namespace app
