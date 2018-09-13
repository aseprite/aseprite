// Aseprite
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/add_layer.h"
#include "app/cmd/clear_cel.h"
#include "app/cmd/remove_frame_tag.h"
#include "app/cmd/remove_layer.h"
#include "app/cmd/remove_slice.h"
#include "app/cmd/set_sprite_size.h"
#include "app/cmd/set_transparent_color.h"
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
#include "doc/frame_tag.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/slice.h"
#include "doc/sprite.h"

namespace app {
namespace script {

namespace {

int Sprite_new(lua_State* L)
{
  doc::ImageSpec spec(doc::ColorMode::RGB, 1, 1, 0);
  if (auto spec2 = may_get_obj<doc::ImageSpec>(L, 1)) {
    spec = *spec2;
  }
  else {
    const int w = lua_tointeger(L, 1);
    const int h = lua_tointeger(L, 2);
    const int colorMode = (lua_isnone(L, 3) ? IMAGE_RGB: lua_tointeger(L, 3));
    spec.setWidth(w);
    spec.setHeight(h);
    spec.setColorMode((doc::ColorMode)colorMode);
  }

  std::unique_ptr<Sprite> sprite(
    Sprite::createBasicSprite(
      (doc::PixelFormat)spec.colorMode(),
      spec.width(), spec.height(), 256));
  std::unique_ptr<Doc> doc(new Doc(sprite.get()));
  sprite.release();

  app::Context* ctx = App::instance()->context();
  doc->setContext(ctx);

  push_ptr(L, doc->sprite());
  doc.release();
  return 1;
}

int Sprite_eq(lua_State* L)
{
  const auto a = get_ptr<Sprite>(L, 1);
  const auto b = get_ptr<Sprite>(L, 2);
  lua_pushboolean(L, a->id() == b->id());
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

int Sprite_saveAs(lua_State* L)
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
    appCtx->executeCommand(saveCommand);

    doc->setFilename(fn);
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

int Sprite_setPalette(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  auto pal = get_palette_from_arg(L, 2);
  if (sprite && pal) {
    Doc* doc = static_cast<Doc*>(sprite->document());

    Tx tx;
    doc->getApi(tx).setPalette(sprite, 0, pal);
    tx.commit();
  }
  return 0;
}

int Sprite_newLayer(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  doc::Layer* newLayer = new doc::LayerImage(sprite);

  Tx tx;
  tx(new cmd::AddLayer(sprite->root(), newLayer, sprite->root()->lastLayer()));
  tx.commit();

  push_ptr(L, newLayer);
  return 1;
}

int Sprite_newGroup(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  doc::Layer* newGroup = new doc::LayerGroup(sprite);

  Tx tx;
  tx(new cmd::AddLayer(sprite->root(), newGroup, sprite->root()->lastLayer()));
  tx.commit();

  push_ptr(L, newGroup);
  return 1;
}

int Sprite_deleteLayer(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  auto layer = may_get_ptr<Layer>(L, 2);
  if (!layer && lua_isstring(L, 2)) {
    const char* layerName = lua_tostring(L, 2);
    if (layerName) {
      for (Layer* child : sprite->allLayers()) {
        if (child->name() == layerName) {
          layer = child;
          break;
        }
      }
    }
  }
  if (layer) {
    Tx tx;
    tx(new cmd::RemoveLayer(layer));
    tx.commit();
    return 0;
  }
  else {
    return luaL_error(L, "layer not found");
  }
}

int Sprite_newFrame(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  doc::frame_t frame = sprite->lastFrame()+1;
  if (lua_gettop(L) >= 2) {
    frame = lua_tointeger(L, 2)-1;
    if (frame < 0)
      return luaL_error(L, "frame index out of bounds %d", frame+1);
  }

  Doc* doc = static_cast<Doc*>(sprite->document());

  Tx tx;
  doc->getApi(tx).addFrame(sprite, frame);
  tx.commit();

  push_sprite_frame(L, sprite, frame);
  return 1;
}

int Sprite_newEmptyFrame(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  doc::frame_t frame = sprite->lastFrame()+1;
  if (lua_gettop(L) >= 1) {
    frame = lua_tointeger(L, 2)-1;
    if (frame < 0)
      return luaL_error(L, "frame index out of bounds %d", frame+1);
  }

  Doc* doc = static_cast<Doc*>(sprite->document());

  Tx tx;
  DocApi(doc, tx).addEmptyFrame(sprite, frame);
  tx.commit();

  push_sprite_frame(L, sprite, frame);
  return 1;
}

int Sprite_deleteFrame(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  doc::frame_t frame = lua_tointeger(L, 2)-1;
  if (frame < 0 || frame > sprite->lastFrame())
    return luaL_error(L, "frame index out of bounds %d", frame+1);

  Doc* doc = static_cast<Doc*>(sprite->document());

  Tx tx;
  doc->getApi(tx).removeFrame(sprite, frame);
  tx.commit();
  return 0;
}

int Sprite_newCel(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  auto layerBase = get_ptr<Layer>(L, 2);
  if (!layerBase->isImage())
    return luaL_error(L, "unexpected kinf of layer in Sprite:newCel()");

  frame_t frame = lua_tointeger(L, 3)-1;
  if (frame < 0 || frame > sprite->lastFrame())
    return luaL_error(L, "frame index out of bounds %d", frame+1);

  LayerImage* layer = static_cast<LayerImage*>(layerBase);
  ImageRef image(nullptr);
  gfx::Point pos(0, 0);

  Image* srcImage = may_get_image_from_arg(L, 4);
  if (srcImage) {
    image.reset(Image::createCopy(srcImage));
    pos = convert_args_into_point(L, 5);
  }
  else {
    image.reset(Image::create(sprite->spec()));
  }

  auto cel = new Cel(frame, image);
  cel->setPosition(pos);

  Doc* doc = static_cast<Doc*>(sprite->document());

  Tx tx;
  DocApi api = doc->getApi(tx);
  if (layer->cel(frame))
    api.clearCel(layer, frame);
  api.addCel(layer, cel);
  tx.commit();

  push_ptr(L, cel);
  return 1;
}

int Sprite_deleteCel(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  (void)sprite;                 // unused

  auto cel = may_get_ptr<doc::Cel>(L, 2);
  if (!cel) {
    if (auto layer = may_get_ptr<doc::Layer>(L, 2)) {
      doc::frame_t frame = lua_tointeger(L, 3);
      if (layer->isImage())
        cel = static_cast<doc::LayerImage*>(layer)->cel(frame);
    }
  }

  if (cel) {
    Tx tx;
    tx(new cmd::ClearCel(cel));
    tx.commit();
    return 0;
  }
  else {
    return luaL_error(L, "cel not found");
  }
}

int Sprite_newTag(lua_State* L)
{
  auto sprite = get_ptr<doc::Sprite>(L, 1);
  auto from = lua_tointeger(L, 2)-1;
  auto to = lua_tointeger(L, 3)-1;
  auto tag = new doc::FrameTag(from, to);
  sprite->frameTags().add(tag);
  push_ptr(L, tag);
  return 1;
}

int Sprite_deleteTag(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  auto tag = may_get_ptr<FrameTag>(L, 2);
  if (!tag && lua_isstring(L, 2)) {
    const char* tagName = lua_tostring(L, 2);
    if (tagName)
      tag = sprite->frameTags().getByName(tagName);
  }
  if (tag) {
    Tx tx;
    tx(new cmd::RemoveFrameTag(sprite, tag));
    tx.commit();
    return 0;
  }
  else {
    return luaL_error(L, "tag not found");
  }
}

int Sprite_newSlice(lua_State* L)
{
  auto sprite = get_ptr<doc::Sprite>(L, 1);
  auto slice = new doc::Slice();

  gfx::Rect bounds = convert_args_into_rect(L, 2);
  if (!bounds.isEmpty())
    slice->insert(0, doc::SliceKey(bounds));

  sprite->slices().add(slice);
  push_ptr(L, slice);
  return 1;
}

int Sprite_deleteSlice(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  doc::Slice* slice = may_get_ptr<Slice>(L, 2);
  if (!slice && lua_isstring(L, 2)) {
    const char* sliceName = lua_tostring(L, 2);
    if (sliceName)
      slice = sprite->slices().getByName(sliceName);
  }
  if (slice) {
    Tx tx;
    tx(new cmd::RemoveSlice(sprite, slice));
    tx.commit();
    return 0;
  }
  else {
    return luaL_error(L, "slice not found");
  }
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

int Sprite_get_spec(lua_State* L)
{
  const auto sprite = get_ptr<Sprite>(L, 1);
  push_obj(L, sprite->spec());
  return 1;
}

int Sprite_get_selection(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  push_sprite_selection(L, sprite);
  return 1;
}

int Sprite_get_frames(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  push_sprite_frames(L, sprite);
  return 1;
}

int Sprite_get_palettes(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  push_sprite_palettes(L, sprite);
  return 1;
}

int Sprite_get_layers(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  push_sprite_layers(L, sprite);
  return 1;
}

int Sprite_get_cels(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  push_sprite_cels(L, sprite);
  return 1;
}

int Sprite_get_tags(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  push_sprite_tags(L, sprite);
  return 1;
}

int Sprite_get_slices(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  push_sprite_slices(L, sprite);
  return 1;
}

int Sprite_get_backgroundLayer(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  doc::Layer* layer = sprite->backgroundLayer();
  if (layer)
    push_ptr(L, layer);
  else
    lua_pushnil(L);
  return 1;
}

int Sprite_get_transparentColor(lua_State* L)
{
  const auto sprite = get_ptr<Sprite>(L, 1);
  lua_pushinteger(L, sprite->transparentColor());
  return 1;
}

int Sprite_set_transparentColor(lua_State* L)
{
  auto sprite = get_ptr<Sprite>(L, 1);
  const int index = lua_tointeger(L, 2);
  Tx tx;
  tx(new cmd::SetTransparentColor(sprite, index));
  tx.commit();
  return 0;
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
  { "__eq", Sprite_eq },
  { "resize", Sprite_resize },
  { "crop", Sprite_crop },
  { "saveAs", Sprite_saveAs },
  { "saveCopyAs", Sprite_saveCopyAs },
  { "loadPalette", Sprite_loadPalette },
  { "setPalette", Sprite_setPalette },
  // Layers
  { "newLayer", Sprite_newLayer },
  { "newGroup", Sprite_newGroup },
  { "deleteLayer", Sprite_deleteLayer },
  // Frames
  { "newFrame", Sprite_newFrame },
  { "newEmptyFrame", Sprite_newEmptyFrame },
  { "deleteFrame", Sprite_deleteFrame },
  // Cel
  { "newCel", Sprite_newCel },
  { "deleteCel", Sprite_deleteCel },
  // Tag
  { "newTag", Sprite_newTag },
  { "deleteTag", Sprite_deleteTag },
  // Slices
  { "newSlice", Sprite_newSlice },
  { "deleteSlice", Sprite_deleteSlice },
  { nullptr, nullptr }
};

const Property Sprite_properties[] = {
  { "filename", Sprite_get_filename, nullptr },
  { "width", Sprite_get_width, Sprite_set_width },
  { "height", Sprite_get_height, Sprite_set_height },
  { "colorMode", Sprite_get_colorMode, nullptr },
  { "spec", Sprite_get_spec, nullptr },
  { "selection", Sprite_get_selection, nullptr },
  { "frames", Sprite_get_frames, nullptr },
  { "palettes", Sprite_get_palettes, nullptr },
  { "layers", Sprite_get_layers, nullptr },
  { "cels", Sprite_get_cels, nullptr },
  { "tags", Sprite_get_tags, nullptr },
  { "slices", Sprite_get_slices, nullptr },
  { "backgroundLayer", Sprite_get_backgroundLayer, nullptr },
  { "transparentColor", Sprite_get_transparentColor, Sprite_set_transparentColor },
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
