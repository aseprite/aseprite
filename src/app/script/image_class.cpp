// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2015-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/copy_rect.h"
#include "app/cmd/copy_region.h"
#include "app/commands/new_params.h" // Used for enum <-> Lua conversions
#include "app/context.h"
#include "app/doc.h"
#include "app/file/file.h"
#include "app/script/blend_mode.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/security.h"
#include "app/site.h"
#include "app/tx.h"
#include "app/util/autocrop.h"
#include "app/util/resize_image.h"
#include "base/fs.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "render/render.h"

#include <algorithm>
#include <cstring>
#include <memory>

namespace app {
namespace script {

namespace {

static ImageBufferPtr buf; // TODO non-thread safe

struct ImageObj {
  doc::ObjectId imageId = 0;
  doc::ObjectId celId = 0;
  doc::ObjectId tilesetId = 0;
  ImageObj(doc::Image* image)
    : imageId(image->id()) {
  }
  ImageObj(doc::Cel* cel)
    : imageId(cel->image()->id())
    , celId(cel->id()) {
  }
  ImageObj(doc::Tileset* tileset, doc::Image* image)
    : imageId(image->id())
    , tilesetId(tileset->id()) {
  }
  ImageObj(const ImageObj&) = delete;
  ImageObj& operator=(const ImageObj&) = delete;

  ~ImageObj() {
    ASSERT(!imageId);
  }

  void gc(lua_State* L) {
    if (!celId && !tilesetId)
      delete this->image(L);
    imageId = 0;
  }

  doc::Image* image(lua_State* L) {
    return check_docobj(L, doc::get<doc::Image>(imageId));
  }

