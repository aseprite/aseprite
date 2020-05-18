// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_frame_duration.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/tx.h"
#include "doc/frame.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

struct FrameObj {
  ObjectId spriteId;
  frame_t frame;
  FrameObj(Sprite* sprite, frame_t frame)
    : spriteId(sprite ? sprite->id(): 0),
      frame(frame) {
  }
  FrameObj(const FrameObj&) = delete;
  FrameObj& operator=(const FrameObj&) = delete;

  Sprite* sprite(lua_State* L) { return check_docobj(L, doc::get<Sprite>(spriteId)); }
};

int Frame_gc(lua_State* L)
{
  get_obj<FrameObj>(L, 1)->~FrameObj();
  return 0;
}

int Frame_eq(lua_State* L)
{
  const auto a = get_obj<FrameObj>(L, 1);
  const auto b = get_obj<FrameObj>(L, 2);
  lua_pushboolean(L,
                  (a->spriteId == b->spriteId &&
                   a->frame == b->frame));
  return 1;
}

int Frame_get_sprite(lua_State* L)
{
  auto obj = get_obj<FrameObj>(L, 1);
  push_docobj<Sprite>(L, obj->spriteId);
  return 1;
}

int Frame_get_frameNumber(lua_State* L)
{
  auto obj = get_obj<FrameObj>(L, 1);
  lua_pushinteger(L, obj->frame+1);
  return 1;
}

int Frame_get_duration(lua_State* L)
{
  auto obj = get_obj<FrameObj>(L, 1);
  auto sprite = obj->sprite(L);
  lua_pushnumber(L, sprite->frameDuration(obj->frame) / 1000.0);
  return 1;
}

int Frame_get_previous(lua_State* L)
{
  auto obj = get_obj<FrameObj>(L, 1);
  auto sprite = obj->sprite(L);
  auto frame = obj->frame-1;
  if (frame >= 0 && frame < sprite->totalFrames())
    push_sprite_frame(L, sprite, frame);
  else
    lua_pushnil(L);
  return 1;
}

int Frame_get_next(lua_State* L)
{
  auto obj = get_obj<FrameObj>(L, 1);
  auto sprite = obj->sprite(L);
  auto frame = obj->frame+1;
  if (frame >= 0 && frame < sprite->totalFrames())
    push_sprite_frame(L, sprite, frame);
  else
    lua_pushnil(L);
  return 1;
}

int Frame_set_duration(lua_State* L)
{
  auto obj = get_obj<FrameObj>(L, 1);
  auto sprite = obj->sprite(L);
  double duration = lua_tonumber(L, 2) * 1000.0;
  Tx tx;
  tx(new cmd::SetFrameDuration(sprite, obj->frame, int(duration)));
  tx.commit();
  return 1;
}

const luaL_Reg Frame_methods[] = {
  { "__gc", Frame_gc },
  { "__eq", Frame_eq },
  { nullptr, nullptr }
};

const Property Frame_properties[] = {
  { "sprite", Frame_get_sprite, nullptr },
  { "frameNumber", Frame_get_frameNumber, nullptr },
  { "duration", Frame_get_duration, Frame_set_duration },
  { "previous", Frame_get_previous, nullptr },
  { "next", Frame_get_next, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(FrameObj);

void register_frame_class(lua_State* L)
{
  using Frame = FrameObj;
  REG_CLASS(L, Frame);
  REG_CLASS_PROPERTIES(L, Frame);
}

void push_sprite_frame(lua_State* L, Sprite* sprite, frame_t frame)
{
  push_new<FrameObj>(L, sprite, frame);
}

doc::frame_t get_frame_number_from_arg(lua_State* L, int index)
{
  auto obj = may_get_obj<FrameObj>(L, index);
  if (obj)
    return obj->frame;
  else if (lua_isnil(L, index) || lua_isnone(L, index))
    return 0;
  else
    return lua_tointeger(L, index)-1;
}

} // namespace script
} // namespace app
