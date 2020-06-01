// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/new_params.h"

#include "app/color.h"
#include "app/doc_exporter.h"
#include "app/sprite_sheet_type.h"
#include "app/tools/ink_type.h"
#include "base/convert_to.h"
#include "base/split_string.h"
#include "base/string.h"
#include "doc/algorithm/resize_image.h"
#include "doc/color_mode.h"
#include "filters/color_curve.h"
#include "filters/hue_saturation_filter.h"
#include "filters/outline_filter.h"
#include "filters/tiled_mode.h"

#ifdef ENABLE_SCRIPTING
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/values.h"
#endif

namespace app {

//////////////////////////////////////////////////////////////////////
// Convert values from strings (e.g. useful for values from gui.xml)
//////////////////////////////////////////////////////////////////////

template<>
void Param<bool>::fromString(const std::string& value)
{
  setValue(value == "1" || value == "true");
}

template<>
void Param<int>::fromString(const std::string& value)
{
  setValue(base::convert_to<int>(value));
}

template<>
void Param<double>::fromString(const std::string& value)
{
  setValue(base::convert_to<double>(value));
}

template<>
void Param<std::string>::fromString(const std::string& value)
{
  setValue(value);
}

template<>
void Param<doc::algorithm::ResizeMethod>::fromString(const std::string& value)
{
  if (base::utf8_icmp(value, "bilinear") == 0)
    setValue(doc::algorithm::RESIZE_METHOD_BILINEAR);
  else if (base::utf8_icmp(value, "rotsprite") == 0)
    setValue(doc::algorithm::RESIZE_METHOD_ROTSPRITE);
  else
    setValue(doc::algorithm::ResizeMethod::RESIZE_METHOD_NEAREST_NEIGHBOR);
}

template<>
void Param<app::SpriteSheetType>::fromString(const std::string& value)
{
  if (value == "horizontal")
    setValue(app::SpriteSheetType::Horizontal);
  else if (value == "vertical")
    setValue(app::SpriteSheetType::Vertical);
  else if (value == "rows")
    setValue(app::SpriteSheetType::Rows);
  else if (value == "columns")
    setValue(app::SpriteSheetType::Columns);
  else if (value == "packed")
    setValue(app::SpriteSheetType::Packed);
  else
    setValue(app::SpriteSheetType::None);
}

template<>
void Param<app::SpriteSheetDataFormat>::fromString(const std::string& value)
{
  // JsonArray, json-array, json_array, etc.
  if (base::utf8_icmp(value, "JsonArray") == 0 ||
      base::utf8_icmp(value, "json-array") == 0 ||
      base::utf8_icmp(value, "json_array") == 0)
    setValue(app::SpriteSheetDataFormat::JsonArray);
  else
    setValue(app::SpriteSheetDataFormat::JsonHash);
}

template<>
void Param<doc::ColorMode>::fromString(const std::string& value)
{
  if (base::utf8_icmp(value, "rgb") == 0)
    setValue(doc::ColorMode::RGB);
  else if (base::utf8_icmp(value, "gray") == 0 ||
           base::utf8_icmp(value, "grayscale") == 0)
    setValue(doc::ColorMode::GRAYSCALE);
  else if (base::utf8_icmp(value, "indexed") == 0)
    setValue(doc::ColorMode::INDEXED);
  else
    setValue(doc::ColorMode::RGB);
}

template<>
void Param<app::Color>::fromString(const std::string& value)
{
  setValue(app::Color::fromString(value));
}

template<>
void Param<filters::TiledMode>::fromString(const std::string& value)
{
  if (base::utf8_icmp(value, "both") == 0)
    setValue(filters::TiledMode::BOTH);
  else if (base::utf8_icmp(value, "x") == 0)
    setValue(filters::TiledMode::X_AXIS);
  else if (base::utf8_icmp(value, "y") == 0)
    setValue(filters::TiledMode::Y_AXIS);
  else
    setValue(filters::TiledMode::NONE);
}

template<>
void Param<filters::OutlineFilter::Place>::fromString(const std::string& value)
{
  if (base::utf8_icmp(value, "inside") == 0)
    setValue(filters::OutlineFilter::Place::Inside);
  else
    setValue(filters::OutlineFilter::Place::Outside);
}

template<>
void Param<filters::OutlineFilter::Matrix>::fromString(const std::string& value)
{
  if (base::utf8_icmp(value, "circle") == 0)
    setValue(filters::OutlineFilter::Matrix::Circle);
  else if (base::utf8_icmp(value, "square") == 0)
    setValue(filters::OutlineFilter::Matrix::Square);
  else if (base::utf8_icmp(value, "horizontal") == 0)
    setValue(filters::OutlineFilter::Matrix::Horizontal);
  else if (base::utf8_icmp(value, "vertical") == 0)
    setValue(filters::OutlineFilter::Matrix::Vertical);
  else
    setValue((filters::OutlineFilter::Matrix)0);
}

template<>
void Param<filters::HueSaturationFilter::Mode>::fromString(const std::string& value)
{
  if (base::utf8_icmp(value, "hsv") == 0 ||
      base::utf8_icmp(value, "hsv_mul") == 0)
    setValue(filters::HueSaturationFilter::Mode::HSV_MUL);
  else if (base::utf8_icmp(value, "hsv_add") == 0)
    setValue(filters::HueSaturationFilter::Mode::HSV_ADD);
  else if (base::utf8_icmp(value, "hsl_add") == 0)
    setValue(filters::HueSaturationFilter::Mode::HSL_ADD);
  else
    setValue(filters::HueSaturationFilter::Mode::HSL_MUL);
}

template<>
void Param<filters::ColorCurve>::fromString(const std::string& value)
{
  filters::ColorCurve curve;
  std::vector<std::string> parts;
  base::split_string(value, parts, ",");
  for (int i=0; i+1<int(parts.size()); i+=2) {
    curve.addPoint(
      gfx::Point(
        base::convert_to<int>(parts[i]),
        base::convert_to<int>(parts[i+1])));
  }
  setValue(curve);
}

template<>
void Param<tools::InkType>::fromString(const std::string& value)
{
  setValue(tools::string_id_to_ink_type(value));
}

//////////////////////////////////////////////////////////////////////
// Convert values from Lua
//////////////////////////////////////////////////////////////////////

#ifdef ENABLE_SCRIPTING

template<>
void Param<bool>::fromLua(lua_State* L, int index)
{
  setValue(lua_toboolean(L, index));
}

template<>
void Param<int>::fromLua(lua_State* L, int index)
{
  setValue(lua_tointeger(L, index));
}

template<>
void Param<double>::fromLua(lua_State* L, int index)
{
  setValue(lua_tonumber(L, index));
}

template<>
void Param<std::string>::fromLua(lua_State* L, int index)
{
  if (const char* s = lua_tostring(L, index))
    setValue(s);
  else
    setValue(std::string());
}

template<>
void Param<doc::algorithm::ResizeMethod>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((doc::algorithm::ResizeMethod)lua_tointeger(L, index));
}

