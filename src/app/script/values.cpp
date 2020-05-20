// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/values.h"

#include "app/pref/preferences.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"

namespace app {
namespace script {

// TODO this is similar to app::Param<> specializations::fromLua() specializations

// ----------------------------------------------------------------------
// bool

template<>
void push_value_to_lua(lua_State* L, const bool& value) {
  lua_pushboolean(L, value);
}

template<>
bool get_value_from_lua(lua_State* L, int index) {
  return lua_toboolean(L, index);
}

// ----------------------------------------------------------------------
// int

template<>
void push_value_to_lua(lua_State* L, const int& value) {
  lua_pushinteger(L, value);
}

template<>
int get_value_from_lua(lua_State* L, int index) {
  return lua_tointeger(L, index);
}

// ----------------------------------------------------------------------
// double

template<>
void push_value_to_lua(lua_State* L, const double& value) {
  lua_pushnumber(L, value);
}

template<>
double get_value_from_lua(lua_State* L, int index) {
  return lua_tonumber(L, index);
}

// ----------------------------------------------------------------------
// std::string

template<>
void push_value_to_lua(lua_State* L, const std::string& value) {
  lua_pushstring(L, value.c_str());
}

template<>
std::string get_value_from_lua(lua_State* L, int index) {
  if (const char* v = lua_tostring(L, index))
    return std::string(v);
  else
    return std::string();
}

// ----------------------------------------------------------------------
// Color

template<>
void push_value_to_lua(lua_State* L, const app::Color& value) {
  push_obj(L, value);
}

template<>
app::Color get_value_from_lua(lua_State* L, int index) {
  return convert_args_into_color(L, index);
}

// ----------------------------------------------------------------------
// Point

template<>
void push_value_to_lua(lua_State* L, const gfx::Point& value) {
  push_obj(L, value);
}

template<>
gfx::Point get_value_from_lua(lua_State* L, int index) {
  return convert_args_into_point(L, index);
}

// ----------------------------------------------------------------------
// Size

template<>
void push_value_to_lua(lua_State* L, const gfx::Size& value) {
  push_obj(L, value);
}

template<>
gfx::Size get_value_from_lua(lua_State* L, int index) {
  return convert_args_into_size(L, index);
}

// ----------------------------------------------------------------------
// Rect

template<>
void push_value_to_lua(lua_State* L, const gfx::Rect& value) {
  push_obj(L, value);
}

template<>
gfx::Rect get_value_from_lua(lua_State* L, int index) {
  return convert_args_into_rect(L, index);
}

// ----------------------------------------------------------------------
// tools::InkType

template<>
void push_value_to_lua(lua_State* L, const app::tools::InkType& inkType) {
  lua_pushinteger(L, (int)inkType);
}

template<>
app::tools::InkType get_value_from_lua(lua_State* L, int index) {
  if (lua_type(L, index) == LUA_TSTRING) {
    if (const char* s = lua_tostring(L, index))
      return app::tools::string_id_to_ink_type(s);
  }
  return (app::tools::InkType)lua_tointeger(L, index);
}

// ----------------------------------------------------------------------
// enums

#define FOR_ENUM(T)                                             \
  template<>                                                    \
  void push_value_to_lua(lua_State* L, const T& value) {        \
    lua_pushinteger(L, static_cast<int>(value));                \
  }                                                             \
                                                                \
  template<>                                                    \
  T get_value_from_lua(lua_State* L, int index) {               \
    return static_cast<T>(lua_tointeger(L, index));             \
  }

FOR_ENUM(app::CelsTarget)
FOR_ENUM(app::ColorBar::ColorSelector)
FOR_ENUM(app::SpriteSheetDataFormat)
FOR_ENUM(app::SpriteSheetType)
FOR_ENUM(app::gen::BgType)
FOR_ENUM(app::gen::BrushPreview)
FOR_ENUM(app::gen::BrushType)
FOR_ENUM(app::gen::ColorProfileBehavior)
FOR_ENUM(app::gen::EyedropperChannel)
FOR_ENUM(app::gen::EyedropperSample)
FOR_ENUM(app::gen::FillReferTo)
FOR_ENUM(app::gen::OnionskinType)
FOR_ENUM(app::gen::PaintingCursorType)
FOR_ENUM(app::gen::PivotPosition)
FOR_ENUM(app::gen::PixelConnectivity)
FOR_ENUM(app::gen::RightClickMode)
FOR_ENUM(app::gen::SelectionMode)
FOR_ENUM(app::gen::StopAtGrid)
FOR_ENUM(app::gen::SymmetryMode)
FOR_ENUM(app::gen::TimelinePosition)
FOR_ENUM(app::gen::WindowColorProfile)
FOR_ENUM(app::gen::ToGrayAlgorithm)
FOR_ENUM(app::tools::FreehandAlgorithm)
FOR_ENUM(app::tools::RotationAlgorithm)
FOR_ENUM(doc::AniDir)
FOR_ENUM(doc::BrushPattern)
FOR_ENUM(doc::ColorMode)
FOR_ENUM(filters::HueSaturationFilter::Mode)
FOR_ENUM(filters::TiledMode)
FOR_ENUM(render::OnionskinPosition)

} // namespace script
} // namespace app
