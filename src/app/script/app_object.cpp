// Aseprite
// Copyright (C) 2015-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/console_object.h"

#include "app/document.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/script/app_scripting.h"
#include "app/script/sprite_wrap.h"
#include "app/ui_context.h"
#include "script/engine.h"

// App sub-objects
#include "app/script/pixel_color.h"

#include <iostream>

namespace app {

namespace {

script::result_t App_open(script::ContextHandle handle)
{
  script::Context ctx(handle);
  if (!ctx.isString(0) ||
      !ctx.toString(0))
    return 0;

  const char* fn = ctx.toString(0);

  app::Document* oldDoc = UIContext::instance()->activeDocument();

  Command* openCommand = CommandsModule::instance()->getCommandByName(CommandId::OpenFile);
  Params params;
  params.set("filename", fn);
  UIContext::instance()->executeCommand(openCommand, params);

  app::Document* newDoc = UIContext::instance()->activeDocument();
  if (newDoc != oldDoc)
    ctx.pushObject(unwrap_engine(ctx)->wrapSprite(newDoc), "Sprite");
  else
    ctx.pushNull();
  return 1;
}

script::result_t App_exit(script::ContextHandle handle)
{
  Command* exitCommand = CommandsModule::instance()->getCommandByName(CommandId::Exit);
  UIContext::instance()->executeCommand(exitCommand);
  return 0;
}

script::result_t App_get_activeSprite(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Document* doc = UIContext::instance()->activeDocument();
  if (doc)
    ctx.pushObject(unwrap_engine(ctx)->wrapSprite(doc), "Sprite");
  else
    ctx.pushNull();
  return 1;
}

script::result_t App_get_activeImage(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Document* doc = UIContext::instance()->activeDocument();
  if (doc) {
    SpriteWrap* wrap = unwrap_engine(ctx)->wrapSprite(doc);
    ctx.pushObject(wrap->activeImage(), "Image");
  }
  else
    ctx.pushNull();
  return 1;
}

script::result_t App_get_pixelColor(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushObject();
  ctx.registerFuncs(-1, pixelColor_methods);
  return 1;
}

script::result_t App_get_version(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushString(VERSION);
  return 1;
}

const script::FunctionEntry App_methods[] = {
  { "open", App_open, 1 },
  { "exit", App_exit, 1 },
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry App_props[] = {
  { "activeImage", App_get_activeImage, nullptr },
  { "activeSprite", App_get_activeSprite, nullptr },
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
