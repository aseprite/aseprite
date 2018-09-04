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

struct TagsObj {
  Sprite* sprite;
  TagsObj(Sprite* sprite)
    : sprite(sprite) {
  }
  TagsObj(const TagsObj&) = delete;
  TagsObj& operator=(const TagsObj&) = delete;
};

int Tags_gc(lua_State* L)
{
  get_obj<TagsObj>(L, 1)->~TagsObj();
  return 0;
}

int Tags_len(lua_State* L)
{
  auto obj = get_obj<TagsObj>(L, 1);
  lua_pushinteger(L, obj->sprite->frameTags().size());
  return 1;
}

int Tags_index(lua_State* L)
{
  auto obj = get_obj<TagsObj>(L, 1);
  auto& tags = obj->sprite->frameTags();
  const int i = lua_tonumber(L, 2);
  if (i < 1 || i > int(tags.size()))
    return luaL_error(L, "index out of bounds %d", i);
  push_ptr<FrameTag>(L, *(tags.begin()+i-1));
  return 1;
}

const luaL_Reg Tags_methods[] = {
  { "__gc", Tags_gc },
  { "__len", Tags_len },
  { "__index", Tags_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(TagsObj);

void register_tags_class(lua_State* L)
{
  using Tags = TagsObj;
  REG_CLASS(L, Tags);
}

void push_sprite_tags(lua_State* L, Sprite* sprite)
{
  push_new<TagsObj>(L, sprite);
}

} // namespace script
} // namespace app
