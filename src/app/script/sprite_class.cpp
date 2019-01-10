// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/cmd/add_layer.h"
#include "app/cmd/assign_color_profile.h"
#include "app/cmd/clear_cel.h"
#include "app/cmd/convert_color_profile.h"
#include "app/cmd/remove_frame_tag.h"
#include "app/cmd/remove_layer.h"
#include "app/cmd/remove_slice.h"
#include "app/cmd/set_sprite_size.h"
#include "app/cmd/set_transparent_color.h"
#include "app/color_spaces.h"
#include "app/commands/commands.h"
#include "app/commands/params.h"
#include "app/context.h"
#include "app/doc.h"
#include "app/doc_access.h"
#include "app/doc_api.h"
#include "app/file/palette_file.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/security.h"
#include "app/site.h"
#include "app/transaction.h"
#include "app/tx.h"
#include "app/ui/doc_view.h"
#include "base/fs.h"
#include "doc/frame_tag.h"
#include "doc/layer.h"
#include "doc/mask.h"
#include "doc/palette.h"
#include "doc/slice.h"
#include "doc/sprite.h"

#include <algorithm>

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
    spec.setColorSpace(get_working_rgb_space_from_preferences());
  }

  std::unique_ptr<Sprite> sprite(Sprite::createBasicSprite(spec, 256));
  std::unique_ptr<Doc> doc(new Doc(sprite.get()));
  sprite.release();

  app::Context* ctx = App::instance()->context();
  doc->setContext(ctx);

  push_docobj(L, doc->sprite());
  doc.release();
  return 1;
}

int Sprite_eq(lua_State* L)
{
  const auto a = get_docobj<Sprite>(L, 1);
  const auto b = get_docobj<Sprite>(L, 2);
  lua_pushboolean(L, a->id() == b->id());
  return 1;
}

int Sprite_resize(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  gfx::Size size = convert_args_into_size(L, 2);

  // Fix invalid sizes
  size.w = std::max(1, size.w);
  size.h = std::max(1, size.h);

  Doc* doc = static_cast<Doc*>(sprite->document());
  Tx tx;
  DocApi(doc, tx).setSpriteSize(doc->sprite(), size.w, size.h);
  tx.commit();
  return 0;
}

