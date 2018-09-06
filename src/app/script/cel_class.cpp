// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_cel_opacity.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/userdata.h"
#include "doc/cel.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

int Cel_eq(lua_State* L)
{
  const auto a = get_ptr<Cel>(L, 1);
  const auto b = get_ptr<Cel>(L, 2);
  lua_pushboolean(L, a->id() == b->id());
  return 1;
}

int Cel_get_sprite(lua_State* L)
{
  const auto cel = get_ptr<Cel>(L, 1);
  push_ptr(L, cel->sprite());
  return 1;
}

int Cel_get_layer(lua_State* L)
{
  const auto cel = get_ptr<Cel>(L, 1);
  push_ptr<Layer>(L, (Layer*)cel->layer());
  return 1;
}

int Cel_get_frame(lua_State* L)
{
  const auto cel = get_ptr<Cel>(L, 1);
  lua_pushinteger(L, cel->frame()+1);
  return 1;
}

int Cel_get_image(lua_State* L)
{
  auto cel = get_ptr<Cel>(L, 1);
  push_cel_image(L, cel);
  return 1;
}

int Cel_get_bounds(lua_State* L)
{
  const auto cel = get_ptr<Cel>(L, 1);
  push_new<gfx::Rect>(L, cel->bounds());
  return 1;
}

int Cel_get_opacity(lua_State* L)
{
  const auto cel = get_ptr<Cel>(L, 1);
  lua_pushinteger(L, cel->opacity());
  return 1;
}

int Cel_set_opacity(lua_State* L)
{
  auto cel = get_ptr<Cel>(L, 1);
  Tx tx;
  tx(new cmd::SetCelOpacity(cel, lua_tointeger(L, 2)));
  tx.commit();
  return 0;
}

const luaL_Reg Cel_methods[] = {
  { "__eq", Cel_eq },
  { nullptr, nullptr }
};

const Property Cel_properties[] = {
  { "sprite", Cel_get_sprite, nullptr },
  { "layer", Cel_get_layer, nullptr },
  { "frame", Cel_get_frame, nullptr },
  { "image", Cel_get_image, nullptr },
  { "bounds", Cel_get_bounds, nullptr },
  { "opacity", Cel_get_opacity, Cel_set_opacity },
  { "color", UserData_get_color<Cel>, UserData_set_color<Cel> },
  { "data", UserData_get_text<Cel>, UserData_set_text<Cel> },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Cel);

void register_cel_class(lua_State* L)
{
  using Cel = doc::Cel;
  REG_CLASS(L, Cel);
  REG_CLASS_PROPERTIES(L, Cel);
}

void push_sprite_cel(lua_State* L, Cel* cel)
{
  push_ptr<Cel>(L, cel);
}

} // namespace script
} // namespace app
