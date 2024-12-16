// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "doc/tilesets.h"

namespace app { namespace script {

using namespace doc;

namespace {

int Tilesets_len(lua_State* L)
{
  auto obj = get_docobj<Tilesets>(L, 1);
  lua_pushinteger(L, obj->size());
  return 1;
}

int Tilesets_index(lua_State* L)
{
  auto obj = get_docobj<Tilesets>(L, 1);
  const int i = lua_tonumber(L, 2);
  if (i >= 1 && i <= int(obj->size())) {
    auto ts = obj->get(i - 1);
    if (ts) {
      push_docobj(L, ts);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

const luaL_Reg Tilesets_methods[] = {
  { "__len",   Tilesets_len   },
  { "__index", Tilesets_index },
  { nullptr,   nullptr        }
};

} // anonymous namespace

DEF_MTNAME(Tilesets);

void register_tilesets_class(lua_State* L)
{
  REG_CLASS(L, Tilesets);
}

void push_tilesets(lua_State* L, Tilesets* tilesets)
{
  push_docobj(L, tilesets);
}

}} // namespace app::script
