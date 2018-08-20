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
#include "app/site.h"

namespace app {
namespace script {

namespace {

int Site_get_sprite(lua_State* L)
{
  auto site = get_obj<Site>(L, 1);
  if (site->sprite())
    push_ptr(L, site->sprite());
  else
    lua_pushnil(L);
  return 1;
}

int Site_get_image(lua_State* L)
{
  auto site = get_obj<Site>(L, 1);
  if (site->image())
    push_ptr(L, site->image());
  else
    lua_pushnil(L);
  return 1;
}

const luaL_Reg Site_methods[] = {
  { nullptr, nullptr }
};

const Property Site_properties[] = {
  { "sprite", Site_get_sprite, nullptr },
  { "image", Site_get_image, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(app::Site);

void register_site_class(lua_State* L)
{
  REG_CLASS(L, Site);
  REG_CLASS_PROPERTIES(L, Site);
}

} // namespace script
} // namespace app
