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

using namespace doc;

namespace {

struct FramesObj {
  Sprite* sprite;
  FramesObj(Sprite* sprite)
    : sprite(sprite) {
  }
  FramesObj(const FramesObj&) = delete;
  FramesObj& operator=(const FramesObj&) = delete;
};

int Frames_gc(lua_State* L)
{
  get_obj<FramesObj>(L, 1)->~FramesObj();
  return 0;
}

int Frames_len(lua_State* L)
{
  auto obj = get_obj<FramesObj>(L, 1);
  lua_pushinteger(L, obj->sprite->totalFrames());
  return 1;
}

int Frames_index(lua_State* L)
{
  auto obj = get_obj<FramesObj>(L, 1);
  const int i = lua_tonumber(L, 2);
  if (i >= 1 && i <= obj->sprite->totalFrames())
    push_sprite_frame(L, obj->sprite, i-1);
  else
    lua_pushnil(L);
  return 1;
}

const luaL_Reg Frames_methods[] = {
  { "__gc", Frames_gc },
  { "__len", Frames_len },
  { "__index", Frames_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(FramesObj);

void register_frames_class(lua_State* L)
{
  using Frames = FramesObj;
  REG_CLASS(L, Frames);
}

void push_sprite_frames(lua_State* L, Sprite* sprite)
{
  push_new<FramesObj>(L, sprite);
}

} // namespace script
} // namespace app
