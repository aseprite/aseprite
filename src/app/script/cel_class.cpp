// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "doc/cel.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

int Cel_get_sprite(lua_State* L)
{
  auto cel = get_ptr<Cel>(L, 1);
  push_ptr(L, cel->sprite());
  return 1;
}

int Cel_get_layer(lua_State* L)
{
  auto cel = get_ptr<Cel>(L, 1);
  push_ptr<Layer>(L, (Layer*)cel->layer());
  return 1;
}

int Cel_get_frame(lua_State* L)
{
  auto cel = get_ptr<Cel>(L, 1);
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

int Cel_get_userData(lua_State* L)
{
  auto cel = get_ptr<Cel>(L, 1);
  push_userdata(L, cel->data());
  return 1;
}

const luaL_Reg Cel_methods[] = {
  { nullptr, nullptr }
};

const Property Cel_properties[] = {
  { "sprite", Cel_get_sprite, nullptr },
  { "layer", Cel_get_layer, nullptr },
  { "frame", Cel_get_frame, nullptr },
  { "image", Cel_get_image, nullptr },
  { "bounds", Cel_get_bounds, nullptr },
  { "userData", Cel_get_userData, nullptr },
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
