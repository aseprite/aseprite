// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
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
                int onclickRef)
    : Command(id.c_str(), CmdUIOnlyFlag)
    , m_title(title)
    , m_onclickRef(onclickRef) {
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

  std::string m_title;
  int m_onclickRef;
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

    int type = lua_getfield(L, 2, "onclick");
    if (type == LUA_TFUNCTION) {
      int onclickRef = luaL_ref(L, LUA_REGISTRYINDEX);

      // Delete the command if it already exist (e.g. we are
      // overwriting a previous registered command)
      deleteCommandIfExistent(plugin->ext, id);

      auto cmd = new PluginCommand(id, title, onclickRef);
      Commands::instance()->add(cmd);
      plugin->ext->addCommand(id);

#ifdef ENABLE_UI
      // Add a new menu option if the "group" is defined
      if (!group.empty() &&
          App::instance()->isGui()) { // On CLI menus do not make sense
        if (auto appMenus = AppMenus::instance()) {
          std::unique_ptr<MenuItem> menuItem(new AppMenuItem(title, cmd));
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
  { nullptr, nullptr }
};

const Property Plugin_properties[] = {
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
