// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/app_menus.h"
#include "app/commands/command.h"
#include "app/commands/commands.h"
#include "app/console.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/ui/app_menuitem.h"

namespace app {
namespace script {

namespace {

struct Plugin {
  Extension* ext;
  Plugin(Extension* ext) : ext(ext) { }
};

class PluginCommand : public Command {
public:
  PluginCommand(const std::string& id,
                const std::string& title,
                int onclickRef,
                int onenabledRef)
    : Command(id.c_str(), CmdUIOnlyFlag)
    , m_title(title)
    , m_onclickRef(onclickRef)
    , m_onenabledRef(onenabledRef)
  {
  }

  ~PluginCommand() {
    auto app = App::instance();
    ASSERT(app);
    if (!app)
      return;

    if (script::Engine* engine = app->scriptEngine()) {
      lua_State* L = engine->luaState();
      luaL_unref(L, LUA_REGISTRYINDEX, m_onclickRef);
    }
  }

protected:
  std::string onGetFriendlyName() const override {
    return m_title;
  }

  void onExecute(Context* context) override {
    script::Engine* engine = App::instance()->scriptEngine();
    lua_State* L = engine->luaState();

    lua_rawgeti(L, LUA_REGISTRYINDEX, m_onclickRef);
    if (lua_pcall(L, 0, 1, 0)) {
      if (const char* s = lua_tostring(L, -1)) {
        Console().printf("Error: %s", s);
      }
    }
    else {
      lua_pop(L, 1);
    }
  }

  bool onEnabled(Context* context) override {
    if (m_onenabledRef) {
      script::Engine* engine = App::instance()->scriptEngine();
      lua_State* L = engine->luaState();

      lua_rawgeti(L, LUA_REGISTRYINDEX, m_onenabledRef);
      if (lua_pcall(L, 0, 1, 0)) {
        if (const char* s = lua_tostring(L, -1)) {
          Console().printf("Error: %s", s);
          return false;
        }
      }
      else {
        bool ret = lua_toboolean(L, -1);
        lua_pop(L, 1);
        return ret;
      }
    }
    return true;
  }

