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

#include "app/document.h"
#include "app/script/app_object.h"
#include "app/script/console_object.h"
#include "app/script/image_class.h"
#include "app/script/image_wrap.h"
#include "app/script/selection_class.h"
#include "app/script/sprite_class.h"
#include "app/script/sprite_wrap.h"

namespace app {

AppScripting::AppScripting(script::EngineDelegate* delegate)
  : script::Engine(delegate)
{
  auto& ctx = context();
  register_app_object(ctx);
  register_console_object(ctx);

  ctx.pushGlobalObject();
  register_image_class(-1, ctx);
  register_sprite_class(-1, ctx);
  register_selection_class(-1, ctx);

  ctx.pushPointer(this);
  ctx.setProp(-2, script::kPtrId);

  ctx.pop();
}

SpriteWrap* AppScripting::wrapSprite(app::Document* doc)
{
  auto it = m_sprites.find(doc->id());
  if (it != m_sprites.end())
    return it->second;
  else {
    auto wrap = new SpriteWrap(doc);
    m_sprites[doc->id()] = wrap;
    return wrap;
  }
}

void AppScripting::onAfterEval(bool err)
{
  // Commit all transactions
  if (!err) {
    for (auto& it : m_sprites)
      it.second->commit();
  }
  destroyWrappers();
}

void AppScripting::destroyWrappers()
{
  for (auto& it : m_sprites)
    delete it.second;
  m_sprites.clear();
}

AppScripting* unwrap_engine(script::Context& ctx)
{
  ctx.pushGlobalObject();
  ctx.getProp(-1, script::kPtrId);
  void* ptr = ctx.getPointer(-1);
  ctx.pop(2);
  return (AppScripting*)ptr;
}

}
