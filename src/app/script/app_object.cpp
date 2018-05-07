// Aseprite
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/context.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/script/app_scripting.h"
#include "app/script/sprite_wrap.h"
#include "script/engine.h"

#include <iostream>

namespace app {

namespace {

void App_open(script::ContextHandle handle)
{
  script::Context ctx(handle);
  const char* filename = ctx.requireString(1);

  app::Context* appCtx = App::instance()->context();
  app::Document* oldDoc = appCtx->activeDocument();

  Command* openCommand =
    Commands::instance()->byId(CommandId::OpenFile());
  Params params;
  params.set("filename", filename);
  appCtx->executeCommand(openCommand, params);

  app::Document* newDoc = appCtx->activeDocument();
  if (newDoc != oldDoc)
    ctx.newObject("Sprite", unwrap_engine(ctx)->wrapSprite(newDoc), nullptr);
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

void App_get_activeSprite(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Context* appCtx = App::instance()->context();
  app::Document* doc = appCtx->activeDocument();
  if (doc)
    ctx.newObject("Sprite", unwrap_engine(ctx)->wrapSprite(doc), nullptr);
  else
    ctx.pushNull();
}

void App_get_activeImage(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Context* appCtx = App::instance()->context();
  app::Document* doc = appCtx->activeDocument();
  if (doc) {
    SpriteWrap* sprWrap = unwrap_engine(ctx)->wrapSprite(doc);
    ASSERT(sprWrap);

    ImageWrap* imgWrap = sprWrap->activeImage();
    if (imgWrap != nullptr) {
      ctx.newObject("Image", imgWrap, nullptr);
      return;
    }
  }
  ctx.pushNull();
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
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry App_props[] = {
  { "activeSprite", App_get_activeSprite, nullptr },
  { "activeImage", App_get_activeImage, nullptr },
  { "pixelColor", App_get_pixelColor, nullptr },
  { "version", App_get_version, nullptr },
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
