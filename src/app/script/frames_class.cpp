// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

struct FramesObj {
  ObjectId spriteId;
  const std::vector<frame_t>* frames;

  FramesObj(Sprite* sprite)
    : spriteId(sprite->id()),
      frames(nullptr) {
  }
  FramesObj(Sprite* sprite, const std::vector<frame_t>& frames)
    : spriteId(sprite->id()),
      frames(new std::vector<frame_t>(frames)) {
  }

  ~FramesObj() {
    delete frames;
  }

  FramesObj(const FramesObj&) = delete;
  FramesObj& operator=(const FramesObj&) = delete;

  Sprite* sprite(lua_State* L) { return check_docobj(L, doc::get<Sprite>(spriteId)); }
};

int Frames_gc(lua_State* L)
{
  get_obj<FramesObj>(L, 1)->~FramesObj();
  return 0;
}

int Frames_len(lua_State* L)
{
  auto obj = get_obj<FramesObj>(L, 1);
  if (obj->frames)
    lua_pushinteger(L, int(obj->frames->size()));
  else
    lua_pushinteger(L, obj->sprite(L)->totalFrames());
  return 1;
}

int Frames_index(lua_State* L)
{
  auto obj = get_obj<FramesObj>(L, 1);
  auto sprite = obj->sprite(L);
  int i = lua_tonumber(L, 2);

  if (obj->frames) {
    if (i >= 1 && i <= obj->frames->size())
      i = (*obj->frames)[i-1]+1;
    else
      i = 0;
  }

  if (i >= 1 && i <= sprite->totalFrames())
    push_sprite_frame(L, sprite, i-1);
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

void push_sprite_frames(lua_State* L, doc::Sprite* sprite, const std::vector<doc::frame_t>& frames)
{
  push_new<FramesObj>(L, sprite, frames);
}

} // namespace script
} // namespace app
