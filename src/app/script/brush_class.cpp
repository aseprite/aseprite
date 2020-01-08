// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "doc/brush.h"
#include "doc/image.h"
#include "doc/mask.h"

#include <algorithm>

namespace app {
namespace script {

namespace {

using namespace doc;

struct BrushObj {
  BrushRef brush;
  BrushObj(const BrushRef& brush)
    : brush(brush) {
  }
};

BrushRef Brush_new(lua_State* L, int index)
{
  BrushRef brush;
  if (auto brush2 = may_get_obj<BrushObj>(L, index)) {
    ASSERT(brush2->brush);
    if (brush2->brush)
      brush.reset(new Brush(*brush2->brush));
  }
  else if (auto image = may_get_image_from_arg(L, index)) {
    if (image) {
      brush.reset(new Brush(kImageBrushType, 1, 0));
      brush->setImage(image, nullptr);
      brush->setPattern(BrushPattern::DEFAULT_FOR_SCRIPTS);
    }
  }
  else if (lua_istable(L, index)) {
    Image* image = nullptr;
    if (lua_getfield(L, index, "image") != LUA_TNIL) {
      image = may_get_image_from_arg(L, -1);
    }
    lua_pop(L, 1);

    BrushType type = (image ? BrushType::kImageBrushType:
                              BrushType::kCircleBrushType);
    if (lua_getfield(L, index, "type") != LUA_TNIL)
      type = (BrushType)lua_tointeger(L, -1);
    lua_pop(L, 1);

    int size = 1;
    if (lua_getfield(L, index, "size") != LUA_TNIL) {
      size = lua_tointeger(L, -1);
      size = std::max(1, size);
    }
    lua_pop(L, 1);

    int angle = 0;
    if (lua_getfield(L, index, "angle") != LUA_TNIL)
      angle = lua_tointeger(L, -1);
    lua_pop(L, 1);

    brush.reset(new Brush(type, size, angle));
    if (image) {
      // TODO add a way to add a bitmap mask
      doc::Mask mask;
      mask.replace(image->bounds());
      brush->setImage(image, mask.bitmap());
    }

    if (lua_getfield(L, index, "center") != LUA_TNIL) {
      gfx::Point center = convert_args_into_point(L, -1);
      brush->setCenter(center);
    }
    lua_pop(L, 1);

    if (lua_getfield(L, index, "pattern") != LUA_TNIL) {
      BrushPattern pattern = (BrushPattern)lua_tointeger(L, -1);
      brush->setPattern(pattern);
    }
    else {
      brush->setPattern(BrushPattern::DEFAULT_FOR_SCRIPTS);
    }
    lua_pop(L, 1);

    if (lua_getfield(L, index, "patternOrigin") != LUA_TNIL) {
      gfx::Point patternOrigin = convert_args_into_point(L, -1);
      brush->setPatternOrigin(patternOrigin);
    }
    lua_pop(L, 1);
  }
  else {
    int size = lua_tointeger(L, index);
    size = std::max(1, size);
    brush.reset(new Brush(BrushType::kCircleBrushType, size, 0));
    brush->setPattern(BrushPattern::DEFAULT_FOR_SCRIPTS);
  }
  return brush;
}

int Brush_new(lua_State* L)
{
  BrushRef brush = Brush_new(L, 1);
  if (brush)
    push_new<BrushObj>(L, brush);
  else
    lua_pushnil(L);
  return 1;
}

int Brush_gc(lua_State* L)
{
  get_obj<BrushObj>(L, 1)->~BrushObj();
  return 0;
}

int Brush_setFgColor(lua_State* L)
{
  auto obj = get_obj<BrushObj>(L, 1);
  if (obj->brush &&
      obj->brush->image()) {
    const doc::color_t color = convert_args_into_pixel_color(
      L, 2, obj->brush->image()->pixelFormat());
    obj->brush->setImageColor(Brush::ImageColor::MainColor, color);
  }
  return 0;
}

int Brush_setBgColor(lua_State* L)
{
  auto obj = get_obj<BrushObj>(L, 1);
  if (obj->brush &&
      obj->brush->image()) {
    const doc::color_t color = convert_args_into_pixel_color(
      L, 2, obj->brush->image()->pixelFormat());
    obj->brush->setImageColor(
      Brush::ImageColor::BackgroundColor, color);
  }
  return 0;
}

int Brush_get_type(lua_State* L)
{
  const auto obj = get_obj<BrushObj>(L, 1);
  lua_pushinteger(L, (int)obj->brush->type());
  return 1;
}

int Brush_get_size(lua_State* L)
{
  const auto obj = get_obj<BrushObj>(L, 1);
  lua_pushinteger(L, obj->brush->size());
  return 1;
}

int Brush_get_angle(lua_State* L)
{
  const auto obj = get_obj<BrushObj>(L, 1);
  lua_pushinteger(L, obj->brush->angle());
  return 1;
}

int Brush_get_image(lua_State* L)
{
  const auto obj = get_obj<BrushObj>(L, 1);
  if (obj->brush->type() == BrushType::kImageBrushType)
    push_image(L, Image::createCopy(obj->brush->image()));
  else
    lua_pushnil(L);
  return 1;
}

int Brush_get_center(lua_State* L)
{
  const auto obj = get_obj<BrushObj>(L, 1);
  push_obj(L, obj->brush->center());
  return 1;
}

int Brush_get_pattern(lua_State* L)
{
  const auto obj = get_obj<BrushObj>(L, 1);
  lua_pushinteger(L, (int)obj->brush->pattern());
  return 1;
}

int Brush_get_patternOrigin(lua_State* L)
{
  const auto obj = get_obj<BrushObj>(L, 1);
  push_obj(L, obj->brush->patternOrigin());
  return 1;
}

const luaL_Reg Brush_methods[] = {
  { "__gc", Brush_gc },
  { "setFgColor", Brush_setFgColor },
  { "setBgColor", Brush_setBgColor },
  { nullptr, nullptr }
};

const Property Brush_properties[] = {
  { "type", Brush_get_type, nullptr },
  { "size", Brush_get_size, nullptr },
  { "angle", Brush_get_angle, nullptr },
  { "image", Brush_get_image, nullptr },
  { "center", Brush_get_center, nullptr },
  { "pattern", Brush_get_pattern, nullptr },
  { "patternOrigin", Brush_get_patternOrigin, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(BrushObj);

void register_brush_class(lua_State* L)
{
  using Brush = BrushObj;
  REG_CLASS(L, Brush);
  REG_CLASS_NEW(L, Brush);
  REG_CLASS_PROPERTIES(L, Brush);
}

void push_brush(lua_State* L, const BrushRef& brush)
{
  push_new<BrushObj>(L, brush);
}

BrushRef get_brush_from_arg(lua_State* L, int index)
{
  if (auto obj = may_get_obj<BrushObj>(L, index))
    return obj->brush;
  else
    return Brush_new(L, index);
}

} // namespace script
} // namespace app
