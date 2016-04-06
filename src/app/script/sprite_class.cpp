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

#include "app/document.h"
#include "app/document_api.h"
#include "app/transaction.h"
#include "app/ui/document_view.h"
#include "app/ui_context.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "script/engine.h"

#include <map>
#include <iostream>

namespace app {

namespace {

class SpriteInScript {
public:
  SpriteInScript(app::Document* doc)
    : m_doc(doc) {
  }

  ~SpriteInScript() {
  }

  app::Document* document() {
    return m_doc;
  }

private:
  app::Document* m_doc;
};

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
    ctx.pushThis(wrap_sprite(doc.release()));
  }
  return 0;
}

script::result_t Sprite_putPixel(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int x = ctx.requireInt(0);
  int y = ctx.requireInt(1);
  doc::color_t color = ctx.requireUInt(2);

  Document* doc = (Document*)unwrap_sprite(ctx.getThis());
  DocumentView* docView = UIContext::instance()->getFirstDocumentView(doc);
  if (!docView)
    return 0;

  doc::Site site;
  docView->getSite(&site);

  int celX, celY;
  doc::Image* image = site.image(&celX, &celY, nullptr);
  if (image)
    image->putPixel(x-celX, y-celY, color);

  return 0;
}

script::result_t Sprite_getPixel(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int x = ctx.requireInt(0);
  int y = ctx.requireInt(1);

  Document* doc = (Document*)unwrap_sprite(ctx.getThis());
  DocumentView* docView = UIContext::instance()->getFirstDocumentView(doc);
  if (!docView)
    return 0;

  doc::Site site;
  docView->getSite(&site);

  int celX, celY;
  doc::Image* image = site.image(&celX, &celY, nullptr);
  if (image) {
    doc::color_t color = image->getPixel(x-celX, y-celY);
    ctx.pushUInt(color);
    return 1;
  }
  else
    return 0;
}

script::result_t Sprite_resize(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int w = ctx.requireInt(0);
  int h = ctx.requireInt(1);

  Document* doc = (Document*)unwrap_sprite(ctx.getThis());
  {
    Transaction transaction(UIContext::instance(), "Script Execution", ModifyDocument);
    DocumentApi api(doc, transaction);
    api.setSpriteSize(doc->sprite(), w, h);
    transaction.commit();
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

  Document* doc = (Document*)unwrap_sprite(ctx.getThis());
  {
    Transaction transaction(UIContext::instance(), "Script Execution", ModifyDocument);
    DocumentApi api(doc, transaction);
    api.cropSprite(doc->sprite(), gfx::Rect(x, y, w, h));
    transaction.commit();
  }

  return 0;
}

script::result_t Sprite_get_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  Document* doc = (Document*)unwrap_sprite(ctx.getThis());
  ctx.pushInt(doc->sprite()->width());
  return 1;
}

script::result_t Sprite_set_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int w = ctx.requireInt(0);
  Document* doc = (Document*)unwrap_sprite(ctx.getThis());
  doc->sprite()->setSize(w, doc->sprite()->height());
  return 0;
}

script::result_t Sprite_get_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  Document* doc = (Document*)unwrap_sprite(ctx.getThis());
  ctx.pushInt(doc->sprite()->height());
  return 1;
}

script::result_t Sprite_set_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int h = ctx.requireInt(0);
  Document* doc = (Document*)unwrap_sprite(ctx.getThis());
  doc->sprite()->setSize(doc->sprite()->width(), h);
  return 0;
}

const script::FunctionEntry Sprite_methods[] = {
  { "getPixel", Sprite_getPixel, 2 },
  { "putPixel", Sprite_putPixel, 3 },
  { "resize", Sprite_resize, 2 },
  { "crop", Sprite_crop, 4 },
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Sprite_props[] = {
  { "width", Sprite_get_width, Sprite_set_width },
  { "height", Sprite_get_height, Sprite_set_height },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

static std::map<doc::ObjectId, SpriteInScript*> g_sprites;

void* wrap_sprite(app::Document* doc)
{
  auto it = g_sprites.find(doc->id());
  if (it != g_sprites.end())
    return it->second;
  else {
    SpriteInScript* wrap = new SpriteInScript(doc);
    g_sprites[doc->id()] = wrap;
    return wrap;
  }
}

app::Document* unwrap_sprite(void* ptr)
{
  if (ptr)
    return ((SpriteInScript*)ptr)->document();
  else
    return nullptr;
}

void register_sprite_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, "Sprite", Sprite_ctor, 3, Sprite_methods, Sprite_props);
}

} // namespace app
