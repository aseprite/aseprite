// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/console.h"
#include "app/context.h"
#include "app/context_observer.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/site.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace script {

namespace {

class ScriptSiteObserver : public ContextObserver {
public:
  static ScriptSiteObserver* instance() { 
    static ScriptSiteObserver instance;
    return &instance;
  }

  void onActiveSiteChange(const Site& site) {
    if (m_onchangeRef == 0)
      return;

    script::Engine* engine = App::instance()->scriptEngine();
    lua_State* L = engine->luaState();

    lua_rawgeti(L, LUA_REGISTRYINDEX, m_onchangeRef);
    if (lua_pcall(L, 0, 0, 0)) {
      if (const char* s = lua_tostring(L, -1)) {
        Console().printf("Error: %s", s);
      }
    }
  }

  int callbackRef() { 
    return m_onchangeRef; 
  }

  void setCallbackRef(int onchangeRef) { 
    m_onchangeRef = onchangeRef;
  }

private:
  ScriptSiteObserver()
    : ContextObserver(),
      m_onchangeRef(0)
  { }

  ~ScriptSiteObserver() { }

  int m_onchangeRef;
};

int Site_get_sprite(lua_State* L)
{
  auto site = get_obj<Site>(L, 1);
  if (site->sprite())
    push_docobj(L, site->sprite());
  else
    lua_pushnil(L);
  return 1;
}

int Site_get_layer(lua_State* L)
{
  auto site = get_obj<Site>(L, 1);
  if (site->layer())
    push_docobj<Layer>(L, site->layer());
  else
    lua_pushnil(L);
  return 1;
}

int Site_get_cel(lua_State* L)
{
  auto site = get_obj<Site>(L, 1);
  if (site->cel())
    push_docobj<Cel>(L, site->cel());
  else
    lua_pushnil(L);
  return 1;
}

int Site_get_frame(lua_State* L)
{
  auto site = get_obj<Site>(L, 1);
  if (site->sprite())
    push_sprite_frame(L, site->sprite(), site->frame());
  else
    lua_pushnil(L);
  return 1;
}

int Site_get_frameNumber(lua_State* L)
{
  auto site = get_obj<Site>(L, 1);
  lua_pushinteger(L, site->frame()+1);
  return 1;
}

int Site_get_image(lua_State* L)
{
  auto site = get_obj<Site>(L, 1);
  if (site->cel())
    push_cel_image(L, site->cel());
  else
    lua_pushnil(L);
  return 1;
}

int Site_get_onChange(lua_State* L)
{
  lua_rawgeti(L, LUA_REGISTRYINDEX, ScriptSiteObserver::instance()->callbackRef());
  return 1;
}

int Site_set_onChange(lua_State* L)
{
  auto site = get_obj<Site>(L, 1);
  auto obs = ScriptSiteObserver::instance();

  if (lua_isnil(L, 2) && obs->callbackRef()) {
    luaL_unref(L, LUA_REGISTRYINDEX, obs->callbackRef());
    obs->setCallbackRef(0);
    App::instance()->context()->remove_observer(obs);
  }
  else if (lua_isfunction(L, 2)) {
    int onchangeRef = luaL_ref(L, LUA_REGISTRYINDEX);
    if (!obs->callbackRef()) {
      App::instance()->context()->add_observer(obs);
    }
    obs->setCallbackRef(onchangeRef);
  }
  
  return 0;
}

const luaL_Reg Site_methods[] = {
  { nullptr, nullptr }
};

const Property Site_properties[] = {
  { "sprite", Site_get_sprite, nullptr },
  { "layer", Site_get_layer, nullptr },
  { "cel", Site_get_cel, nullptr },
  { "frame", Site_get_frame, nullptr },
  { "frameNumber", Site_get_frameNumber, nullptr },
  { "image", Site_get_image, nullptr },
  { "onChange", Site_get_onChange, Site_set_onChange },
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
