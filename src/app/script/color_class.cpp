// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
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

namespace app { namespace script {

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
      color =
        app::Color::fromRgb(lua_tointeger(L, -4), lua_tointeger(L, -3), lua_tointeger(L, -2), a);
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
      color =
        app::Color::fromRgb(lua_tointeger(L, -4), lua_tointeger(L, -3), lua_tointeger(L, -2), a);
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
      color = app::Color::fromHsv(lua_tonumber(L, -2), lua_tonumber(L, -3), lua_tonumber(L, -4), a);
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
      color = app::Color::fromHsv(lua_tonumber(L, -2), lua_tonumber(L, -3), lua_tonumber(L, -4), a);
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
      color = app::Color::fromHsl(lua_tonumber(L, -2), lua_tonumber(L, -3), lua_tonumber(L, -4), a);
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
      color = app::Color::fromHsl(lua_tonumber(L, -2), lua_tonumber(L, -3), lua_tonumber(L, -4), a);
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

    // Convert { index } into a Color
    if (lua_getfield(L, index, "index") != LUA_TNIL) {
      int i = lua_tointeger(L, -1);
      color = app::Color::fromIndex(i);
      lua_pop(L, 1);
      return color;
    }
    else
      lua_pop(L, 1);

    // Convert { tile } into a Color
    if (lua_getfield(L, index, "tile") != LUA_TNIL) {
      tile_t t = lua_tointeger(L, -1);
      color = app::Color::fromTile(t);
      lua_pop(L, 1);
      return color;
    }
    else
      lua_pop(L, 1);
  }
  // raw color into app color
  else if (!lua_isnone(L, index)) {
    if (lua_isinteger(L, index) && (index < 0 || lua_isnone(L, index + 1))) {
      doc::color_t docColor = lua_tointeger(L, index);

      // TODO depending on current pixel format?
      switch (app_get_current_pixel_format()) {
        case IMAGE_RGB:
          color = app::Color::fromRgb(doc::rgba_getr(docColor),
                                      doc::rgba_getg(docColor),
                                      doc::rgba_getb(docColor),
                                      doc::rgba_geta(docColor));
          break;
        case IMAGE_GRAYSCALE:
          color = app::Color::fromGray(doc::graya_getv(docColor), doc::graya_geta(docColor));
          break;
        case IMAGE_INDEXED: color = app::Color::fromIndex(docColor); break;
      }
    }
    else if (index >= 0) {
      color = app::Color::fromRgb(lua_tointeger(L, index),
                                  lua_tointeger(L, index + 1),
                                  lua_tointeger(L, index + 2),
                                  lua_isnone(L, index + 3) ? 255 : lua_tointeger(L, index + 3));
    }
  }
  return color;
}

LUA_FUNCTION(Color_new)
{
  push_obj(L, Color_new(L, 1));
  return 1;
}

LUA_FUNCTION(Color_gc)
{
  get_obj<app::Color>(L, 1)->~Color();
  return 0;
}

LUA_FUNCTION(Color_eq)
{
  const auto a = get_obj<app::Color>(L, 1);
  const auto b = get_obj<app::Color>(L, 2);
  lua_pushboolean(L, *a == *b);
  return 1;
}

LUA_FUNCTION(Color_get_red)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushinteger(L, color->getRed());
  return 1;
}

LUA_FUNCTION(Color_get_green)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushinteger(L, color->getGreen());
  return 1;
}

LUA_FUNCTION(Color_get_blue)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushinteger(L, color->getBlue());
  return 1;
}

LUA_FUNCTION(Color_get_alpha)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushinteger(L, color->getAlpha());
  return 1;
}

LUA_FUNCTION(Color_get_hsvHue)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHsvHue());
  return 1;
}

LUA_FUNCTION(Color_get_hsvSaturation)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHsvSaturation());
  return 1;
}

LUA_FUNCTION(Color_get_hsvValue)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHsvValue());
  return 1;
}

LUA_FUNCTION(Color_get_hslHue)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHslHue());
  return 1;
}

LUA_FUNCTION(Color_get_hslSaturation)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHslSaturation());
  return 1;
}

LUA_FUNCTION(Color_get_hslLightness)
{
  const auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getHslLightness());
  return 1;
}

LUA_FUNCTION(Color_get_hue)
{
  const auto color = get_obj<app::Color>(L, 1);
  if (color->getType() == Color::HslType)
    return Color_get_hslHue(L);
  else
    return Color_get_hsvHue(L);
}

LUA_FUNCTION(Color_get_saturation)
{
  const auto color = get_obj<app::Color>(L, 1);
  if (color->getType() == Color::HslType)
    return Color_get_hslSaturation(L);
  else
    return Color_get_hsvSaturation(L);
}

LUA_FUNCTION(Color_get_gray)
{
  auto color = get_obj<app::Color>(L, 1);
  lua_pushnumber(L, color->getGray());
  return 1;
}

LUA_FUNCTION(Color_get_index)
{
  auto color = get_obj<app::Color>(L, 1);
  lua_pushinteger(L, color->getIndex());
  return 1;
}

