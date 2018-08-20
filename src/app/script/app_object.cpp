// Aseprite
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/site.h"
#include "app/site.h"
#include "app/tx.h"

#include <iostream>

namespace app {
namespace script {

namespace {

int App_open(lua_State* L)
{
  const char* filename = luaL_checkstring(L, 1);

  app::Context* ctx = App::instance()->context();
  Doc* oldDoc = ctx->activeDocument();

  Command* openCommand =
    Commands::instance()->byId(CommandId::OpenFile());
  Params params;
  params.set("filename", filename);
  ctx->executeCommand(openCommand, params);

  Doc* newDoc = ctx->activeDocument();
  if (newDoc != oldDoc)
    push_ptr(L, newDoc->sprite());
  else
    lua_pushnil(L);
  return 1;
}

int App_exit(lua_State* L)
{
  app::Context* ctx = App::instance()->context();
  if (ctx && ctx->isUIAvailable()) {
    Command* exitCommand =
      Commands::instance()->byId(CommandId::Exit());
    ctx->executeCommand(exitCommand);
  }
  return 0;
}

int App_transaction(lua_State* L)
{
  int top = lua_gettop(L);
  int nresults = 0;
  if (lua_isfunction(L, 1)) {
    Tx tx; // Create a new transaction so it exists in the whole
           // duration of the argument function call.
    lua_pushvalue(L, -1);
    if (lua_pcall(L, 0, LUA_MULTRET, 0) == LUA_OK)
      tx.commit();
    nresults = lua_gettop(L) - top;
  }
  return nresults;
}

int App_undo(lua_State* L)
{
  app::Context* ctx = App::instance()->context();
  if (ctx) {
    Command* undo = Commands::instance()->byId(CommandId::Undo());
    ctx->executeCommand(undo);
  }
  return 0;
}

int App_redo(lua_State* L)
{
  app::Context* ctx = App::instance()->context();
  if (ctx) {
    Command* redo = Commands::instance()->byId(CommandId::Redo());
    ctx->executeCommand(redo);
  }
  return 0;
}

int App_get_activeSprite(lua_State* L)
{
  app::Context* ctx = App::instance()->context();
  Doc* doc = ctx->activeDocument();
  if (doc)
    push_ptr(L, doc->sprite());
  else
    lua_pushnil(L);
  return 1;
}

int App_get_activeImage(lua_State* L)
{
  app::Context* ctx = App::instance()->context();
  Site site = ctx->activeSite();
  if (site.image())
    push_ptr(L, site.image());
  else
    lua_pushnil(L);
  return 1;
}

int App_get_site(lua_State* L)
{
  app::Context* ctx = App::instance()->context();
  Site site = ctx->activeSite();
  push_obj(L, site);
  return 1;
}

int App_get_version(lua_State* L)
{
  lua_pushstring(L, VERSION);
  return 1;
}

const luaL_Reg App_methods[] = {
  { "open",        App_open },
  { "exit",        App_exit },
  { "transaction", App_transaction },
  { "undo",        App_undo },
  { "redo",        App_redo },
  { nullptr,       nullptr }
};

const Property App_properties[] = {
  { "activeSprite", App_get_activeSprite, nullptr },
  { "activeImage", App_get_activeImage, nullptr },
  { "version", App_get_version, nullptr },
  { "site", App_get_site, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(App);

void register_app_object(lua_State* L)
{
  REG_CLASS(L, App);
  REG_CLASS_PROPERTIES(L, App);

  lua_newtable(L);              // Create a table which will be the "app" object
  lua_pushvalue(L, -1);
  luaL_getmetatable(L, get_mtname<App>());
  lua_setmetatable(L, -2);
  lua_setglobal(L, "app");
  lua_pop(L, 1);                // Pop app table
}

} // namespace script
} // namespace app
