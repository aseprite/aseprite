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
#include "app/script/app_scripting.h"
#include "app/site.h"
#include "app/site.h"
#include "app/tx.h"
#include "script/engine.h"

#include <iostream>

namespace app {

namespace {

void App_open(script::ContextHandle handle)
{
  script::Context ctx(handle);
  const char* filename = ctx.requireString(1);

  app::Context* appCtx = App::instance()->context();
  Doc* oldDoc = appCtx->activeDocument();

  Command* openCommand =
    Commands::instance()->byId(CommandId::OpenFile());
  Params params;
  params.set("filename", filename);
  appCtx->executeCommand(openCommand, params);

  Doc* newDoc = appCtx->activeDocument();
  if (newDoc != oldDoc)
    push_sprite(ctx, newDoc->sprite());
  else
    ctx.pushNull();
}

void App_exit(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Context* appCtx = App::instance()->context();
  if (appCtx && appCtx->isUIAvailable()) {
    Command* exitCommand =
      Commands::instance()->byId(CommandId::Exit());
    appCtx->executeCommand(exitCommand);
  }
  ctx.pushUndefined();
}

void App_transaction(script::ContextHandle handle)
{
  script::Context ctx(handle);
  if (ctx.isCallable(1)) {
    Tx tx; // Create a new transaction so it exists in the whole
           // duration of the argument function call.
    ctx.copy(1);
    ctx.call(0);
    tx.commit();
  }
  else
    ctx.pushUndefined();
}

void App_undo(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Context* appCtx = App::instance()->context();
  if (appCtx) {
    Command* undo = Commands::instance()->byId(CommandId::Undo());
    appCtx->executeCommand(undo);
  }
  ctx.pushUndefined();
}

void App_redo(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Context* appCtx = App::instance()->context();
  if (appCtx) {
    Command* redo = Commands::instance()->byId(CommandId::Redo());
    appCtx->executeCommand(redo);
  }
  ctx.pushUndefined();
}

void App_get_activeSprite(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Context* appCtx = App::instance()->context();
  Doc* doc = appCtx->activeDocument();
  if (doc)
    push_sprite(ctx, doc->sprite());
  else
    ctx.pushNull();
}

void App_get_activeImage(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Context* appCtx = App::instance()->context();
  Site site = appCtx->activeSite();
  if (site.image())
    push_image(ctx, site.image());
  else
    ctx.pushNull();
}

void App_get_site(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Context* appCtx = App::instance()->context();
  Site site = appCtx->activeSite();
  push_site(ctx, site);
}

void App_get_pixelColor(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.newObject("PixelColor", nullptr, nullptr);
}

void App_get_version(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushString(VERSION);
}

const script::FunctionEntry App_methods[] = {
  { "open", App_open, 1 },
  { "exit", App_exit, 0 },
  { "transaction", App_transaction, 1 },
  { "undo", App_undo, 0 },
  { "redo", App_redo, 0 },
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry App_props[] = {
  { "activeSprite", App_get_activeSprite, nullptr },
  { "activeImage", App_get_activeImage, nullptr },
  { "pixelColor", App_get_pixelColor, nullptr },
  { "version", App_get_version, nullptr },
  { "site", App_get_site, nullptr },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_app_object(script::Context& ctx)
{
  ctx.pushGlobalObject();
  ctx.registerObject(-1, "app", App_methods, App_props);
  ctx.pop();
}

} // namespace app
