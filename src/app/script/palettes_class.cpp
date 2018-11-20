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

struct PalettesObj {
  ObjectId spriteId;
  PalettesObj(Sprite* sprite)
    : spriteId(sprite->id()) {
  }
  PalettesObj(const PalettesObj&) = delete;
  PalettesObj& operator=(const PalettesObj&) = delete;

  Sprite* sprite(lua_State* L) { return check_docobj(L, doc::get<Sprite>(spriteId)); }
};

int Palettes_gc(lua_State* L)
{
  get_obj<PalettesObj>(L, 1)->~PalettesObj();
  return 0;
}

int Palettes_len(lua_State* L)
{
  auto obj = get_obj<PalettesObj>(L, 1);
  lua_pushinteger(L, obj->sprite(L)->getPalettes().size());
  return 1;
}

int Palettes_index(lua_State* L)
{
  auto obj = get_obj<PalettesObj>(L, 1);
  auto sprite = obj->sprite(L);
  auto& pals = sprite->getPalettes();
  int i = lua_tointeger(L, 2);
  if (i < 1 || i > int(pals.size()))
    return luaL_error(L, "index out of bounds %d", i);
  push_sprite_palette(L, sprite, pals[i-1]);
  return 1;
}

const luaL_Reg Palettes_methods[] = {
  { "__gc", Palettes_gc },
  { "__len", Palettes_len },
  { "__index", Palettes_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(PalettesObj);

void register_palettes_class(lua_State* L)
{
  using Palettes = PalettesObj;
  REG_CLASS(L, Palettes);
}

void push_sprite_palettes(lua_State* L, Sprite* sprite)
{
  push_new<PalettesObj>(L, sprite);
}

} // namespace script
} // namespace app
