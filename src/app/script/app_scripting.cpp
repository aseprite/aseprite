// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/app_scripting.h"

#include "app/doc.h"
#include "app/script/image_wrap.h"
#include "app/script/sprite_wrap.h"

namespace app {

namespace {

const script::ConstantEntry ColorMode_constants[] = {
  { "RGB", double(IMAGE_RGB) },
  { "GRAYSCALE", double(IMAGE_GRAYSCALE) },
  { "INDEXED", double(IMAGE_INDEXED) },
  { nullptr, 0.0 }
};

}

void register_app_object(script::Context& ctx);
void register_console_object(script::Context& ctx);

void register_image_class(script::index_t idx, script::Context& ctx);
void register_pixel_color_class(script::index_t idx, script::Context& ctx);
void register_point_class(script::index_t idx, script::Context& ctx);
void register_rectangle_class(script::index_t idx, script::Context& ctx);
void register_selection_class(script::index_t idx, script::Context& ctx);
void register_size_class(script::index_t idx, script::Context& ctx);
void register_sprite_class(script::index_t idx, script::Context& ctx);

AppScripting::AppScripting(script::EngineDelegate* delegate)
  : script::Engine(delegate)
{
  auto& ctx = context();
  ctx.setContextUserData(this);

  // Register global objects (app and console)
  register_app_object(ctx);
  register_console_object(ctx);

  ctx.pushGlobalObject();

  // Register constants
  {
    ctx.newObject();
    ctx.registerConstants(-1, ColorMode_constants);
    ctx.setProp(-2, "ColorMode");
  }

  // Register classes/prototypes
  register_image_class(-1, ctx);
  register_pixel_color_class(-1, ctx);
  register_point_class(-1, ctx);
  register_rectangle_class(-1, ctx);
  register_selection_class(-1, ctx);
  register_size_class(-1, ctx);
  register_sprite_class(-1, ctx);

  ctx.pop(1);
}

SpriteWrap* AppScripting::wrapSprite(Doc* doc)
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
  return (AppScripting*)ctx.getContextUserData();
}

}
