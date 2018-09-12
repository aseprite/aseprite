// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace app {
namespace script {

using namespace doc;

namespace {

struct SpritesObj {
  std::vector<Doc*> docs;
  SpritesObj(const Docs& docs) {
    std::copy(docs.begin(), docs.end(),
              std::back_inserter(this->docs));
  }
  SpritesObj(const SpritesObj&) = delete;
  SpritesObj& operator=(const SpritesObj&) = delete;
};

int Sprites_gc(lua_State* L)
{
  get_obj<SpritesObj>(L, 1)->~SpritesObj();
  return 0;
}

int Sprites_len(lua_State* L)
{
  auto obj = get_obj<SpritesObj>(L, 1);
  lua_pushinteger(L, obj->docs.size());
  return 1;
}

int Sprites_index(lua_State* L)
{
  auto obj = get_obj<SpritesObj>(L, 1);
  const int i = lua_tonumber(L, 2);
  if (i >= 1 && i <= int(obj->docs.size()))
    push_ptr<Sprite>(L, obj->docs[i-1]->sprite());
  else
    lua_pushnil(L);
  return 1;
}

const luaL_Reg Sprites_methods[] = {
  { "__gc", Sprites_gc },
  { "__len", Sprites_len },
  { "__index", Sprites_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(SpritesObj);

void register_sprites_class(lua_State* L)
{
  using Sprites = SpritesObj;
  REG_CLASS(L, Sprites);
}

void push_sprites(lua_State* L)
{
  app::Context* ctx = App::instance()->context();
  push_new<SpritesObj>(L, ctx->documents());
}

} // namespace script
} // namespace app
