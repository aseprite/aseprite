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
#include "doc/sprite.h"

namespace app {
namespace script {

namespace {

struct SlicesObj {
  Sprite* sprite;
  SlicesObj(Sprite* sprite)
    : sprite(sprite) {
  }
  SlicesObj(const SlicesObj&) = delete;
  SlicesObj& operator=(const SlicesObj&) = delete;
};

int Slices_gc(lua_State* L)
{
  get_obj<SlicesObj>(L, 1)->~SlicesObj();
  return 0;
}

int Slices_len(lua_State* L)
{
  auto obj = get_obj<SlicesObj>(L, 1);
  lua_pushinteger(L, obj->sprite->slices().size());
  return 1;
}

int Slices_index(lua_State* L)
{
  auto obj = get_obj<SlicesObj>(L, 1);
  auto& slices = obj->sprite->slices();
  const int i = lua_tonumber(L, 2);
  if (i < 1 || i > int(slices.size()))
    return luaL_error(L, "index out of bounds %d", i);
  push_ptr<Slice>(L, *(slices.begin()+i-1));
  return 1;
}

const luaL_Reg Slices_methods[] = {
  { "__gc", Slices_gc },
  { "__len", Slices_len },
  { "__index", Slices_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(SlicesObj);

void register_slices_class(lua_State* L)
{
  using Slices = SlicesObj;
  REG_CLASS(L, Slices);
}

void push_sprite_slices(lua_State* L, Sprite* sprite)
{
  push_new<SlicesObj>(L, sprite);
}

} // namespace script
} // namespace app
