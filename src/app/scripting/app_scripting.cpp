// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/scripting/app_scripting.h"

#include "app/document.h"
#include "app/document_api.h"
#include "app/transaction.h"
#include "app/ui/document_view.h"
#include "app/ui_context.h"
#include "doc/color.h"
#include "doc/image.h"
#include "doc/site.h"

namespace app {

#include "app/scripting/raw_color.h"
#include "app/scripting/sprite.h"

AppScripting::AppScripting(scripting::EngineDelegate* delegate)
  : scripting::Engine(delegate)
{
  registerFunction("rgba", rgba, 4);
  registerFunction("rgbaR", rgbaR, 1);
  registerFunction("rgbaG", rgbaG, 1);
  registerFunction("rgbaB", rgbaB, 1);
  registerFunction("rgbaA", rgbaA, 1);
  registerClass("Sprite", Sprite_ctor, 3, Sprite_methods, Sprite_props);
}

}
