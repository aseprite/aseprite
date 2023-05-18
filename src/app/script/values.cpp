// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
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
#include "doc/remap.h"

#include <any>
#include <cstddef>
#include <variant>

namespace app {
namespace script {

// TODO this is similar to app::Param<> specializations::fromLua() specializations

// ----------------------------------------------------------------------
// nullptr_t

template<>
void push_value_to_lua(lua_State* L, const std::nullptr_t&) {
  TRACEARGS("push_value_to_lua nullptr_t");
  lua_pushnil(L);
}

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
void push_value_to_lua(lua_State* L, const int8_t& value) { lua_pushinteger(L, value); }
template<>
void push_value_to_lua(lua_State* L, const int16_t& value) { lua_pushinteger(L, value); }
template<>
void push_value_to_lua(lua_State* L, const int32_t& value) { lua_pushinteger(L, value); }
template<>
void push_value_to_lua(lua_State* L, const int64_t& value) { lua_pushinteger(L, value); }
template<>
void push_value_to_lua(lua_State* L, const uint8_t& value) { lua_pushinteger(L, value); }
template<>
void push_value_to_lua(lua_State* L, const uint16_t& value) { lua_pushinteger(L, value); }
template<>
void push_value_to_lua(lua_State* L, const uint32_t& value) { lua_pushinteger(L, value); }
template<>
void push_value_to_lua(lua_State* L, const uint64_t& value) { lua_pushinteger(L, value); }

template<>
int get_value_from_lua(lua_State* L, int index) {
  return lua_tointeger(L, index);
}

// ----------------------------------------------------------------------
// float

template<>
void push_value_to_lua(lua_State* L, const float& value) {
  lua_pushnumber(L, value);
}

template<>
float get_value_from_lua(lua_State* L, int index) {
  return lua_tonumber(L, index);
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
// fixed

template<>
void push_value_to_lua(lua_State* L, const doc::UserData::Fixed& value) {
  lua_pushnumber(L, fixmath::fixtof(value.value));
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
// doc::Remap

template<>
void push_value_to_lua(lua_State* L, const doc::Remap& value) {
  lua_newtable(L);
  for (int i=0; i<value.size(); ++i) {
    lua_pushinteger(L, value[i]);

    // This will be a weird Lua table where the base index start at 0,
    // anyway the tile=0 cannot be remapped, so it doesn't contain
    // useful information anyway. The idea here is that the user can
    // do something like this:
    //
    //   newTileIndex = remap[oldTileIndex]
    //
    // And it should just work.
    lua_seti(L, -2, i);
  }
}

// ----------------------------------------------------------------------
// app::Params

template<>
void push_value_to_lua(lua_State* L, const Params& params) {
  lua_newtable(L);
  for (const auto& param : params) {
    lua_pushstring(L, param.first.c_str());
    lua_pushstring(L, param.second.c_str());
    lua_rawset(L, -3);
  }
}

// ----------------------------------------------------------------------
// std::any

template<>
void push_value_to_lua(lua_State* L, const std::any& value) {
  if (!value.has_value())
    lua_pushnil(L);
  else if (auto v = std::any_cast<bool>(&value))
    push_value_to_lua(L, *v);
  else if (auto v = std::any_cast<int>(&value))
    push_value_to_lua(L, *v);
  else if (auto v = std::any_cast<std::string>(&value))
    push_value_to_lua(L, *v);
  else if (auto v = std::any_cast<lua_CFunction>(&value))
    lua_pushcfunction(L, *v);
  else if (auto v = std::any_cast<const doc::Remap*>(&value))
    push_value_to_lua(L, **v);
  else if (auto v = std::any_cast<const doc::Tileset*>(&value))
    push_tileset(L, *v);
  else if (auto v = std::any_cast<const Params>(&value)) {
    push_value_to_lua(L, *v);
  }
  else {
    ASSERT(false);
    throw std::runtime_error("Cannot convert type inside std::any");
  }
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
// Uuid

template<>
void push_value_to_lua(lua_State* L, const base::Uuid& value) {
  push_obj(L, value);
}

template<>
base::Uuid get_value_from_lua(lua_State* L, int index) {
  return convert_args_into_uuid(L, index);
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
// doc::tile_t

#if 0 // doc::tile_t matches uint32_t, and we have the uint32_t version already defined
template<>
void push_value_to_lua(lua_State* L, const doc::tile_t& value) {
  lua_pushinteger(L, value);
}
#endif

template<>
doc::tile_t get_value_from_lua(lua_State* L, int index) {
  return lua_tointeger(L, index);
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
FOR_ENUM(app::gen::Downsampling)
FOR_ENUM(app::gen::EyedropperChannel)
FOR_ENUM(app::gen::EyedropperSample)
FOR_ENUM(app::gen::FillReferTo)
FOR_ENUM(app::gen::OnionskinType)
FOR_ENUM(app::gen::PaintingCursorType)
FOR_ENUM(app::gen::PivotPosition)
FOR_ENUM(app::gen::PixelConnectivity)
FOR_ENUM(app::gen::RightClickMode)
FOR_ENUM(app::gen::SelectionMode)
FOR_ENUM(app::gen::SequenceDecision)
FOR_ENUM(app::gen::StopAtGrid)
FOR_ENUM(app::gen::SymmetryMode)
FOR_ENUM(app::gen::TimelinePosition)
FOR_ENUM(app::gen::ToGrayAlgorithm)
FOR_ENUM(app::gen::WindowColorProfile)
FOR_ENUM(app::tools::FreehandAlgorithm)
FOR_ENUM(app::tools::RotationAlgorithm)
FOR_ENUM(doc::AniDir)
FOR_ENUM(doc::BrushPattern)
FOR_ENUM(doc::ColorMode)
FOR_ENUM(doc::RgbMapAlgorithm)
FOR_ENUM(filters::HueSaturationFilter::Mode)
FOR_ENUM(filters::TiledMode)
FOR_ENUM(render::OnionskinPosition)

// ----------------------------------------------------------------------
// UserData::Properties / Variant

template<>
void push_value_to_lua(lua_State* L, const doc::UserData::Properties& value);
template<>
void push_value_to_lua(lua_State* L, const doc::UserData::Vector& value);

template<>
doc::UserData::Properties get_value_from_lua(lua_State* L, int index);
template<>
doc::UserData::Vector get_value_from_lua(lua_State* L, int index);

template<>
void push_value_to_lua(lua_State* L, const doc::UserData::Variant& value)
{
#if 1 // We are targetting macOS 10.9, so we don't have the std::visit() available
  switch (value.type()) {
    case USER_DATA_PROPERTY_TYPE_NULLPTR:
      push_value_to_lua<std::nullptr_t>(L, nullptr);
      break;
    case USER_DATA_PROPERTY_TYPE_BOOL:
      push_value_to_lua(L, *std::get_if<bool>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_INT8:
      push_value_to_lua(L, *std::get_if<int8_t>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_UINT8:
      push_value_to_lua(L, *std::get_if<uint8_t>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_INT16:
      push_value_to_lua(L, *std::get_if<int16_t>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_UINT16:
      push_value_to_lua(L, *std::get_if<uint16_t>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_INT32:
      push_value_to_lua(L, *std::get_if<int32_t>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_UINT32:
      push_value_to_lua(L, *std::get_if<uint32_t>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_INT64:
      push_value_to_lua(L, *std::get_if<int64_t>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_UINT64:
      push_value_to_lua(L, *std::get_if<uint64_t>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_FIXED:
      push_value_to_lua(L, *std::get_if<doc::UserData::Fixed>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_FLOAT:
      push_value_to_lua(L, *std::get_if<float>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_DOUBLE:
      push_value_to_lua(L, *std::get_if<double>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_STRING:
      push_value_to_lua(L, *std::get_if<std::string>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_POINT:
      push_value_to_lua(L, *std::get_if<gfx::Point>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_SIZE:
      push_value_to_lua(L, *std::get_if<gfx::Size>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_RECT:
      push_value_to_lua(L, *std::get_if<gfx::Rect>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_VECTOR:
      push_value_to_lua(L, *std::get_if<doc::UserData::Vector>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_PROPERTIES:
      push_value_to_lua(L, *std::get_if<doc::UserData::Properties>(&value));
      break;
    case USER_DATA_PROPERTY_TYPE_UUID:
      push_value_to_lua(L, *std::get_if<base::Uuid>(&value));
      break;
  }
#else // TODO enable this in the future
  std::visit([L](auto&& v){ push_value_to_lua(L, v); }, value);
#endif
}

template<>
doc::UserData::Variant get_value_from_lua(lua_State* L, int index)
{
  doc::UserData::Variant v;

  switch (lua_type(L, index)) {

    case LUA_TNONE:
    case LUA_TNIL:
      v = nullptr;
      break;

    case LUA_TBOOLEAN:
      v = (lua_toboolean(L, index) ? true: false);
      break;

    case LUA_TNUMBER:
      if (lua_isinteger(L, index)) {
        // This is required because some compilers/stdc++ impls
        // (clang-10 + libstdc++ 7.5.0) don't convert "long long" type
        // to "int64_t" automatically (?)
        if constexpr (sizeof(lua_Integer) == 8) {
          v = (int64_t)lua_tointeger(L, index);
        }
        else if constexpr (sizeof(lua_Integer) == 4) {
          v = (int32_t)lua_tointeger(L, index);
        }
        else {
          static_assert((sizeof(lua_Integer) == 8 ||
                         sizeof(lua_Integer) == 4), "Invalid lua_Integer size");
        }
      }
      else {
        v = lua_tonumber(L, index);
      }
      break;

    case LUA_TSTRING:
      v = std::string(lua_tostring(L, index));
      break;

    case LUA_TTABLE: {
      int i = 0;
      bool isArray = true;
      if (index < 0)
        --index;
      lua_pushnil(L);
      while (lua_next(L, index) != 0) {
        if (lua_isinteger(L, -2)) {
          if (++i != lua_tointeger(L, -2)) {
            isArray = false;
            lua_pop(L, 2);  // Pop value and key
            break;
          }
        }
        else {
          isArray = false;
          lua_pop(L, 2);
          break;
        }
        lua_pop(L, 1); // Pop the value, leave the key for lua_next()
      }
      if (index < 0)
        ++index;
      if (isArray) {
        v = get_value_from_lua<doc::UserData::Vector>(L, index);
      }
      else {
        v = get_value_from_lua<doc::UserData::Properties>(L, index);
      }
      break;
    }

    case LUA_TUSERDATA: {
      if (auto rect = may_get_obj<gfx::Rect>(L, index)) {
        v = *rect;
      }
      else if (auto pt = may_get_obj<gfx::Point>(L, index)) {
        v = *pt;
      }
      else if (auto sz = may_get_obj<gfx::Size>(L, index)) {
        v = *sz;
      }
      else if (auto uuid = may_get_obj<base::Uuid>(L, index)) {
        v = *uuid;
      }
      break;
    }
  }

  return v;
}

template<>
void push_value_to_lua(lua_State* L, const doc::UserData::Properties& value)
{
  lua_newtable(L);
  for (const auto& kv : value) {
    push_value_to_lua(L, kv.second);
    lua_setfield(L, -2, kv.first.c_str());
  }
}

template<>
void push_value_to_lua(lua_State* L, const doc::UserData::Vector& value)
{
  int i = 0;
  lua_newtable(L);
  for (const auto& kv : value) {
    push_value_to_lua(L, kv);
    lua_seti(L, -2, ++i);
  }
}

template<>
doc::UserData::Properties get_value_from_lua(lua_State* L, int index)
{
  doc::UserData::Properties m;

  if (index < 0)
    --index;
  lua_pushnil(L);
  while (lua_next(L, index) != 0) {
    if (auto k = lua_tostring(L, -2))
      m[k] = get_value_from_lua<doc::UserData::Variant>(L, -1);
    lua_pop(L, 1);
  }

  return m;
}

template<>
doc::UserData::Vector get_value_from_lua(lua_State* L, int index)
{
  doc::UserData::Vector v;

  lua_len(L, index);
  int len = lua_tointeger(L, -1);
  lua_pop(L, 1);
  if (len > 0)
    v.reserve(len);

  if (index < 0)
    --index;
  lua_pushnil(L);
  while (lua_next(L, index) != 0) {
    v.push_back(get_value_from_lua<doc::UserData::Variant>(L, -1));
    lua_pop(L, 1);
  }

  return v;
}

} // namespace script
} // namespace app
