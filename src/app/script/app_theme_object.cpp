// Aseprite
// Copyright (c) 2022-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color.h"
#include "app/color_utils.h"
#include "app/script/luacpp.h"
#include "app/ui/skin/skin_theme.h"

namespace app {
namespace script {

namespace {

struct Theme { };
struct ThemeDimension { };
struct ThemeColor { };

int ThemeDimension_index(lua_State* L)
{
  const char* id = lua_tostring(L, 2);
  if (!id)
    return luaL_error(L, "id in app.theme.dimension.id must be a string");

#ifdef ENABLE_UI
  const int value = skin::SkinTheme::instance()->getDimensionById(id);
  lua_pushinteger(L, value);
#else
  lua_pushinteger(L, 0);
#endif
  return 1;
}

int ThemeColor_index(lua_State* L)
{
  const char* id = lua_tostring(L, 2);
  if (!id)
    return luaL_error(L, "id in app.theme.color.id must be a string");

#ifdef ENABLE_UI
  const gfx::Color uiColor = skin::SkinTheme::instance()->getColorById(id);
  push_obj<app::Color>(L, color_utils::color_from_ui(uiColor));
#else
  push_obj<app::Color>(L, app::Color::fromMask());
#endif
  return 1;
}

int Theme_styleMetrics(lua_State* L)
{
  [[maybe_unused]]
  auto t = get_obj<Theme>(L, 1);

  const char* id = lua_tostring(L, 2);
  if (!id)
    return 0;

  auto theme = skin::SkinTheme::instance();
  if (!theme)
    return 0;

  ui::Style* style = theme->getStyleById(id);
  if (!style)
    return 0;

  ui::Widget widget(ui::kGenericWidget);
  gfx::Border border = theme->calcBorder(&widget, style);

  lua_newtable(L);
  {
    lua_newtable(L);
    lua_pushinteger(L, border.left());
    lua_setfield(L, -2, "left");
    lua_pushinteger(L, border.top());
    lua_setfield(L, -2, "top");
    lua_pushinteger(L, border.right());
    lua_setfield(L, -2, "right");
    lua_pushinteger(L, border.bottom());
    lua_setfield(L, -2, "bottom");
  }
  lua_setfield(L, -2, "border");

  return 1;
}

int Theme_get_dimension(lua_State* L)
{
  push_obj<ThemeDimension>(L, ThemeDimension());
  return 1;
}

int Theme_get_color(lua_State* L)
{
  push_obj<ThemeColor>(L, ThemeColor());
  return 1;
}

const luaL_Reg Theme_methods[] = {
  { "styleMetrics", Theme_styleMetrics },
  { nullptr, nullptr }
};

const Property Theme_properties[] = {
  { "dimension", Theme_get_dimension, nullptr },
  { "color", Theme_get_color, nullptr },
  { nullptr, nullptr, nullptr }
};

const luaL_Reg ThemeDimension_methods[] = {
  { "__index", ThemeDimension_index },
  { nullptr, nullptr }
};

const luaL_Reg ThemeColor_methods[] = {
  { "__index", ThemeColor_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Theme);
DEF_MTNAME(ThemeDimension);
DEF_MTNAME(ThemeColor);

void register_theme_classes(lua_State* L)
{
  REG_CLASS(L, Theme);
  REG_CLASS(L, ThemeDimension);
  REG_CLASS(L, ThemeColor);
  REG_CLASS_PROPERTIES(L, Theme);
}

void push_app_theme(lua_State* L)
{
  push_obj<Theme>(L, Theme());
}

} // namespace script
} // namespace app
