// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "doc/grid.h"

namespace app { namespace script {

using namespace doc;

namespace {

doc::Grid Grid_new(lua_State* L, int index)
{
  doc::Grid grid;
  // Copy other size
  if (auto grid2 = may_get_obj<doc::Grid>(L, index)) {
    grid = *grid2;
  }
  // Convert Rectangle into a Grid
  else if (lua_istable(L, index)) {
    gfx::Rect rect = convert_args_into_rect(L, index);
    grid = Grid(rect.size());
    grid.origin(rect.origin());
  }
  return grid;
}

LUA_FUNCTION(Grid_new)
{
  push_obj(L, Grid_new(L, 1));
  return 1;
}

LUA_FUNCTION(Grid_gc)
{
  get_obj<doc::Grid>(L, 1)->~Grid();
  return 0;
}

LUA_FUNCTION(Grid_get_tileSize)
{
  auto grid = get_obj<doc::Grid>(L, 1);
  push_obj(L, grid->tileSize());
  return 1;
}

LUA_FUNCTION(Grid_get_origin)
{
  auto grid = get_obj<doc::Grid>(L, 1);
  push_obj(L, grid->origin());
  return 1;
}

const luaL_Reg Grid_methods[] = {
  { "__gc",  Grid_gc },
  { nullptr, nullptr }
};

const Property Grid_properties[] = {
  { "tileSize", Grid_get_tileSize, nullptr },
  { "origin",   Grid_get_origin,   nullptr },
  { nullptr,    nullptr,           nullptr }
};
DEF_ITERATOR_PAIRS(Grid);
} // anonymous namespace

DEF_MTNAME(Grid);

LUA_FUNCTION(register_grid_class)
{
  using Grid = doc::Grid;
  REG_CLASS(L, Grid);
  REG_CLASS_NEW(L, Grid);
  REG_CLASS_PROPERTIES(L, Grid);
  return 0;
}

doc::Grid convert_args_into_grid(lua_State* L, int index)
{
  return Grid_new(L, index);
}

}} // namespace app::script