  doc::Cel* cel(lua_State* L) {
    if (celId)
      return check_docobj(L, doc::get<doc::Cel>(celId));
    else
      return nullptr;
  }
};

void render_sprite(Image* dst,
                   const Sprite* sprite,
                   const frame_t frame,
                   const int x, const int y)
{
  render::Render render;
  render.setNewBlend(true);
  render.renderSprite(
    dst, sprite, frame,
    gfx::Clip(x, y,
              0, 0,
              sprite->width(),
              sprite->height()));
}

int Image_clone(lua_State* L);

int Image_new(lua_State* L)
{
  doc::Image* image = nullptr;
  doc::ImageSpec spec(doc::ColorMode::RGB, 1, 1, 0);
  if (auto spec2 = may_get_obj<doc::ImageSpec>(L, 1)) {
    spec = *spec2;
  }
  else if (auto imgObj = may_get_obj<ImageObj>(L, 1)) {
    // Copy a region of the image
    if (auto rc = may_get_obj<gfx::Rect>(L, 2)) {
      doc::Image* crop = nullptr;
      try {
        auto docImg = imgObj->image(L);
        crop = doc::crop_image(docImg, *rc, docImg->maskColor());
      }
      catch (const std::invalid_argument&) {
        // Do nothing (will return nil)
      }
      if (crop) {
        push_new<ImageObj>(L, crop);
        return 1;
      }
      else {
        return 0;
      }
    }
    // Copy the whole image
    else {
      return Image_clone(L);
    }
  }
  else if (auto spr = may_get_docobj<doc::Sprite>(L, 1)) {
    image = doc::Image::create(spr->spec());
    if (!image)
      return 0;

    render_sprite(image, spr, 0, 0, 0);
  }
  else if (lua_istable(L, 1)) {
    // Image{ fromFile }
    int type = lua_getfield(L, 1, "fromFile");
    if (type != LUA_TNIL) {
      if (const char* fromFile = lua_tostring(L, -1)) {
        std::string fn = fromFile;
        lua_pop(L, 1);
        return load_sprite_from_file(
          L, fn.c_str(),
          LoadSpriteFromFileParam::OneFrameAsImage);
      }
    }
    lua_pop(L, 1);

    // In case that there is no "fromFile" field
    if (type == LUA_TNIL) {
      // Image{ width, height, colorMode }
      lua_getfield(L, 1, "width");
      lua_getfield(L, 1, "height");
      spec.setWidth(lua_tointeger(L, -2));
      spec.setHeight(lua_tointeger(L, -1));
      lua_pop(L, 2);

      type = lua_getfield(L, 1, "colorMode");
      if (type != LUA_TNIL)
        spec.setColorMode((doc::ColorMode)lua_tointeger(L, -1));
      lua_pop(L, 1);
    }
  }
  else {
    const int w = lua_tointeger(L, 1);
    const int h = lua_tointeger(L, 2);
    const int colorMode = (lua_isnone(L, 3) ? doc::IMAGE_RGB:
                                              lua_tointeger(L, 3));
    spec.setWidth(w);
    spec.setHeight(h);
    spec.setColorMode((doc::ColorMode)colorMode);
  }
  if (!image) {
    if (spec.width() < 1) spec.setWidth(1);
    if (spec.height() < 1) spec.setHeight(1);
    image = doc::Image::create(spec);
    if (!image) {
      // Invalid spec (e.g. width=0, height=0, etc.)
      return 0;
    }
    doc::clear_image(image, spec.maskColor());
  }
  push_new<ImageObj>(L, image);
  return 1;
}

int Image_clone(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  doc::Image* cloned = doc::Image::createCopy(obj->image(L));
  push_new<ImageObj>(L, cloned);
  return 1;
}

int Image_gc(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  obj->gc(L);
  obj->~ImageObj();
  return 0;
}

int Image_eq(lua_State* L)
{
  const auto a = get_obj<ImageObj>(L, 1);
  const auto b = get_obj<ImageObj>(L, 2);
  lua_pushboolean(L, a->imageId == b->imageId);
  return 1;
}

int Image_clear(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  auto img = obj->image(L);
  doc::color_t color;
  gfx::Rect rc;
  int i = 2;

  if (auto rcPtr = may_get_obj<gfx::Rect>(L, i)) {
    rc = *rcPtr;
    ++i;
  }
  else {
    rc = img->bounds();         // Clear the whole image
  }

  if (lua_isnone(L, i))
    color = img->maskColor();
  else if (lua_isinteger(L, i))
    color = lua_tointeger(L, i);
  else
    color = convert_args_into_pixel_color(L, i, img->pixelFormat());

  doc::fill_rect(img, rc, color); // Clips the rectangle to the image bounds
  return 0;
}

int Image_drawPixel(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  auto img = obj->image(L);
  const int x = lua_tointeger(L, 2);
  const int y = lua_tointeger(L, 3);
  doc::color_t color;
  if (lua_isinteger(L, 4))
    color = lua_tointeger(L, 4);
  else
    color = convert_args_into_pixel_color(L, 4, img->pixelFormat());
  doc::put_pixel(img, x, y, color);
  return 0;
}

int Image_drawImage(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  auto sprite = get_obj<ImageObj>(L, 2);
  gfx::Point pos = convert_args_into_point(L, 3);

  // Arguments index fix to support the following cases:
  //   - Image:drawImage(image, x, y, opacity, blendMode)
  //   - Image:drawImage(image, Point(x, y), opacity, blendMode)
  //   - Image:drawImage(image, {x, y}, opacity, blendMode)
  //   - Image:drawImage(image, {x=x1, y=y1}, opacity, blendMode)
  //
  // TODO create a similar convert_args_into_point() function so we
  //      can get the argsFix/modified index directly from there to
  //      read the next argument
  int argsFix = 0;
  if (lua_isinteger(L, 3))
    argsFix = 1;

  int opacity = 255;
  if (lua_isinteger(L, 4 + argsFix))
    opacity = std::clamp(int(lua_tointeger(L, 4 + argsFix)), 0, 255);

  doc::BlendMode blendMode = doc::BlendMode::NORMAL;
  if (lua_isinteger(L, 5 + argsFix)) {
    blendMode = base::convert_to<doc::BlendMode>(
                  app::script::BlendMode(lua_tointeger(L, 5 + argsFix)));
  }

  Image* dst = obj->image(L);
  const Image* src = sprite->image(L);

  // If the destination image is not related to a sprite, we just draw
  // the source image without undo information.
  if (obj->cel(L) == nullptr) {
    doc::blend_image(dst, src,
                     pos.x, pos.y,
                     opacity, blendMode);
  }
  else {
    gfx::Rect bounds(0, 0, src->size().w, src->size().h);
    buf.reset(new doc::ImageBuffer);
    ImageRef tmp_src(
      doc::crop_image(dst,
                      gfx::Rect(pos.x, pos.y, src->size().w, src->size().h),
                      0, buf));
    doc::blend_image(tmp_src.get(), src, 0, 0, opacity, blendMode);
    // TODO Use something similar to doc::algorithm::shrink_bounds2()
    //      but we need something that does the render and compares
    //      the minimal modified area.
    Tx tx;
    tx(new cmd::CopyRegion(
         dst, tmp_src.get(), gfx::Region(bounds),
         gfx::Point(pos.x + bounds.x, pos.y + bounds.y)));
    tx.commit();
  }
  return 0;
}

int Image_drawSprite(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  const auto sprite = get_docobj<Sprite>(L, 2);
  doc::frame_t frame = get_frame_number_from_arg(L, 3);
  gfx::Point pos = convert_args_into_point(L, 4);
  doc::Image* dst = obj->image(L);

  ASSERT(dst);
  ASSERT(sprite);

  // If the destination image is not related to a sprite, we just draw
  // the source image without undo information.
  if (obj->cel(L) == nullptr) {
    render_sprite(dst, sprite, frame, pos.x, pos.y);
  }
  else {
    Tx tx;

    ImageRef tmp(Image::createCopy(dst));
    render_sprite(tmp.get(), sprite, frame, pos.x, pos.y);

    int x1, y1, x2, y2;
    if (get_shrink_rect2(&x1, &y1, &x2, &y2, dst, tmp.get())) {
      tx(new cmd::CopyRect(
           dst, tmp.get(),
           gfx::Clip(x1, y1, x1, y1, x2-x1+1, y2-y1+1)));
    }

    tx.commit();
  }
  return 0;
}

int Image_pixels(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  push_image_iterator_function(L, obj->image(L), 2);
  return 1;
}

int Image_getPixel(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  const int x = lua_tointeger(L, 2);
  const int y = lua_tointeger(L, 3);
  const doc::color_t color = doc::get_pixel(obj->image(L), x, y);
  lua_pushinteger(L, color);
  return 1;
}

int Image_isEqual(lua_State* L)
{
  auto objA = get_obj<ImageObj>(L, 1);
  auto objB = get_obj<ImageObj>(L, 2);
  bool res = doc::is_same_image(objA->image(L),
                                objB->image(L));
  lua_pushboolean(L, res);
  return 1;
}

int Image_isEmpty(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  auto img = obj->image(L);
  bool res = doc::is_empty_image(img);
  lua_pushboolean(L, res);
  return 1;
}

int Image_isPlain(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  auto img = obj->image(L);
  doc::color_t color;
  if (lua_isnone(L, 2))
    color = img->maskColor();
  else if (lua_isinteger(L, 2))
    color = lua_tointeger(L, 2);
  else
    color = convert_args_into_pixel_color(L, 2, img->pixelFormat());

  bool res = doc::is_plain_image(img, color);
  lua_pushboolean(L, res);
  return 1;
}

int Image_saveAs(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  Image* img = obj->image(L);
  Cel* cel = obj->cel(L);
  Palette* pal = (cel ? cel->sprite()->palette(cel->frame()): nullptr);
  std::string fn;
  bool result = false;

  if (lua_istable(L, 2)) {
    // Image:saveAs{ filename }
    int type = lua_getfield(L, 2, "filename");
    if (type != LUA_TNIL) {
      if (const char* fn0 = lua_tostring(L, -1))
        fn = fn0;
    }
    lua_pop(L, 1);

    // Image:saveAs{ palette }
    lua_getfield(L, 2, "palette");
    if (type != LUA_TNIL) {
      if (auto pal0 = get_palette_from_arg(L, -1))
        pal = pal0;
    }
    lua_pop(L, 1);
  }
  else {
    // Image:saveAs(filename)
    const char* fn0 = luaL_checkstring(L, 2);
    if (fn0)
      fn = fn0;
  }

  if (fn.empty())
    return luaL_error(L, "missing filename in Image:saveAs()");

  std::string absFn = base::get_absolute_path(fn);
  if (!ask_access(L, absFn.c_str(), FileAccessMode::Write, ResourceType::File))
    return luaL_error(L, "script doesn't have access to write file %s",
                      absFn.c_str());

  std::unique_ptr<Sprite> sprite(Sprite::MakeStdSprite(img->spec(), 256));

  std::vector<ImageRef> oneImage;
  sprite->getImages(oneImage);
  ASSERT(oneImage.size() == 1);
  if (!oneImage.empty())
    copy_image(oneImage.front().get(), img);

  if (pal)
    sprite->setPalette(pal, false);

  std::unique_ptr<Doc> doc(new Doc(sprite.get()));
  sprite.release();
  doc->setFilename(absFn);

  app::Context* ctx = App::instance()->context();
  result = (save_document(ctx, doc.get()) >= 0);

  lua_pushboolean(L, result);
  return 1;
}

int Image_resize(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  doc::Image* img = obj->image(L);
  Cel* cel = obj->cel(L);
  ASSERT(img);
  gfx::Size newSize = img->size();
  auto method = doc::algorithm::ResizeMethod::RESIZE_METHOD_NEAREST_NEIGHBOR;
  gfx::Point pivot(0, 0);

  if (lua_istable(L, 2)) {
    // Image:resize{ (size | width, height),
    //               method [, pivot] }

    int type = lua_getfield(L, 2, "size");
    if (VALID_LUATYPE(type)) {
      newSize = convert_args_into_size(L, -1);
      lua_pop(L, 1);
    }
    else {
      lua_pop(L, 1);

      type = lua_getfield(L, 2, "width");
      if (VALID_LUATYPE(type))
        newSize.w = lua_tointeger(L, -1);
      lua_pop(L, 1);

      type = lua_getfield(L, 2, "height");
      if (VALID_LUATYPE(type))
        newSize.h = lua_tointeger(L, -1);
      lua_pop(L, 1);
    }

    type = lua_getfield(L, 2, "method");
    if (VALID_LUATYPE(type))  {
      // TODO improve these lua <-> enum conversions, a lot of useless
      //      work is done to create this dummy NewParams, etc.
      NewParams dummyParams;
      Param<doc::algorithm::ResizeMethod> param(&dummyParams, method, "method");
      param.fromLua(L, -1);
      method = param();
    }
    lua_pop(L, 1);

    type = lua_getfield(L, 2, "pivot");
    if (VALID_LUATYPE(type))
      pivot = convert_args_into_point(L, -1);
    lua_pop(L, 1);
  }
  else {
    newSize.w = lua_tointeger(L, 2);
    newSize.h = lua_tointeger(L, 3);
  }

  newSize.w = std::max(1, newSize.w);
  newSize.h = std::max(1, newSize.h);

  const gfx::SizeF scale(
    double(newSize.w) / double(img->width()),
    double(newSize.h) / double(img->height()));

  // If the destination image is not related to a sprite, we just draw
  // the source image without undo information.
  if (cel) {
    Tx tx;
    resize_cel_image(tx, cel, scale, method,
                     gfx::PointF(pivot));
    tx.commit();
    obj->imageId = cel->image()->id();
  }
  else {
    Context* ctx = App::instance()->context();
    ASSERT(ctx);
    Site site = ctx->activeSite();
    const doc::Palette* pal = site.palette();
    const doc::RgbMap* rgbmap = site.rgbMap();

    std::unique_ptr<doc::Image> newImg(
      resize_image(img, scale, method,
                   pal, rgbmap));
    // Delete old image, and we put the same ID of the old image into
    // the new image so this userdata references the resized image.
    delete img;
    newImg->setId(obj->imageId);
    // Release the image from the smart pointer because now it's owned
    // by the ImageObj userdata.
    newImg.release();
  }
  return 0;
}

int Image_shrinkBounds(lua_State* L)
{
  auto obj = get_obj<ImageObj>(L, 1);
  doc::Image* img = obj->image(L);
  doc::color_t refcolor;
  if (lua_isnone(L, 2))
    refcolor = img->maskColor();
  else if (lua_isinteger(L, 2))
    refcolor = lua_tointeger(L, 2);
  else
    refcolor = convert_args_into_pixel_color(L, 2, img->pixelFormat());

  gfx::Rect bounds;
  if (!doc::algorithm::shrink_bounds(img, refcolor, nullptr, bounds)) {
    bounds = gfx::Rect();
  }

  push_obj(L, bounds);
  return 1;
}

int Image_get_id(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  lua_pushinteger(L, obj->imageId);
  return 1;
}

int Image_get_version(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  lua_pushinteger(L, obj->image(L)->version());
  return 1;
}

int Image_get_rowStride(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  lua_pushinteger(L, obj->image(L)->getRowStrideSize());
  return 1;
}

int Image_get_bytes(lua_State* L)
{
  const auto img = get_obj<ImageObj>(L, 1)->image(L);
  lua_pushlstring(L, (const char*)img->getPixelAddress(0, 0), img->getRowStrideSize() * img->height());
  return 1;
}

int Image_set_bytes(lua_State* L)
{
  const auto img = get_obj<ImageObj>(L, 1)->image(L);
  size_t bytes_size, bytes_needed = img->getRowStrideSize() * img->height();
  const char* bytes = lua_tolstring(L, 2, &bytes_size);

  if (bytes_size == bytes_needed) {
    std::memcpy(img->getPixelAddress(0, 0), bytes, bytes_size);
  }
  else {
    lua_pushfstring(L, "Data size does not match: given %d, needed %d.", bytes_size, bytes_needed);
    lua_error(L);
  }

  return 0;
}

int Image_get_width(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  lua_pushinteger(L, obj->image(L)->width());
  return 1;
}

int Image_get_height(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  lua_pushinteger(L, obj->image(L)->height());
  return 1;
}

int Image_get_bounds(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  push_obj(L, obj->image(L)->bounds());
  return 1;
}

int Image_get_colorMode(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  lua_pushinteger(L, obj->image(L)->pixelFormat());
  return 1;
}

int Image_get_spec(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  push_obj(L, obj->image(L)->spec());
  return 1;
}

int Image_get_cel(lua_State* L)
{
  const auto obj = get_obj<ImageObj>(L, 1);
  push_docobj<Cel>(L, obj->celId);
  return 1;
}

const luaL_Reg Image_methods[] = {
  { "clone", Image_clone },
  { "clear", Image_clear },
  { "getPixel", Image_getPixel },
  { "drawPixel", Image_drawPixel }, { "putPixel", Image_drawPixel },
  { "drawImage", Image_drawImage }, { "putImage", Image_drawImage }, // TODO putImage is deprecated
  { "drawSprite", Image_drawSprite }, { "putSprite", Image_drawSprite }, // TODO putSprite is deprecated
  { "pixels", Image_pixels },
  { "isEqual", Image_isEqual },
  { "isEmpty", Image_isEmpty },
  { "isPlain", Image_isPlain },
  { "saveAs", Image_saveAs },
  { "resize", Image_resize },
  { "shrinkBounds", Image_shrinkBounds },
  { "__gc", Image_gc },
  { "__eq", Image_eq },
  { nullptr, nullptr }
};

const Property Image_properties[] = {
  { "id", Image_get_id, nullptr },
  { "version", Image_get_version, nullptr },
  { "rowStride", Image_get_rowStride, nullptr },
  { "bytes", Image_get_bytes, Image_set_bytes },
  { "width", Image_get_width, nullptr },
  { "height", Image_get_height, nullptr },
  { "bounds", Image_get_bounds, nullptr },
  { "colorMode", Image_get_colorMode, nullptr },
  { "spec", Image_get_spec, nullptr },
  { "cel", Image_get_cel, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(ImageObj);
DEF_MTNAME_ALIAS(ImageObj, Image);

void register_image_class(lua_State* L)
{
  using Image = ImageObj;
  REG_CLASS(L, Image);
  REG_CLASS_NEW(L, Image);
  REG_CLASS_PROPERTIES(L, Image);
}

void push_cel_image(lua_State* L, doc::Cel* cel)
{
  push_new<ImageObj>(L, cel);
}

void push_image(lua_State* L, doc::Image* image)
{
  push_new<ImageObj>(L, image);
}

void push_tileset_image(lua_State* L, doc::Tileset* tileset, doc::Image* image)
{
  push_new<ImageObj>(L, tileset, image);
}

doc::Image* may_get_image_from_arg(lua_State* L, int index)
{
  auto obj = may_get_obj<ImageObj>(L, index);
  if (obj)
    return obj->image(L);
  else
    return nullptr;
}

doc::Image* get_image_from_arg(lua_State* L, int index)
{
  return get_obj<ImageObj>(L, index)->image(L);
}

doc::Cel* get_image_cel_from_arg(lua_State* L, int index)
{
  return get_obj<ImageObj>(L, index)->cel(L);
}

} // namespace script
} // namespace app
