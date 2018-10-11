// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/script/luacpp.h"

namespace app {
namespace script {

namespace {

app::Color Color_new(lua_State* L, int index)
{
  app::Color color;
  // Copy other color
  if (auto color2 = may_get_obj<app::Color>(L, index)) {
    color = *color2;
  }
  // Convert table into a Color
  else if (lua_istable(L, index)) {
    // Convert { r, g, b } into a Color
    if (lua_getfield(L, index, "r") != LUA_TNIL) {
      lua_getfield(L, index, "g");
      lua_getfield(L, index, "b");
      int a = 255;
      if (lua_getfield(L, index, "a") != LUA_TNIL)
        a = lua_tointeger(L, -1);
      color = app::Color::fromRgb(lua_tointeger(L, -4),
                                  lua_tointeger(L, -3),
                                  lua_tointeger(L, -2), a);
      lua_pop(L, 4);
      return color;
    }
    else
      lua_pop(L, 1);

    // Convert { red, green, blue } into a Color
    if (lua_getfield(L, index, "red") != LUA_TNIL) {
      lua_getfield(L, index, "green");
      lua_getfield(L, index, "blue");
      int a = 255;
      if (lua_getfield(L, index, "alpha") != LUA_TNIL)
        a = lua_tointeger(L, -1);
      color = app::Color::fromRgb(lua_tointeger(L, -4),
                                  lua_tointeger(L, -3),
                                  lua_tointeger(L, -2), a);
      lua_pop(L, 4);
      return color;
    }
    else
      lua_pop(L, 1);

    // Convert { h, s, v } into a Color
    if (lua_getfield(L, index, "v") != LUA_TNIL) {
      lua_getfield(L, index, "s");
      lua_getfield(L, index, "h");
      int a = 255;
      if (lua_getfield(L, index, "a") != LUA_TNIL)
        a = lua_tointeger(L, -1);
      color = app::Color::fromHsv(lua_tonumber(L, -2),
                                  lua_tonumber(L, -3),
                                  lua_tonumber(L, -4), a);
      lua_pop(L, 4);
      return color;
    }
    else
      lua_pop(L, 1);

    // Convert { hue, saturation, value } into a Color
    if (lua_getfield(L, index, "value") != LUA_TNIL) {
      lua_getfield(L, index, "saturation");
      lua_getfield(L, index, "hue");
      int a = 255;
      if (lua_getfield(L, index, "alpha") != LUA_TNIL)
        a = lua_tointeger(L, -1);
      color = app::Color::fromHsv(lua_tonumber(L, -2),
                                  lua_tonumber(L, -3),
                                  lua_tonumber(L, -4), a);
      lua_pop(L, 4);
      return color;
    }
    else
      lua_pop(L, 1);

    // Convert { h, s, l } into a Color
    if (lua_getfield(L, index, "l") != LUA_TNIL) {
      lua_getfield(L, index, "s");
      lua_getfield(L, index, "h");
      int a = 255;
      if (lua_getfield(L, index, "a") != LUA_TNIL)
        a = lua_tointeger(L, -1);
      color = app::Color::fromHsl(lua_tonumber(L, -2),
                                  lua_tonumber(L, -3),
                                  lua_tonumber(L, -4), a);
      lua_pop(L, 4);
      return color;
    }
    else
      lua_pop(L, 1);

    // Convert { hue, saturation, lightness } into a Color
    if (lua_getfield(L, index, "lightness") != LUA_TNIL) {
      lua_getfield(L, index, "saturation");
      lua_getfield(L, index, "hue");
      int a = 255;
      if (lua_getfield(L, index, "alpha") != LUA_TNIL)
        a = lua_tointeger(L, -1);
      color = app::Color::fromHsv(lua_tonumber(L, -2),
                                  lua_tonumber(L, -3),
                                  lua_tonumber(L, -4), a);
      lua_pop(L, 4);
      return color;
    }
    else
      lua_pop(L, 1);

    // Convert { gray } into a Color
    if (lua_getfield(L, index, "gray") != LUA_TNIL) {
      int a = 255;
      if (lua_getfield(L, index, "alpha") != LUA_TNIL)
        a = lua_tointeger(L, -1);
      color = app::Color::fromGray(lua_tonumber(L, -2), a);
      lua_pop(L, 2);
      return color;
    }
    else
      lua_pop(L, 1);
  }
  // raw color into app color
  else if (!lua_isnone(L, index)) {
    if (lua_isinteger(L, index) && lua_isnone(L, index+1)) {
      doc::color_t docColor = lua_tointeger(L, index);
      switch (app_get_current_pixel_format()) {
        case IMAGE_RGB:
          color = app::Color::fromRgb(doc::rgba_getr(docColor),
                                      doc::rgba_getg(docColor),
                                      doc::rgba_getb(docColor),
                                      doc::rgba_geta(docColor));
          break;
        case IMAGE_GRAYSCALE:
          color = app::Color::fromGray(doc::graya_getv(docColor),
                                       doc::graya_geta(docColor));
          break;
        case IMAGE_INDEXED:
          color = app::Color::fromIndex(docColor);
          break;
      }
    }
    else {
      color = app::Color::fromRgb(lua_tointeger(L, index),
                                  lua_tointeger(L, index+1),
                                  lua_tointeger(L, index+2),
                                  lua_isnone(L, index+3) ?
                                  255: lua_tointeger(L, index+3));
    }
  }
  return color;
}

int Color_new(lua_State* L)
{
  push_obj(L, Color_new(L, 1));
  return 1;
}

int Color_gc(lua_State* L)
{
  get_obj<app::Color>(L, 1)->~Color();
  return 0;
}

int Color_eq(lua_State* L)
{
  const auto a = get_obj<app::Color>(L, 1);
  const auto b = get_obj<app::Color>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

int Color_get_red(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushinteger(L, color->getRed());
  return 1;
}

int Color_get_green(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushinteger(L, color->getGreen());
  return 1;
}

int Color_get_blue(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushinteger(L, color->getBlue());
  return 1;
}

int Color_get_alpha(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushinteger(L, color->getAlpha());
  return 1;
}

int Color_get_hsvHue(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHsvHue());
  return 1;
}

int Color_get_hsvSaturation(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHsvSaturation());
  return 1;
}

int Color_get_hsvValue(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHsvValue());
  return 1;
}

int Color_get_hslHue(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHslHue());
  return 1;
}

