// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "ui/window.h"

namespace app { namespace script {

namespace {

int Window_get_width(lua_State* L)
{
  auto window = get_ptr<ui::Window>(L, 1);
  lua_pushinteger(L, window->bounds().w);
  return 1;
}

int Window_get_height(lua_State* L)
{
  auto window = get_ptr<ui::Window>(L, 1);
  lua_pushinteger(L, window->bounds().h);
  return 1;
}

int Window_get_events(lua_State* L)
{
  auto window = get_ptr<ui::Window>(L, 1);
  push_window_events(L, window);
  return 1;
}

const luaL_Reg Window_methods[] = {
  { nullptr, nullptr }
};

const Property Window_properties[] = {
  { "width",  Window_get_width,  nullptr },
  { "height", Window_get_height, nullptr },
  { "events", Window_get_events, nullptr },
  { nullptr,  nullptr,           nullptr }
};

} // anonymous namespace

DEF_MTNAME(ui::Window);

void register_window_class(lua_State* L)
{
  using ui::Window;
  REG_CLASS(L, Window);
  REG_CLASS_PROPERTIES(L, Window);
}

}} // namespace app::script
