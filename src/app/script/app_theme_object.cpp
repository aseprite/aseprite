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

class Theme {
public:
  Theme(int uiscale) : m_uiscale(uiscale) { }

  gfx::Border styleMetrics(const std::string& id) const {
    auto theme = skin::SkinTheme::instance();
    if (!theme) return gfx::Border(0);

    ui::Style* style = theme->getStyleById(id);
    if (!style) return gfx::Border(0);

    ui::Widget widget(ui::kGenericWidget);
    auto border = theme->calcBorder(&widget, style);
    if (m_uiscale > 1)
      border /= m_uiscale;

    return border;
  }

  int getDimensionById(const std::string& id) const {
    int value = skin::SkinTheme::instance()->getDimensionById(id);
    if (m_uiscale > 1)
      value /= m_uiscale;
    return value;
  }

private:
  int m_uiscale;
};

struct ThemeDimension {
  Theme* theme;

  ThemeDimension(Theme* theme) : theme(theme) { }

  int getById(const std::string& id) const {
    return theme->getDimensionById(id);
  }
};

struct ThemeColor { };

#ifdef ENABLE_UI
void push_border_as_table(lua_State* L, const gfx::Border& border)
{
  lua_newtable(L);
  {
    lua_newtable(L);
    setfield_integer(L, "left", border.left());
    setfield_integer(L, "top", border.top());
    setfield_integer(L, "right", border.right());
    setfield_integer(L, "bottom", border.bottom());
  }
  lua_setfield(L, -2, "border");
}
#endif

int ThemeDimension_index(lua_State* L)
{
  [[maybe_unused]]
  auto themeDimension = get_obj<ThemeDimension>(L, 1);

  const char* id = lua_tostring(L, 2);
  if (!id)
    return luaL_error(L, "id in app.theme.dimension.id must be a string");

#ifdef ENABLE_UI
  const int value = themeDimension->getById(id);
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
  auto theme = get_obj<Theme>(L, 1);

  const char* id = lua_tostring(L, 2);
  if (!id)
    return 0;

#ifdef ENABLE_UI
  gfx::Border border = theme->styleMetrics(id);
  push_border_as_table(L, border);
  return 1;
#else  // ENABLE_UI
  return 0;
#endif
}

int Theme_get_dimension(lua_State* L)
{
  auto theme = get_obj<Theme>(L, 1);
  push_new<ThemeDimension>(L, theme);
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

void push_app_theme(lua_State* L, int uiscale)
{
  push_new<Theme>(L, uiscale);
}

} // namespace script
} // namespace app
