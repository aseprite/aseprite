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

static scripting::result_t rgba(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  int r = ctx.requireInt(0);
  int g = ctx.requireInt(1);
  int b = ctx.requireInt(2);
  int a = (ctx.isUndefined(3) ? 255: ctx.requireInt(3));
  ctx.pushUInt(doc::rgba(r, g, b, a));
  return 1;
}

static scripting::result_t rgbaR(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getr(ctx.requireUInt(0)));
  return 1;
}

static scripting::result_t rgbaG(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getg(ctx.requireUInt(0)));
  return 1;
}

static scripting::result_t rgbaB(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getb(ctx.requireUInt(0)));
  return 1;
}

static scripting::result_t rgbaA(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  ctx.pushUInt(doc::rgba_geta(ctx.requireUInt(0)));
  return 1;
}

static scripting::result_t Sprite_ctor(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  if (ctx.isConstructorCall()) {
    int w = ctx.requireInt(0);
    int h = ctx.requireInt(1);
    int colorMode = (ctx.isUndefined(2) ? IMAGE_RGB: ctx.requireInt(2));

    base::UniquePtr<Sprite> sprite(
      Sprite::createBasicSprite((doc::PixelFormat)colorMode, w, h, 256));
    base::UniquePtr<Document> doc(new Document(sprite));
    sprite.release();

    doc->setContext(UIContext::instance());
    ctx.pushThis(doc.release());
  }
  return 0;
}

static scripting::result_t Sprite_putPixel(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  int x = ctx.requireInt(0);
  int y = ctx.requireInt(1);
  doc::color_t color = ctx.requireUInt(2);

  Document* doc = (Document*)ctx.getThis();
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

static scripting::result_t Sprite_getPixel(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  int x = ctx.requireInt(0);
  int y = ctx.requireInt(1);

  Document* doc = (Document*)ctx.getThis();
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

static scripting::result_t Sprite_resize(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  int w = ctx.requireInt(0);
  int h = ctx.requireInt(1);

  Document* doc = (Document*)ctx.getThis();
  {
    Transaction transaction(UIContext::instance(), "Script Execution", ModifyDocument);
    DocumentApi api(doc, transaction);
    api.setSpriteSize(doc->sprite(), w, h);
    transaction.commit();
  }

  return 0;
}

static scripting::result_t Sprite_get_width(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  Document* doc = (Document*)ctx.getThis();
  ctx.pushInt(doc->sprite()->width());
  return 1;
}

static scripting::result_t Sprite_get_height(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  Document* doc = (Document*)ctx.getThis();
  ctx.pushInt(doc->sprite()->height());
  return 1;
}

const scripting::FunctionEntry Sprite_methods[] = {
  { "putPixel", Sprite_putPixel, 3 },
  { "getPixel", Sprite_getPixel, 2 },
  { "resize", Sprite_resize, 2 },
  { nullptr, nullptr, 0 }
};

const scripting::PropertyEntry Sprite_props[] = {
  { "width", Sprite_get_width, nullptr },
  { "height", Sprite_get_height, nullptr },
  { nullptr, nullptr, 0 }
};

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
