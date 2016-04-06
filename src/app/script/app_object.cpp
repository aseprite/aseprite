// Aseprite
// Copyright (C) 2015-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/console_object.h"

#include "app/script/sprite_class.h"
#include "app/ui_context.h"
#include "script/engine.h"

// App sub-objects
#include "app/script/pixel_color.h"

#include <iostream>

namespace app {

namespace {

script::result_t App_get_activeSprite(script::ContextHandle handle)
{
  script::Context ctx(handle);
  app::Document* doc = UIContext::instance()->activeDocument();
  if (doc)
    ctx.pushObject(wrap_sprite(doc), "Sprite");
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
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry App_props[] = {
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
