// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/luacpp.h"
#include "doc/cels_range.h"
#include "doc/sprite.h"

#include <algorithm>

namespace app {
namespace script {

using namespace doc;

namespace {

struct CelsObj {
  Sprite* sprite;
  CelsObj(Sprite* sprite) : sprite(sprite) { }
  CelsObj(const CelsObj&) = delete;
  CelsObj& operator=(const CelsObj&) = delete;
};

int Cels_gc(lua_State* L)
{
  get_obj<CelsObj>(L, 1)->~CelsObj();
  return 0;
}

int Cels_len(lua_State* L)
{
  auto obj = get_obj<CelsObj>(L, 1);
  lua_pushinteger(L, obj->sprite->cels().size());
  return 1;
}

int Cels_index(lua_State* L)
{
  auto obj = get_obj<CelsObj>(L, 1);
  const int i = lua_tointeger(L, 2);
  if (i < 1 || i > obj->sprite->cels().size())
    return luaL_error(L, "index out of bounds %d", i);

  auto it = obj->sprite->cels().begin();
  std::advance(it, i-1);          // TODO improve this

  push_ptr<Cel>(L, *it);
  return 1;
}

const luaL_Reg Cels_methods[] = {
  { "__gc", Cels_gc },
  { "__len", Cels_len },
  { "__index", Cels_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(CelsObj);

void register_cels_class(lua_State* L)
{
  using Cels = CelsObj;
  REG_CLASS(L, Cels);
}

void push_sprite_cels(lua_State* L, Sprite* sprite)
{
  push_new<CelsObj>(L, sprite);
}

} // namespace script
} // namespace app
