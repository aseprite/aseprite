// Aseprite
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/set_sprite_size.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/file/palette_file.h"
#include "app/script/app_scripting.h"
#include "app/script/sprite_wrap.h"
#include "app/site.h"
#include "app/transaction.h"
#include "app/ui/doc_view.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/sprite.h"
#include "script/engine.h"

namespace app {

namespace {

const char* kTag = "Sprite";

void Sprite_new(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int w = ctx.requireInt(1);
  int h = ctx.requireInt(2);
  int colorMode = (ctx.isUndefined(3) ? IMAGE_RGB: ctx.requireInt(3));

  std::unique_ptr<Sprite> sprite(
    Sprite::createBasicSprite((doc::PixelFormat)colorMode, w, h, 256));
  std::unique_ptr<Doc> doc(new Doc(sprite.get()));
  sprite.release();

  app::Context* appCtx = App::instance()->context();
  doc->setContext(appCtx);

  ctx.newObject(kTag, unwrap_engine(ctx)->wrapSprite(doc.release()), nullptr);
}

void Sprite_resize(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  gfx::Size size = convert_args_into_size(ctx);

  if (wrap) {
    wrap->commitImages();

    Doc* doc = wrap->document();
    DocApi api(doc, wrap->transaction());
    api.setSpriteSize(doc->sprite(), size.w, size.h);
  }

  ctx.pushUndefined();
}

void Sprite_crop(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);

  if (wrap) {
    wrap->commitImages();

    Doc* doc = wrap->document();
    gfx::Rect bounds;

    // Use mask bounds
    if (ctx.isUndefined(1)) {
      if (doc->isMaskVisible())
        bounds = doc->mask()->bounds();
      else
        bounds = doc->sprite()->bounds();
    }
    else {
      bounds = convert_args_into_rectangle(ctx);
    }

    if (!bounds.isEmpty()) {
      DocApi api(doc, wrap->transaction());
      api.cropSprite(doc->sprite(), bounds);
    }
  }

  ctx.pushUndefined();
}

void Sprite_save(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);

  if (wrap) {
    wrap->commit();

    Doc* doc = wrap->document();
    app::Context* appCtx = App::instance()->context();
    appCtx->setActiveDocument(doc);
    Command* saveCommand =
      Commands::instance()->byId(CommandId::SaveFile());
    appCtx->executeCommand(saveCommand);
  }

  ctx.pushUndefined();
}

void Sprite_saveAs(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  const char* fn = ctx.requireString(1);

  if (fn && wrap) {
    wrap->commit();

    Doc* doc = wrap->document();
    app::Context* appCtx = App::instance()->context();
    appCtx->setActiveDocument(doc);

    Command* saveCommand =
      Commands::instance()->byId(CommandId::SaveFile());

    Params params;
    doc->setFilename(fn);
    appCtx->executeCommand(saveCommand, params);
  }

  ctx.pushUndefined();
}

void Sprite_saveCopyAs(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  const char* fn = ctx.requireString(1);

  if (fn && wrap) {
    wrap->commit();

    Doc* doc = wrap->document();
    app::Context* appCtx = App::instance()->context();
    appCtx->setActiveDocument(doc);

    Command* saveCommand =
      Commands::instance()->byId(CommandId::SaveFileCopyAs());

    Params params;
    params.set("filename", fn);
    appCtx->executeCommand(saveCommand, params);
  }

  ctx.pushUndefined();
}

void Sprite_loadPalette(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  const char* fn = ctx.toString(1);

  if (fn && wrap) {
    Doc* doc = wrap->document();
    std::unique_ptr<doc::Palette> palette(load_palette(fn));
    if (palette) {
      // TODO Merge this with the code in LoadPaletteCommand
      doc->getApi(wrap->transaction()).setPalette(
        wrap->sprite(), 0, palette.get());
    }
  }

  ctx.pushUndefined();
}

void Sprite_get_filename(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  ctx.pushString(wrap->document()->filename().c_str());
}

void Sprite_get_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  ctx.pushInt(wrap->sprite()->width());
}

void Sprite_get_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  ctx.pushInt(wrap->sprite()->height());
}

void Sprite_set_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  const int width = ctx.requireInt(1);
  wrap->transaction().execute(
    new cmd::SetSpriteSize(wrap->sprite(),
                           width,
                           wrap->sprite()->height()));
  ctx.pushUndefined();
}

void Sprite_set_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  const int height = ctx.requireInt(1);
  wrap->transaction().execute(
    new cmd::SetSpriteSize(wrap->sprite(),
                           wrap->sprite()->width(),
                           height));
  ctx.pushUndefined();
}

void Sprite_get_colorMode(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  ctx.pushInt(wrap->sprite()->pixelFormat());
}

void Sprite_set_colorMode(script::ContextHandle handle)
{
  script::Context ctx(handle);
  // TODO
  // auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  ctx.pushUndefined();
}

void Sprite_get_selection(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.toUserData(0, kTag);
  push_new_selection(ctx, wrap);
}

const script::FunctionEntry Sprite_methods[] = {
  { "resize", Sprite_resize, 2 },
  { "crop", Sprite_crop, 4 },
  { "save", Sprite_save, 2 },
  { "saveAs", Sprite_saveAs, 2 },
  { "saveCopyAs", Sprite_saveCopyAs, 2 },
  { "loadPalette", Sprite_loadPalette, 1 },
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Sprite_props[] = {
  { "filename", Sprite_get_filename, nullptr },
  { "width", Sprite_get_width, Sprite_set_width },
  { "height", Sprite_get_height, Sprite_set_height },
  { "colorMode", Sprite_get_colorMode, Sprite_set_colorMode },
  { "selection", Sprite_get_selection, nullptr },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_sprite_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, kTag,
                    Sprite_new, 3,
                    Sprite_methods, Sprite_props);
}

} // namespace app
