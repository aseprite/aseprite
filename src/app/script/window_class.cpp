// Aseprite
// Copyright (C) 2023-present  Igara Studio S.A.
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

LUA_FUNCTION(Window_get_width)
{
  auto window = get_ptr<ui::Window>(L, 1);
  lua_pushinteger(L, window->bounds().w);
  return 1;
}

LUA_FUNCTION(Window_get_height)
{
  auto window = get_ptr<ui::Window>(L, 1);
  lua_pushinteger(L, window->bounds().h);
  return 1;
}

LUA_FUNCTION(Window_get_events)
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
DEF_ITERATOR_PAIRS(Window);
} // anonymous namespace

DEF_MTNAME(ui::Window);

LUA_FUNCTION(register_window_class)
{
  using ui::Window;
  REG_CLASS(L, Window);
  REG_CLASS_PROPERTIES(L, Window);
  return 0;
}

}} // namespace app::script
