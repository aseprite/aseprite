// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_slice_key.h"
#include "app/cmd/set_slice_name.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/userdata.h"
#include "app/tx.h"
#include "doc/slice.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

int Slice_eq(lua_State* L)
{
  const auto a = get_docobj<Slice>(L, 1);
  const auto b = get_docobj<Slice>(L, 2);
  lua_pushboolean(L, a->id() == b->id());
  return 1;
}

int Slice_get_sprite(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  push_docobj(L, slice->owner()->sprite());
  return 1;
}

int Slice_get_name(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  lua_pushstring(L, slice->name().c_str());
  return 1;
}

int Slice_get_bounds(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  if (!slice->empty())
    push_new<gfx::Rect>(L, slice->begin()->value()->bounds());
  else
    lua_pushnil(L);
  return 1;
}

int Slice_get_center(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  if (!slice->empty() && slice->begin()->value()->hasCenter())
    push_new<gfx::Rect>(L, slice->begin()->value()->center());
  else
    lua_pushnil(L);
  return 1;
}

int Slice_get_pivot(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  if (!slice->empty() && slice->begin()->value()->hasPivot())
    push_new<gfx::Point>(L, slice->begin()->value()->pivot());
  else
    lua_pushnil(L);
  return 1;
}

int Slice_set_name(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  const char* name = lua_tostring(L, 2);
  if (name) {
    Tx tx;
    tx(new cmd::SetSliceName(slice, name));
    tx.commit();
  }
  return 0;
}

int Slice_set_bounds(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  gfx::Rect bounds = convert_args_into_rect(L, 2);
  SliceKey key;
  if (const SliceKey* srcKey = slice->getByFrame(0))
    key = *srcKey;
  key.setBounds(bounds);
  Tx tx;
  tx(new cmd::SetSliceKey(slice, 0, key));
  tx.commit();
  return 0;
}

int Slice_set_center(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  gfx::Rect center = convert_args_into_rect(L, 2);
  SliceKey key;
  if (const SliceKey* srcKey = slice->getByFrame(0))
    key = *srcKey;
  key.setCenter(center);
  Tx tx;
  tx(new cmd::SetSliceKey(slice, 0, key));
  tx.commit();
  return 0;
}

int Slice_set_pivot(lua_State* L)
{
  auto slice = get_docobj<Slice>(L, 1);
  gfx::Point pivot = convert_args_into_point(L, 2);
  SliceKey key;
  if (const SliceKey* srcKey = slice->getByFrame(0))
    key = *srcKey;
  key.setPivot(pivot);
  Tx tx;
  tx(new cmd::SetSliceKey(slice, 0, key));
  tx.commit();
  return 0;
}

const luaL_Reg Slice_methods[] = {
  { "__eq", Slice_eq },
  { nullptr, nullptr }
};

const Property Slice_properties[] = {
  { "sprite", Slice_get_sprite, nullptr },
  { "name", Slice_get_name, Slice_set_name },
  { "bounds", Slice_get_bounds, Slice_set_bounds },
  { "center", Slice_get_center, Slice_set_center },
  { "pivot", Slice_get_pivot, Slice_set_pivot },
  { "color", UserData_get_color<Slice>, UserData_set_color<Slice> },
  { "data", UserData_get_text<Slice>, UserData_set_text<Slice> },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Slice);

void register_slice_class(lua_State* L)
{
  using doc::Slice;
  REG_CLASS(L, Slice);
  REG_CLASS_PROPERTIES(L, Slice);
}

} // namespace script
} // namespace app