template<>
void Param<app::SpriteSheetType>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((app::SpriteSheetType)lua_tointeger(L, index));
}

template<>
void Param<app::SpriteSheetDataFormat>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((app::SpriteSheetDataFormat)lua_tointeger(L, index));
}

template<>
void Param<doc::ColorMode>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((doc::ColorMode)lua_tointeger(L, index));
}

template<>
void Param<app::Color>::fromLua(lua_State* L, int index)
{
  setValue(script::convert_args_into_color(L, index));
}

template<>
void Param<filters::TiledMode>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((filters::TiledMode)lua_tointeger(L, index));
}

template<>
void Param<filters::OutlineFilter::Place>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((filters::OutlineFilter::Place)lua_tointeger(L, index));
}

template<>
void Param<filters::OutlineFilter::Matrix>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((filters::OutlineFilter::Matrix)lua_tointeger(L, index));
}

template<>
void Param<filters::HueSaturationFilter::Mode>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else
    setValue((filters::HueSaturationFilter::Mode)lua_tointeger(L, index));
}

template<>
void Param<filters::ColorCurve>::fromLua(lua_State* L, int index)
{
  if (lua_type(L, index) == LUA_TSTRING)
    fromString(lua_tostring(L, index));
  else if (lua_type(L, index) == LUA_TTABLE) {
    filters::ColorCurve curve;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
      gfx::Point pt = script::convert_args_into_point(L, -1);
      curve.addPoint(pt);
      lua_pop(L, 1);
    }
    setValue(curve);
  }
}

template<>
void Param<tools::InkType>::fromLua(lua_State* L, int index)
{
  script::get_value_from_lua<tools::InkType>(L, index);
}

void CommandWithNewParamsBase::loadParamsFromLuaTable(lua_State* L, int index)
{
  onResetValues();
  if (lua_istable(L, index)) {
    lua_pushnil(L);
    while (lua_next(L, index) != 0) {
      if (const char* k = lua_tostring(L, -2)) {
        if (ParamBase* p = onGetParam(k))
          p->fromLua(L, -1);
      }
      lua_pop(L, 1);            // Pop the value, leave the key
    }
  }
  m_skipLoadParams = true;
}

#endif  // ENABLE_SCRIPTING

void CommandWithNewParamsBase::onLoadParams(const Params& params)
{
#ifdef ENABLE_SCRIPTING
  if (m_skipLoadParams) {
    m_skipLoadParams = false;
    return;
  }
#endif  // ENABLE_SCRIPTING
  onResetValues();
  for (const auto& pair : params) {
    if (ParamBase* p = onGetParam(pair.first))
      p->fromString(pair.second);
  }
}

} // namespace app
