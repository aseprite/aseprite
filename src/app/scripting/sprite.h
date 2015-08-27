// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

namespace {

scripting::result_t Sprite_ctor(scripting::ContextHandle handle)
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

scripting::result_t Sprite_putPixel(scripting::ContextHandle handle)
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

scripting::result_t Sprite_getPixel(scripting::ContextHandle handle)
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

scripting::result_t Sprite_resize(scripting::ContextHandle handle)
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

scripting::result_t Sprite_get_width(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  Document* doc = (Document*)ctx.getThis();
  ctx.pushInt(doc->sprite()->width());
  return 1;
}

scripting::result_t Sprite_get_height(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  Document* doc = (Document*)ctx.getThis();
  ctx.pushInt(doc->sprite()->height());
  return 1;
}

scripting::result_t activeSprite_getter(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  app::Document* doc = UIContext::instance()->activeDocument();
  if (doc)
    ctx.pushObject(doc, "Sprite");
  else
    ctx.pushNull();
  return 1;
}

scripting::result_t activeSprite_setter(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  Document* doc = (Document*)ctx.requireObject(0, "Sprite");
  if (doc)
    UIContext::instance()->setActiveDocument(doc);
  return 0;
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

} // anonymous namespace
