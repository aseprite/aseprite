// Aseprite
// Copyright (C) 2015-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/sprite_class.h"

#include "app/cmd/set_sprite_size.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/script/app_scripting.h"
#include "app/script/sprite_wrap.h"
#include "app/transaction.h"
#include "app/ui/document_view.h"
#include "app/ui_context.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "script/engine.h"

namespace app {

namespace {

script::result_t Sprite_ctor(script::ContextHandle handle)
{
  script::Context ctx(handle);
  if (ctx.isConstructorCall()) {
    int w = ctx.requireInt(0);
    int h = ctx.requireInt(1);
    int colorMode = (ctx.isUndefined(2) ? IMAGE_RGB: ctx.requireInt(2));

    base::UniquePtr<Sprite> sprite(
      Sprite::createBasicSprite((doc::PixelFormat)colorMode, w, h, 256));
    base::UniquePtr<Document> doc(new Document(sprite));
    sprite.release();

    doc->setContext(UIContext::instance());
    ctx.pushThis(unwrap_engine(ctx)->wrapSprite(doc.release()));
  }
  return 0;
}

script::result_t Sprite_resize(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int w = ctx.requireInt(0);
  int h = ctx.requireInt(1);

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (wrap) {
    Document* doc = wrap->document();
    DocumentApi api(doc, wrap->transaction());
    api.setSpriteSize(doc->sprite(), w, h);
  }

  return 0;
}

script::result_t Sprite_crop(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int x = ctx.requireInt(0);
  int y = ctx.requireInt(1);
  int w = ctx.requireInt(2);
  int h = ctx.requireInt(3);

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (wrap) {
    Document* doc = wrap->document();
    DocumentApi api(doc, wrap->transaction());
    api.cropSprite(doc->sprite(), gfx::Rect(x, y, w, h));
  }

  return 0;
}

script::result_t Sprite_saveAs(script::ContextHandle handle)
{
  script::Context ctx(handle);
  const char* fn = ctx.requireString(0);

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (fn && wrap) {
    Document* doc = wrap->document();

    auto uiCtx = UIContext::instance();
    uiCtx->setActiveDocument(doc);

    Command* saveAsCommand = CommandsModule::instance()->getCommandByName(CommandId::SaveFileCopyAs);

    Params params;
    params.set("filename", fn);
    uiCtx->executeCommand(saveAsCommand, params);
  }

  return 0;
}

script::result_t Sprite_get_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.getThis();
  ctx.pushInt(wrap->sprite()->width());
  return 1;
}

script::result_t Sprite_set_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int w = ctx.requireInt(0);
  auto wrap = (SpriteWrap*)ctx.getThis();

  wrap->transaction().execute(
    new cmd::SetSpriteSize(wrap->sprite(),
                           w,
                           wrap->sprite()->height()));

  return 0;
}

script::result_t Sprite_get_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.getThis();
  ctx.pushInt(wrap->sprite()->height());
  return 1;
}

script::result_t Sprite_set_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int h = ctx.requireInt(0);
  auto wrap = (SpriteWrap*)ctx.getThis();

  wrap->transaction().execute(
    new cmd::SetSpriteSize(wrap->sprite(),
                           wrap->sprite()->width(),
                           h));

  return 0;
}

const script::FunctionEntry Sprite_methods[] = {
  { "resize", Sprite_resize, 2 },
  { "crop", Sprite_crop, 4 },
  { "saveAs", Sprite_saveAs, 2 },
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Sprite_props[] = {
  { "width", Sprite_get_width, Sprite_set_width },
  { "height", Sprite_get_height, Sprite_set_height },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_sprite_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, "Sprite", Sprite_ctor, 3, Sprite_methods, Sprite_props);
}

} // namespace app
