// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
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
#include "doc/tag.h"

namespace app {
namespace script {

using namespace doc;

namespace {

struct TagsObj {
  ObjectId spriteId;
  TagsObj(Sprite* sprite)
    : spriteId(sprite->id()) {
  }
  TagsObj(const TagsObj&) = delete;
  TagsObj& operator=(const TagsObj&) = delete;

  Sprite* sprite(lua_State* L) { return check_docobj(L, doc::get<Sprite>(spriteId)); }
};

int Tags_gc(lua_State* L)
{
  get_obj<TagsObj>(L, 1)->~TagsObj();
  return 0;
}

int Tags_len(lua_State* L)
{
  auto obj = get_obj<TagsObj>(L, 1);
  lua_pushinteger(L, obj->sprite(L)->tags().size());
  return 1;
}

int Tags_index(lua_State* L)
{
  auto obj = get_obj<TagsObj>(L, 1);
  auto& tags = obj->sprite(L)->tags();
  const int i = lua_tonumber(L, 2);
  if (i >= 1 && i <= int(tags.size()))
    push_docobj(L, *(tags.begin()+i-1));
  else
    lua_pushnil(L);
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
