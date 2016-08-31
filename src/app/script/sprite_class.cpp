// Aseprite
// Copyright (C) 2015-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/sprite_class.h"

#include "app/cmd/set_sprite_size.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/document.h"
#include "app/document_api.h"
#include "app/file/palette_file.h"
#include "app/script/app_scripting.h"
#include "app/script/sprite_wrap.h"
#include "app/transaction.h"
#include "app/ui/document_view.h"
#include "app/ui_context.h"
#include "doc/mask.h"
#include "doc/palette.h"
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
    ctx.pushThis(unwrap_engine(ctx)->wrapSprite(doc.release()), "Sprite");
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
    wrap->commitImages();

    Document* doc = wrap->document();
    DocumentApi api(doc, wrap->transaction());
    api.setSpriteSize(doc->sprite(), w, h);
  }

  return 0;
}

script::result_t Sprite_crop(script::ContextHandle handle)
{
  script::Context ctx(handle);
  gfx::Rect bounds;

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (wrap) {
    wrap->commitImages();

    Document* doc = wrap->document();

    // Use mask bounds
    if (ctx.isUndefined(0)) {
      if (doc->isMaskVisible())
        bounds = doc->mask()->bounds();
      else
        bounds = doc->sprite()->bounds();
    }
    else {
      bounds.x = ctx.requireInt(0);
      bounds.y = ctx.requireInt(1);
      bounds.w = ctx.requireInt(2);
      bounds.h = ctx.requireInt(3);
    }

    if (!bounds.isEmpty()) {
      DocumentApi api(doc, wrap->transaction());
      api.cropSprite(doc->sprite(), bounds);
    }
  }

  return 0;
}

script::result_t Sprite_save(script::ContextHandle handle)
{
  script::Context ctx(handle);

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (wrap) {
    wrap->commit();

    auto doc = wrap->document();
    auto uiCtx = UIContext::instance();
    uiCtx->setActiveDocument(doc);
    Command* saveCommand = CommandsModule::instance()->getCommandByName(CommandId::SaveFile);
    uiCtx->executeCommand(saveCommand);
  }

  return 0;
}

script::result_t Sprite_saveAs(script::ContextHandle handle)
{
  script::Context ctx(handle);
  const char* fn = ctx.requireString(0);
  bool asCopy = ctx.getBool(1);

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (fn && wrap) {
    wrap->commit();

    auto doc = wrap->document();
    auto uiCtx = UIContext::instance();
    uiCtx->setActiveDocument(doc);

    Command* saveCommand =
      CommandsModule::instance()->getCommandByName(
        (asCopy ? CommandId::SaveFileCopyAs:
                  CommandId::SaveFile));

    Params params;
    if (asCopy)
      params.set("filename", fn);
    else
      doc->setFilename(fn);
    uiCtx->executeCommand(saveCommand, params);
  }

  return 0;
}

script::result_t Sprite_loadPalette(script::ContextHandle handle)
{
  script::Context ctx(handle);
  const char* fn = ctx.requireString(0);

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (fn && wrap) {
    auto doc = wrap->document();
    base::UniquePtr<doc::Palette> palette(load_palette(fn));
    if (palette) {
      // TODO Merge this with the code in LoadPaletteCommand
      doc->getApi(wrap->transaction()).setPalette(
        wrap->sprite(), 0, palette);
    }
  }

  return 0;
}

script::result_t Sprite_get_filename(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.getThis();
  ctx.pushString(wrap->document()->filename().c_str());
  return 1;
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

script::result_t Sprite_get_colorMode(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.getThis();
  ctx.pushInt(wrap->sprite()->pixelFormat());
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

script::result_t Sprite_get_selection(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.getThis();
  ctx.pushObject(wrap, "Selection");
  return 1;
}

const script::FunctionEntry Sprite_methods[] = {
  { "resize", Sprite_resize, 2 },
  { "crop", Sprite_crop, 4 },
  { "save", Sprite_save, 2 },
  { "saveAs", Sprite_saveAs, 2 },
  { "loadPalette", Sprite_loadPalette, 1 },
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Sprite_props[] = {
  { "filename", Sprite_get_filename, nullptr },
  { "width", Sprite_get_width, Sprite_set_width },
  { "height", Sprite_get_height, Sprite_set_height },
  { "colorMode", Sprite_get_colorMode, nullptr },
  { "selection", Sprite_get_selection, nullptr },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_sprite_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, "Sprite", Sprite_ctor, 3, Sprite_methods, Sprite_props);
}

} // namespace app
