// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/script/luacpp.h"

namespace app::script {

int ObjectIterator_pairs_next(lua_State* L)
{
  auto* it = get_obj<ObjectIterator>(L, lua_upvalueindex(1));
  const auto& prop = it->properties[it->i];
  if (prop.getter == nullptr)
    return 0;

  lua_pushstring(L, prop.name);
  prop.getter(L);
  ++it->i;
  return 2;
}

namespace {
int ObjectIterator_gc(lua_State* L)
{
  get_obj<ObjectIterator>(L, 1)->~ObjectIterator();
  return 0;
}

const luaL_Reg ObjectIterator_methods[] = {
  { "__gc",  ObjectIterator_gc },
  { nullptr, nullptr           }
};
} // namespace

DEF_MTNAME(ObjectIterator);

void register_iterator_class(lua_State* L)
{
  REG_CLASS(L, ObjectIterator);
}
} // namespace app::script