  std::string m_title;
  int m_onclickRef;
  int m_onenabledRef;
};

void deleteCommandIfExistent(Extension* ext, const std::string& id)
{
  auto cmd = Commands::instance()->byId(id.c_str());
  if (cmd) {
    Commands::instance()->remove(cmd);
    ext->removeCommand(id);
    delete cmd;
  }
}

void deleteMenuGroupIfExistent(Extension* ext, const std::string& id)
{
#ifdef ENABLE_UI
  if (auto appMenus = AppMenus::instance())
    appMenus->removeMenuGroup(id);
#endif

  ext->removeMenuGroup(id);
}

int Plugin_gc(lua_State* L)
{
  get_obj<Plugin>(L, 1)->~Plugin();
  return 0;
}

int Plugin_newCommand(lua_State* L)
{
  auto plugin = get_obj<Plugin>(L, 1);
  if (lua_istable(L, 2)) {
    std::string id, title, group;
    int onenabledRef = 0;

    lua_getfield(L, 2, "id");
    if (const char* s = lua_tostring(L, -1)) {
      id = s;
    }
    lua_pop(L, 1);

    if (id.empty())
      return luaL_error(L, "Empty id field in plugin:newCommand{ id=... }");

    lua_getfield(L, 2, "title");
    if (const char* s = lua_tostring(L, -1)) {
      title = s;
    }
    lua_pop(L, 1);

    lua_getfield(L, 2, "group");
    if (const char* s = lua_tostring(L, -1)) {
      group = s;
    }
    lua_pop(L, 1);

    int type = lua_getfield(L, 2, "onenabled");
    if (type == LUA_TFUNCTION) {
      onenabledRef = luaL_ref(L, LUA_REGISTRYINDEX); // does a pop
    }
    else {
      lua_pop(L, 1);
    }

    type = lua_getfield(L, 2, "onclick");
    if (type == LUA_TFUNCTION) {
      int onclickRef = luaL_ref(L, LUA_REGISTRYINDEX);

      // Delete the command if it already exist (e.g. we are
      // overwriting a previous registered command)
      deleteCommandIfExistent(plugin->ext, id);

      auto cmd = new PluginCommand(id, title, onclickRef, onenabledRef);
      Commands::instance()->add(cmd);
      plugin->ext->addCommand(id);

#ifdef ENABLE_UI
      // Add a new menu option if the "group" is defined
      if (!group.empty() &&
          App::instance()->isGui()) { // On CLI menus do not make sense
        if (auto appMenus = AppMenus::instance()) {
          auto menuItem = std::make_unique<AppMenuItem>(title, id);
          appMenus->addMenuItemIntoGroup(group, std::move(menuItem));
        }
      }
#endif // ENABLE_UI
    }
    else {
      lua_pop(L, 1);
    }
  }
  return 0;
}

int Plugin_deleteCommand(lua_State* L)
{
  std::string id;

  auto plugin = get_obj<Plugin>(L, 1);
  if (lua_istable(L, 2)) {
    lua_getfield(L, 2, "id");
    if (const char* s = lua_tostring(L, -1)) {
      id = s;
    }
    lua_pop(L, 1);
  }
  else if (const char* s = lua_tostring(L, 2)) {
    id = s;
  }

  if (id.empty())
    return luaL_error(L, "No command id specified in plugin:deleteCommand()");

  // TODO this can crash if we delete the command from the same command
  deleteCommandIfExistent(plugin->ext, id);
  return 0;
}

int Plugin_newMenuGroup(lua_State* L)
{
  auto plugin = get_obj<Plugin>(L, 1);
  if (lua_istable(L, 2)) {
    std::string id, title, group;

    lua_getfield(L, 2, "id");   // This new group ID
    if (const char* s = lua_tostring(L, -1)) {
      id = s;
    }
    lua_pop(L, 1);

    if (id.empty())
      return luaL_error(L, "Empty id field in plugin:newMenuGroup{ id=... }");

    lua_getfield(L, 2, "title");
    if (const char* s = lua_tostring(L, -1)) {
      title = s;
    }
    lua_pop(L, 1);

    lua_getfield(L, 2, "group"); // Parent group
    if (const char* s = lua_tostring(L, -1)) {
      group = s;
    }
    lua_pop(L, 1);

    // Delete the group if it already exist (e.g. we are overwriting a
    // previous registered group)
    deleteMenuGroupIfExistent(plugin->ext, id);

    plugin->ext->addMenuGroup(id);

#ifdef ENABLE_UI
    // Add a new menu option if the "group" is defined
    if (!group.empty() &&
        App::instance()->isGui()) {  // On CLI menus do not make sense
      if (auto appMenus = AppMenus::instance()) {
        auto menuItem = std::make_unique<AppMenuItem>(title, id);
        menuItem->setSubmenu(new Menu);
        appMenus->addMenuGroup(id, menuItem.get());
        appMenus->addMenuItemIntoGroup(group, std::move(menuItem));
      }
    }
#endif // ENABLE_UI
  }
  return 0;
}

int Plugin_deleteMenuGroup(lua_State* L)
{
  std::string id;

  auto plugin = get_obj<Plugin>(L, 1);
  if (lua_istable(L, 2)) {
    lua_getfield(L, 2, "id");
    if (const char* s = lua_tostring(L, -1)) {
      id = s;
    }
    lua_pop(L, 1);
  }
  else if (const char* s = lua_tostring(L, 2)) {
    id = s;
  }

  if (id.empty())
    return luaL_error(L, "No menu group id specified in plugin:deleteMenuGroup()");

  deleteMenuGroupIfExistent(plugin->ext, id);
  return 0;
}

int Plugin_newMenuSeparator(lua_State* L)
{
  auto plugin = get_obj<Plugin>(L, 1);
  if (lua_istable(L, 2)) {
    std::string group;

    lua_getfield(L, 2, "group"); // Parent group
    if (const char* s = lua_tostring(L, -1)) {
      group = s;
    }
    lua_pop(L, 1);

#ifdef ENABLE_UI
    // Add a new separator if the "group" is defined
    if (!group.empty() &&
        App::instance()->isGui()) {  // On CLI menus do not make sense
      if (auto appMenus = AppMenus::instance()) {
        auto menuItem = std::make_unique<ui::MenuSeparator>();
        plugin->ext->addMenuSeparator(menuItem.get());
        appMenus->addMenuItemIntoGroup(group, std::move(menuItem));
      }
    }
#endif // ENABLE_UI
  }
  return 0;
}

int Plugin_get_name(lua_State* L)
{
  auto plugin = get_obj<Plugin>(L, 1);
  lua_pushstring(L, plugin->ext->name().c_str());
  return 1;
}

int Plugin_get_path(lua_State* L)
{
  auto plugin = get_obj<Plugin>(L, 1);
  lua_pushstring(L, plugin->ext->path().c_str());
  return 1;
}

int Plugin_get_preferences(lua_State* L)
{
  if (!lua_getuservalue(L, 1)) {
    lua_newtable(L);
    lua_pushvalue(L, -1);
    lua_setuservalue(L, 1);
  }
  return 1;
}

int Plugin_set_preferences(lua_State* L)
{
  lua_pushvalue(L, 2);
  lua_setuservalue(L, 1);
  return 0;
}

const luaL_Reg Plugin_methods[] = {
  { "__gc", Plugin_gc },
  { "newCommand", Plugin_newCommand },
  { "deleteCommand", Plugin_deleteCommand },
  { "newMenuGroup", Plugin_newMenuGroup },
  { "deleteMenuGroup", Plugin_deleteMenuGroup },
  { "newMenuSeparator", Plugin_newMenuSeparator },
  { nullptr, nullptr }
};

const Property Plugin_properties[] = {
  { "name", Plugin_get_name, nullptr },
  { "path", Plugin_get_path, nullptr },
  { "preferences", Plugin_get_preferences, Plugin_set_preferences },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Plugin);

void register_plugin_class(lua_State* L)
{
  REG_CLASS(L, Plugin);
  REG_CLASS_PROPERTIES(L, Plugin);
}

void push_plugin(lua_State* L, Extension* ext)
{
  push_new<Plugin>(L, ext);
}

} // namespace script
} // namespace app