int Color_get_hslSaturation(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHslSaturation());
  return 1;
}

int Color_get_hslLightness(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHslLightness());
  return 1;
}

int Color_get_hue(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  if (color->getType() == Color::HslType)
    return Color_get_hslHue(L);
  else
    return Color_get_hsvHue(L);
}

int Color_get_saturation(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  if (color->getType() == Color::HslType)
    return Color_get_hslSaturation(L);
  else
    return Color_get_hsvSaturation(L);
}

int Color_get_gray(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getGray());
  return 1;
}

int Color_get_index(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getIndex());
  return 1;
}

int Color_set_red(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromRgb(lua_tointeger(L, 2),
                               color->getGreen(),
                               color->getBlue(),
                               color->getAlpha());
  return 0;
}

int Color_set_green(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromRgb(color->getRed(),
                               lua_tointeger(L, 2),
                               color->getBlue(),
                               color->getAlpha());
  return 0;
}

int Color_set_blue(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromRgb(color->getRed(),
                               color->getGreen(),
                               lua_tointeger(L, 2),
                               color->getAlpha());
  return 0;
}

int Color_set_alpha(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  color->setAlpha(lua_tointeger(L, 2));
  return 0;
}

int Color_set_hsvHue(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsv(lua_tonumber(L, 2),
                               color->getHsvSaturation(),
                               color->getHsvValue(),
                               color->getAlpha());
  return 0;
}

int Color_set_hsvSaturation(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsv(color->getHsvHue(),
                               lua_tonumber(L, 2),
                               color->getHsvValue(),
                               color->getAlpha());
  return 0;
}

int Color_set_hsvValue(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsv(color->getHsvHue(),
                               color->getHsvSaturation(),
                               lua_tonumber(L, 2),
                               color->getAlpha());
  return 0;
}

int Color_set_hslHue(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsl(lua_tonumber(L, 2),
                               color->getHslSaturation(),
                               color->getHslLightness(),
                               color->getAlpha());
  return 0;
}

int Color_set_hslSaturation(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsl(color->getHslHue(),
                               lua_tonumber(L, 2),
                               color->getHslLightness(),
                               color->getAlpha());
  return 0;
}

int Color_set_hslLightness(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsl(color->getHslHue(),
                               color->getHslSaturation(),
                               lua_tonumber(L, 2),
                               color->getAlpha());
  return 0;
}

int Color_set_hue(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  if (color->getType() == Color::HslType)
    return Color_set_hslHue(L);
  else
    return Color_set_hsvHue(L);
}

int Color_set_saturation(lua_State* L)
{
  const auto color = get_obj<app::Color>(L, 1);
  if (color->getType() == Color::HslType)
    return Color_set_hslSaturation(L);
  else
    return Color_set_hsvSaturation(L);
}

int Color_set_gray(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromGray(lua_tointeger(L, 2),
                                color->getAlpha());
  return 0;
}

int Color_set_index(lua_State* L)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromIndex(lua_tointeger(L, 2));
  return 0;
}

const luaL_Reg Color_methods[] = {
  { "__gc", Color_gc },
  { "__eq", Color_eq },
  { nullptr, nullptr }
};

const Property Color_properties[] = {
  { "red", Color_get_red, Color_set_red },
  { "green", Color_get_green, Color_set_green },
  { "blue", Color_get_blue, Color_set_blue },
  { "alpha", Color_get_alpha, Color_set_alpha },
  { "hsvHue", Color_get_hsvHue, Color_set_hsvHue },
  { "hsvSaturation", Color_get_hsvSaturation, Color_set_hsvSaturation },
  { "hsvValue", Color_get_hsvValue, Color_set_hsvValue },
  { "hslHue", Color_get_hslHue, Color_set_hslHue },
  { "hslSaturation", Color_get_hslSaturation, Color_set_hslSaturation },
  { "hslLightness", Color_get_hslLightness, Color_set_hslLightness },
  { "hue", Color_get_hue, Color_set_hue },
  { "saturation", Color_get_saturation, Color_set_saturation },
  { "value", Color_get_hsvValue, Color_set_hsvValue },
  { "lightness", Color_get_hslLightness, Color_set_hslLightness },
  { "index", Color_get_index, Color_set_index },
  { "gray", Color_get_gray, Color_set_gray },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(app::Color);

void register_color_class(lua_State* L)
{
  REG_CLASS(L, Color);
  REG_CLASS_NEW(L, Color);
  REG_CLASS_PROPERTIES(L, Color);
}

app::Color convert_args_into_color(lua_State* L, int index)
{
  return Color_new(L, index);
}

doc::color_t convert_args_into_pixel_color(lua_State* L, int index)
{
  app::Color color = convert_args_into_color(L, index);
  return color_utils::color_for_image(color, doc::IMAGE_RGB);
}

} // namespace script
} // namespace app
