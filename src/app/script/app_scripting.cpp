// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/app_scripting.h"

#include "app/script/app_object.h"
#include "app/script/console_object.h"
#include "app/script/sprite_class.h"

namespace app {

AppScripting::AppScripting(script::EngineDelegate* delegate)
  : script::Engine(delegate)
{
  auto& ctx = context();
  register_app_object(ctx);
  register_console_object(ctx);

  ctx.pushGlobalObject();
  register_sprite_class(-1, ctx);
  ctx.pop();
}

}
