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
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/site.h"
#include "app/transaction.h"
#include "app/tx.h"
#include "app/ui/doc_view.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/sprite.h"

namespace app {
namespace script {

namespace {

int Sprite_new(lua_State* L)
{
  const int w = lua_tointeger(L, 1);
  const int h = lua_tointeger(L, 2);
  const int colorMode = (lua_isnone(L, 3) ? IMAGE_RGB: lua_tointeger(L, 3));

  std::unique_ptr<Sprite> sprite(
    Sprite::createBasicSprite((doc::PixelFormat)colorMode, w, h, 256));
  std::unique_ptr<Doc> doc(new Doc(sprite.get()));
  sprite.release();

  app::Context* ctx = App::instance()->context();
  doc->setContext(ctx);

  push_ptr(L, doc->sprite());
  doc.release();
  return 1;
}

int Sprite_resize(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  const gfx::Size size = convert_args_into_size(L, 2);
  Doc* doc = static_cast<Doc*>(sprite->document());
  Tx tx;
  DocApi(doc, tx).setSpriteSize(doc->sprite(), size.w, size.h);
  tx.commit();
  return 0;
}

int Sprite_crop(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  Doc* doc = static_cast<Doc*>(sprite->document());
  gfx::Rect bounds;

  // Use mask bounds
  if (lua_isnone(L, 2)) {
    if (doc->isMaskVisible())
      bounds = doc->mask()->bounds();
    else
      bounds = sprite->bounds();
  }
  else {
    bounds = convert_args_into_rect(L, 2);
  }

  if (!bounds.isEmpty()) {
    Tx tx;
    DocApi(doc, tx).cropSprite(sprite, bounds);
    tx.commit();
  }
  return 0;
}

int Sprite_save(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  if (sprite) {
    Doc* doc = static_cast<Doc*>(sprite->document());
    app::Context* appCtx = App::instance()->context();
    appCtx->setActiveDocument(doc);
    Command* saveCommand =
      Commands::instance()->byId(CommandId::SaveFile());
    appCtx->executeCommand(saveCommand);
  }
  return 0;
}

int Sprite_saveAs(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  const char* fn = luaL_checkstring(L, 2);
  if (fn && sprite) {
    Doc* doc = static_cast<Doc*>(sprite->document());
    app::Context* appCtx = App::instance()->context();
    appCtx->setActiveDocument(doc);

    Command* saveCommand =
      Commands::instance()->byId(CommandId::SaveFile());

    Params params;
    doc->setFilename(fn);
    appCtx->executeCommand(saveCommand, params);
  }
  return 0;
}

int Sprite_saveCopyAs(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  const char* fn = luaL_checkstring(L, 2);
  if (fn && sprite) {
    Doc* doc = static_cast<Doc*>(sprite->document());
    app::Context* appCtx = App::instance()->context();
    appCtx->setActiveDocument(doc);

    Command* saveCommand =
      Commands::instance()->byId(CommandId::SaveFileCopyAs());

    Params params;
    params.set("filename", fn);
    appCtx->executeCommand(saveCommand, params);
  }
  return 0;
}

int Sprite_loadPalette(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  const char* fn = luaL_checkstring(L, 2);
  if (fn && sprite) {
    Doc* doc = static_cast<Doc*>(sprite->document());
    std::unique_ptr<doc::Palette> palette(load_palette(fn));
    if (palette) {
      Tx tx;
      // TODO Merge this with the code in LoadPaletteCommand
      doc->getApi(tx).setPalette(sprite, 0, palette.get());
      tx.commit();
    }
  }
  return 0;
}

int Sprite_get_filename(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  lua_pushstring(L, sprite->document()->filename().c_str());
  return 1;
}

int Sprite_get_width(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  lua_pushinteger(L, sprite->width());
  return 1;
}

int Sprite_get_height(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  lua_pushinteger(L, sprite->height());
  return 1;
}

int Sprite_get_colorMode(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  lua_pushinteger(L, sprite->pixelFormat());
  return 1;
}

int Sprite_get_selection(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  push_sprite_selection(L, sprite);
  return 1;
}

int Sprite_set_width(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  const int width = lua_tointeger(L, 2);
  Tx tx;
  tx(new cmd::SetSpriteSize(sprite, width, sprite->height()));
  tx.commit();
  return 0;
}

int Sprite_set_height(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  const int height = lua_tointeger(L, 2);
  Tx tx;
  tx(new cmd::SetSpriteSize(sprite, sprite->width(), height));
  tx.commit();
  return 0;
}

const luaL_Reg Sprite_methods[] = {
  { "resize", Sprite_resize },
  { "crop", Sprite_crop },
  { "save", Sprite_save },
  { "saveAs", Sprite_saveAs },
  { "saveCopyAs", Sprite_saveCopyAs },
  { "loadPalette", Sprite_loadPalette },
  { nullptr, nullptr }
};

const Property Sprite_properties[] = {
  { "filename", Sprite_get_filename, nullptr },
  { "width", Sprite_get_width, Sprite_set_width },
  { "height", Sprite_get_height, Sprite_set_height },
  { "colorMode", Sprite_get_colorMode, nullptr },
  { "selection", Sprite_get_selection, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(doc::Sprite);

void register_sprite_class(lua_State* L)
{
  using doc::Sprite;
  REG_CLASS(L, Sprite);
  REG_CLASS_NEW(L, Sprite);
  REG_CLASS_PROPERTIES(L, Sprite);
}

} // namespace script
} // namespace app
