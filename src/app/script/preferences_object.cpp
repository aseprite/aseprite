// Aseprite
// Copyright (C) 2019  Igara Studio S.A.
// Copyright (C) 2017-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/doc.h"
#include "app/pref/option.h"
#include "app/pref/preferences.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "doc/sprite.h"

#include <cstring>

namespace app {
namespace script {

namespace {

struct AppPreferences { };

int Section_index(lua_State* L)
{
  Section* section = get_ptr<Section>(L, 1);

  const char* id = lua_tostring(L, 2);
  if (!id)
    return luaL_error(L, "optionName in 'app.preferences.%s.optionName' must be a string",
                      section->name());

  OptionBase* option = section->option(id);
  if (!option) {
    Section* subSection = section->section(
      (section->name() && *section->name() ? std::string(section->name()) + "." + id:
                                             std::string(id)).c_str());
    if (subSection) {
      push_ptr(L, subSection);
      return 1;
    }
    return luaL_error(L, "option '%s' in section '%s' doesn't exist", id, section->name());
  }

  option->pushLua(L);
  return 1;
}

int Section_newindex(lua_State* L)
{
  Section* section = get_ptr<Section>(L, 1);

  const char* id = lua_tostring(L, 2);
  if (!id)
    return luaL_error(L, "optionName in 'app.preferences.%s.optionName' must be a string",
                      section->name());

  OptionBase* option = section->option(id);
  if (!option)
    return luaL_error(L, "option '%s' in section '%s' doesn't exist",
                      id, section->name());

  option->fromLua(L, 3);
  return 0;
}

int ToolPref_function(lua_State* L)
{
  auto tool = get_tool_from_arg(L, 1);
  if (!tool)
    return luaL_error(L, "tool preferences not found");

  // If we don't have the UI available, we reset the tools
  // preferences, so scripts that are executed in batch mode have a
  // reproducible behavior.
  if (!App::instance()->isGui())
    Preferences::instance().resetToolPreferences(tool);

  ToolPreferences& toolPref = Preferences::instance().tool(tool);
  push_ptr(L, (Section*)&toolPref);
  return 1;
}

int DocPref_function(lua_State* L)
{
  auto sprite = may_get_docobj<Sprite>(L, 1);
  DocumentPreferences& docPref =
    Preferences::instance().document(
      sprite ? static_cast<const Doc*>(sprite->document()): nullptr);
  push_ptr(L, (Section*)&docPref);
  return 1;
}

int AppPreferences_index(lua_State* L)
{
  const char* id = lua_tostring(L, 2);
  if (!id)
    return luaL_error(L, "id in 'app.preferences.id' must be a string");

  if (std::strcmp(id, "tool") == 0) {
    lua_pushcfunction(L, ToolPref_function);
    return 1;
  }
  else if (std::strcmp(id, "document") == 0) {
    lua_pushcfunction(L, DocPref_function);
    return 1;
  }

  Section* section = Preferences::instance().section(id);
  if (!section)
    return luaL_error(L, "section '%s' in preferences doesn't exist", id);

  push_ptr(L, section);
  return 1;
}

const luaL_Reg Section_methods[] = {
  { "__index", Section_index },
  { "__newindex", Section_newindex },
  { nullptr, nullptr }
};

const luaL_Reg AppPreferences_methods[] = {
  { "__index", AppPreferences_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Section);
DEF_MTNAME(AppPreferences);

void register_app_preferences_object(lua_State* L)
{
  REG_CLASS(L, Section);
  REG_CLASS(L, AppPreferences);

  lua_getglobal(L, "app");
  lua_pushstring(L, "preferences");
  push_new<AppPreferences>(L);
  lua_rawset(L, -3);
  lua_pop(L, 1);
}

} // namespace script
} // namespace app
