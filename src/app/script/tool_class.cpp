// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
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

namespace app {
namespace script {

namespace {

int Tool_get_id(lua_State* L)
{
  auto tool = get_ptr<tools::Tool>(L, 1);
  lua_pushstring(L, tool->getId().c_str());
  return 1;
}

const luaL_Reg Tool_methods[] = {
  { nullptr, nullptr }
};

const Property Tool_properties[] = {
  { "id", Tool_get_id, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

using Tool = tools::Tool;
DEF_MTNAME(Tool);

void register_tool_class(lua_State* L)
{
  REG_CLASS(L, Tool);
  REG_CLASS_PROPERTIES(L, Tool);
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

} // namespace script
} // namespace app