LUA_FUNCTION(Color_get_rgbaPixel)
{
  auto color = get_obj<app::Color>(L, 1);
  auto pixelColor = color_utils::color_for_target_mask(
    *color,
    ColorTarget(ColorTarget::TransparentLayer, IMAGE_RGB, 0));
  lua_pushinteger(L, pixelColor);
  return 1;
}

LUA_FUNCTION(Color_get_grayPixel)
{
  auto color = get_obj<app::Color>(L, 1);
  auto pixelColor = color_utils::color_for_target_mask(
    *color,
    ColorTarget(ColorTarget::TransparentLayer, IMAGE_GRAYSCALE, 0));
  lua_pushinteger(L, pixelColor);
  return 1;
}

LUA_FUNCTION(Color_set_red)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromRgb(lua_tointeger(L, 2),
                               color->getGreen(),
                               color->getBlue(),
                               color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_green)
{
  auto color = get_obj<app::Color>(L, 1);
  *color =
    app::Color::fromRgb(color->getRed(), lua_tointeger(L, 2), color->getBlue(), color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_blue)
{
  auto color = get_obj<app::Color>(L, 1);
  *color =
    app::Color::fromRgb(color->getRed(), color->getGreen(), lua_tointeger(L, 2), color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_alpha)
{
  auto color = get_obj<app::Color>(L, 1);
  color->setAlpha(lua_tointeger(L, 2));
  return 0;
}

LUA_FUNCTION(Color_set_hsvHue)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsv(lua_tonumber(L, 2),
                               color->getHsvSaturation(),
                               color->getHsvValue(),
                               color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_hsvSaturation)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsv(color->getHsvHue(),
                               lua_tonumber(L, 2),
                               color->getHsvValue(),
                               color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_hsvValue)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsv(color->getHsvHue(),
                               color->getHsvSaturation(),
                               lua_tonumber(L, 2),
                               color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_hslHue)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsl(lua_tonumber(L, 2),
                               color->getHslSaturation(),
                               color->getHslLightness(),
                               color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_hslSaturation)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsl(color->getHslHue(),
                               lua_tonumber(L, 2),
                               color->getHslLightness(),
                               color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_hslLightness)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromHsl(color->getHslHue(),
                               color->getHslSaturation(),
                               lua_tonumber(L, 2),
                               color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_hue)
{
  const auto color = get_obj<app::Color>(L, 1);
  if (color->getType() == Color::HslType)
    return Color_set_hslHue(L);
  else
    return Color_set_hsvHue(L);
}

LUA_FUNCTION(Color_set_saturation)
{
  const auto color = get_obj<app::Color>(L, 1);
  if (color->getType() == Color::HslType)
    return Color_set_hslSaturation(L);
  else
    return Color_set_hsvSaturation(L);
}

LUA_FUNCTION(Color_set_gray)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromGray(lua_tointeger(L, 2), color->getAlpha());
  return 0;
}

LUA_FUNCTION(Color_set_index)
{
  auto color = get_obj<app::Color>(L, 1);
  *color = app::Color::fromIndex(lua_tointeger(L, 2));
  return 0;
}

const luaL_Reg Color_methods[] = {
  { "__gc",  Color_gc },
  { "__eq",  Color_eq },
  { nullptr, nullptr  }
};

const Property Color_properties[] = {
  { "red",           Color_get_red,           Color_set_red           },
  { "green",         Color_get_green,         Color_set_green         },
  { "blue",          Color_get_blue,          Color_set_blue          },
  { "alpha",         Color_get_alpha,         Color_set_alpha         },
  { "hsvHue",        Color_get_hsvHue,        Color_set_hsvHue        },
  { "hsvSaturation", Color_get_hsvSaturation, Color_set_hsvSaturation },
  { "hsvValue",      Color_get_hsvValue,      Color_set_hsvValue      },
  { "hslHue",        Color_get_hslHue,        Color_set_hslHue        },
  { "hslSaturation", Color_get_hslSaturation, Color_set_hslSaturation },
  { "hslLightness",  Color_get_hslLightness,  Color_set_hslLightness  },
  { "hue",           Color_get_hue,           Color_set_hue           },
  { "saturation",    Color_get_saturation,    Color_set_saturation    },
  { "value",         Color_get_hsvValue,      Color_set_hsvValue      },
  { "lightness",     Color_get_hslLightness,  Color_set_hslLightness  },
  { "index",         Color_get_index,         Color_set_index         },
  { "gray",          Color_get_gray,          Color_set_gray          },
  { "rgbaPixel",     Color_get_rgbaPixel,     nullptr                 },
  { "grayPixel",     Color_get_grayPixel,     nullptr                 },
  { nullptr,         nullptr,                 nullptr                 }
};
DEF_ITERATOR_PAIRS(Color);
} // anonymous namespace

DEF_MTNAME(app::Color);

LUA_FUNCTION(register_color_class)
{
  REG_CLASS(L, Color);
  REG_CLASS_NEW(L, Color);
  REG_CLASS_PROPERTIES(L, Color);
  return 0;
}

app::Color convert_args_into_color(lua_State* L, int index)
{
  return Color_new(L, index);
}

doc::color_t convert_args_into_pixel_color(lua_State* L,
                                           int index,
                                           const doc::PixelFormat pixelFormat)
{
  app::Color color = convert_args_into_color(L, index);
  return color_utils::color_for_image(color, pixelFormat);
}

}} // namespace app::script
