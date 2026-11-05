// Aseprite
// Copyright (C) 2019-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/tools/tool.h"
#include "app/tools/tool_box.h"

namespace app { namespace script {

namespace {

LUA_FUNCTION(Tool_get_id)
{
  auto tool = get_ptr<tools::Tool>(L, 1);
  lua_pushstring(L, tool->getId().c_str());
  return 1;
}

const luaL_Reg Tool_methods[] = {
  { nullptr, nullptr }
};

const Property Tool_properties[] = {
  { "id",    Tool_get_id, nullptr },
  { nullptr, nullptr,     nullptr }
};
DEF_ITERATOR_PAIRS(Tool);
} // anonymous namespace

using Tool = tools::Tool;
DEF_MTNAME(Tool);

LUA_FUNCTION(register_tool_class)
{
  REG_CLASS(L, Tool);
  REG_CLASS_PROPERTIES(L, Tool);
  return 0;
}

void push_tool(lua_State* L, tools::Tool* tool)
{
  push_ptr<Tool>(L, tool);
}

tools::Tool* get_tool_from_arg(lua_State* L, int index)
{
  if (auto tool = may_get_ptr<tools::Tool>(L, index))
    return tool;
  else if (const char* id = lua_tostring(L, index))
    return App::instance()->toolBox()->getToolById(id);
  else
    return nullptr;
}

}} // namespace app::script