int Sprite_crop(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
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

int Sprite_saveAs_base(lua_State* L, std::string& absFn)
{
  bool result = false;
  auto sprite = get_docobj<Sprite>(L, 1);
  const char* fn = luaL_checkstring(L, 2);
  if (fn && sprite) {
    Doc* doc = static_cast<Doc*>(sprite->document());
    app::Context* appCtx = App::instance()->context();
    appCtx->setActiveDocument(doc);

    absFn = base::get_absolute_path(fn);
    if (!ask_access(L, absFn.c_str(), FileAccessMode::Write, true))
      return luaL_error(L, "script doesn't have access to write file %s",
                        absFn.c_str());

    Command* saveCommand =
      Commands::instance()->byId(CommandId::SaveFileCopyAs());

    Params params;
    params.set("filename", absFn.c_str());
    params.set("useUI", "false");
    appCtx->executeCommand(saveCommand, params);

    result = true;
  }
  lua_pushboolean(L, result);
  return 1;
}

int Sprite_saveAs(lua_State* L)
{
  std::string fn;
  int res = Sprite_saveAs_base(L, fn);
  if (!fn.empty()) {
    auto sprite = get_docobj<Sprite>(L, 1);
    if (sprite) {
      Doc* doc = static_cast<Doc*>(sprite->document());
      doc->setFilename(fn);
      doc->markAsSaved();
    }
  }
  return res;
}

int Sprite_saveCopyAs(lua_State* L)
{
  std::string fn;
  return Sprite_saveAs_base(L, fn);
}

int Sprite_close(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  Doc* doc = static_cast<Doc*>(sprite->document());
  try {
    DocDestroyer destroyer(static_cast<app::Context*>(doc->context()), doc, 500);
    destroyer.destroyDocument();
    return 0;
  }
  catch (const LockedDocException& ex) {
    return luaL_error(L, "cannot lock document to close it\n%s", ex.what());
  }
}

int Sprite_loadPalette(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  const char* fn = luaL_checkstring(L, 2);
  if (fn && sprite) {
    std::string absFn = base::get_absolute_path(fn);
    if (!ask_access(L, absFn.c_str(), FileAccessMode::Read, true))
      return luaL_error(L, "script doesn't have access to open file %s",
                        absFn.c_str());

    Doc* doc = static_cast<Doc*>(sprite->document());
    std::unique_ptr<doc::Palette> palette(load_palette(absFn.c_str()));
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
  auto sprite = get_docobj<Sprite>(L, 1);
  auto pal = get_palette_from_arg(L, 2);
  if (sprite && pal) {
    Doc* doc = static_cast<Doc*>(sprite->document());

    Tx tx;
    doc->getApi(tx).setPalette(sprite, 0, pal);
    tx.commit();
  }
  return 0;
}

int Sprite_assignColorSpace(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  auto cs = get_obj<gfx::ColorSpace>(L, 2);
  Tx tx;
  tx(new cmd::AssignColorProfile(
       sprite, std::make_shared<gfx::ColorSpace>(*cs)));
  tx.commit();
  return 1;
}

int Sprite_convertColorSpace(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  auto cs = get_obj<gfx::ColorSpace>(L, 2);
  Tx tx;
  tx(new cmd::ConvertColorProfile(
       sprite, std::make_shared<gfx::ColorSpace>(*cs)));
  tx.commit();
  return 1;
}

int Sprite_newLayer(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  doc::Layer* newLayer = new doc::LayerImage(sprite);

  Tx tx;
  tx(new cmd::AddLayer(sprite->root(), newLayer, sprite->root()->lastLayer()));
  tx.commit();

  push_docobj(L, newLayer);
  return 1;
}

int Sprite_newGroup(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  doc::Layer* newGroup = new doc::LayerGroup(sprite);

  Tx tx;
  tx(new cmd::AddLayer(sprite->root(), newGroup, sprite->root()->lastLayer()));
  tx.commit();

  push_docobj(L, newGroup);
  return 1;
}

int Sprite_deleteLayer(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  auto layer = may_get_docobj<Layer>(L, 2);
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
  auto sprite = get_docobj<Sprite>(L, 1);
  doc::frame_t frame = sprite->lastFrame()+1;
  doc::frame_t copyThis = frame;
  if (lua_gettop(L) >= 2) {
    frame = get_frame_number_from_arg(L, 2);
    if (frame < 0)
      return luaL_error(L, "frame index out of bounds %d", frame+1);
    copyThis = frame+1; // addFrame() copies the previous frame of the given one.
  }

  Doc* doc = static_cast<Doc*>(sprite->document());

  Tx tx;
  doc->getApi(tx).addFrame(sprite, copyThis);
  tx.commit();

  push_sprite_frame(L, sprite, frame);
  return 1;
}

int Sprite_newEmptyFrame(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  doc::frame_t frame = sprite->lastFrame()+1;
  if (lua_gettop(L) >= 2) {
    frame = get_frame_number_from_arg(L, 2);
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
  auto sprite = get_docobj<Sprite>(L, 1);
  doc::frame_t frame = get_frame_number_from_arg(L, 2);
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
  auto sprite = get_docobj<Sprite>(L, 1);
  auto layerBase = get_docobj<Layer>(L, 2);
  if (!layerBase->isImage())
    return luaL_error(L, "unexpected kinf of layer in Sprite:newCel()");

  frame_t frame = get_frame_number_from_arg(L, 3);
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

  push_docobj(L, cel);
  return 1;
}

int Sprite_deleteCel(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  (void)sprite;                 // unused

  auto cel = may_get_docobj<doc::Cel>(L, 2);
  if (!cel) {
    if (auto layer = may_get_docobj<doc::Layer>(L, 2)) {
      doc::frame_t frame = get_frame_number_from_arg(L, 3);
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
  auto sprite = get_docobj<Sprite>(L, 1);
  auto from = get_frame_number_from_arg(L, 2);
  auto to = get_frame_number_from_arg(L, 3);
  auto tag = new doc::FrameTag(from, to);
  sprite->frameTags().add(tag);
  push_docobj(L, tag);
  return 1;
}

int Sprite_deleteTag(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  auto tag = may_get_docobj<FrameTag>(L, 2);
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
  auto sprite = get_docobj<Sprite>(L, 1);
  auto slice = new doc::Slice();

  gfx::Rect bounds = convert_args_into_rect(L, 2);
  if (!bounds.isEmpty())
    slice->insert(0, doc::SliceKey(bounds));

  sprite->slices().add(slice);
  push_docobj(L, slice);
  return 1;
}

int Sprite_deleteSlice(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  doc::Slice* slice = may_get_docobj<Slice>(L, 2);
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
  auto sprite = get_docobj<Sprite>(L, 1);
  lua_pushstring(L, sprite->document()->filename().c_str());
  return 1;
}

int Sprite_get_width(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  lua_pushinteger(L, sprite->width());
  return 1;
}

int Sprite_get_height(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  lua_pushinteger(L, sprite->height());
  return 1;
}

int Sprite_get_colorMode(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  lua_pushinteger(L, sprite->pixelFormat());
  return 1;
}

int Sprite_get_colorSpace(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  auto cs = sprite->colorSpace();
  if (cs)
    push_color_space(L, *cs);
  else
    lua_pushnil(L);
  return 1;
}

int Sprite_get_spec(lua_State* L)
{
  const auto sprite = get_docobj<Sprite>(L, 1);
  push_obj(L, sprite->spec());
  return 1;
}

int Sprite_get_selection(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  push_sprite_selection(L, sprite);
  return 1;
}

int Sprite_get_frames(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  push_sprite_frames(L, sprite);
  return 1;
}

int Sprite_get_palettes(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  push_sprite_palettes(L, sprite);
  return 1;
}

int Sprite_get_layers(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  push_sprite_layers(L, sprite);
  return 1;
}

int Sprite_get_cels(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  push_cels(L, sprite);
  return 1;
}

int Sprite_get_tags(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  push_sprite_tags(L, sprite);
  return 1;
}

int Sprite_get_slices(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  push_sprite_slices(L, sprite);
  return 1;
}

int Sprite_get_backgroundLayer(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  doc::Layer* layer = sprite->backgroundLayer();
  if (layer)
    push_docobj(L, layer);
  else
    lua_pushnil(L);
  return 1;
}

int Sprite_get_transparentColor(lua_State* L)
{
  const auto sprite = get_docobj<Sprite>(L, 1);
  lua_pushinteger(L, sprite->transparentColor());
  return 1;
}

int Sprite_set_transparentColor(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  const int index = lua_tointeger(L, 2);
  Tx tx;
  tx(new cmd::SetTransparentColor(sprite, index));
  tx.commit();
  return 0;
}

int Sprite_set_filename(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  const char* fn = lua_tostring(L, 2);
  sprite->document()->setFilename(fn ? std::string(fn): std::string());
  return 0;
}

int Sprite_set_width(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  const int width = lua_tointeger(L, 2);
  Tx tx;
  tx(new cmd::SetSpriteSize(sprite, width, sprite->height()));
  tx.commit();
  return 0;
}

int Sprite_set_height(lua_State* L)
{
  auto sprite = get_docobj<Sprite>(L, 1);
  const int height = lua_tointeger(L, 2);
  Tx tx;
  tx(new cmd::SetSpriteSize(sprite, sprite->width(), height));
  tx.commit();
  return 0;
}

int Sprite_get_bounds(lua_State* L)
{
  const auto sprite = get_docobj<Sprite>(L, 1);
  push_obj<gfx::Rect>(L, sprite->bounds());
  return 1;
}

const luaL_Reg Sprite_methods[] = {
  { "__eq", Sprite_eq },
  { "resize", Sprite_resize },
  { "crop", Sprite_crop },
  { "saveAs", Sprite_saveAs },
  { "saveCopyAs", Sprite_saveCopyAs },
  { "close", Sprite_close },
  { "loadPalette", Sprite_loadPalette },
  { "setPalette", Sprite_setPalette },
  { "assignColorSpace", Sprite_assignColorSpace },
  { "convertColorSpace", Sprite_convertColorSpace },
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
  { "filename", Sprite_get_filename, Sprite_set_filename },
  { "width", Sprite_get_width, Sprite_set_width },
  { "height", Sprite_get_height, Sprite_set_height },
  { "colorMode", Sprite_get_colorMode, nullptr },
  { "colorSpace", Sprite_get_colorSpace, Sprite_assignColorSpace },
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
  { "bounds", Sprite_get_bounds, nullptr },
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
