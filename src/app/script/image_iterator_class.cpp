// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/copy_region.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/tx.h"
#include "doc/algorithm/shrink_bounds.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_impl.h"
#include "doc/image_ref.h"
#include "doc/primitives.h"

namespace app {
namespace script {

namespace {

template<typename ImageTraits>
struct ImageIteratorObj {
  typename doc::LockImageBits<ImageTraits> bits;
  typename doc::LockImageBits<ImageTraits>::iterator begin, next, end;
  ImageIteratorObj(const doc::Image* image, const gfx::Rect& bounds)
    : bits(image, bounds),
      begin(bits.begin()),
      next(begin),
      end(bits.end()) {
  }
  ImageIteratorObj(const ImageIteratorObj&) = delete;
  ImageIteratorObj& operator=(const ImageIteratorObj&) = delete;
};

using RgbImageIterator = ImageIteratorObj<RgbTraits>;
using GrayscaleImageIterator = ImageIteratorObj<GrayscaleTraits>;
using IndexedImageIterator = ImageIteratorObj<IndexedTraits>;

template<typename ImageTraits>
int ImageIterator_gc(lua_State* L)
{
  get_obj<ImageIteratorObj<ImageTraits>>(L, 1)->~ImageIteratorObj<ImageTraits>();
  return 0;
}

template<typename ImageTraits>
int ImageIterator_call(lua_State* L)
{
  auto obj = get_obj<ImageIteratorObj<ImageTraits>>(L, 1);
  // Get value
  if (lua_isnone(L, 2)) {
    lua_pushinteger(L, *obj->begin);
    return 1;
  }
  // Set value
  else {
    *obj->begin = lua_tointeger(L, 2);
    return 1;
  }
}

template<typename ImageTraits>
int ImageIterator_index(lua_State* L)
{
  auto obj = get_obj<ImageIteratorObj<ImageTraits>>(L, 1);
  const char* field = lua_tostring(L, 2);
  if (field) {
    if (field[0] == 'x' && field[1] == 0) {
      lua_pushinteger(L, obj->begin.x());
      return 1;
    }
    else if (field[0] == 'y' && field[1] == 0) {
      lua_pushinteger(L, obj->begin.y());
      return 1;
    }
  }
  return 0;
}

#define DEFINE_METHODS(Prefix)                          \
  const luaL_Reg Prefix##ImageIterator_methods[] = {    \
    { "__index", ImageIterator_index<Prefix##Traits> }, \
    { "__call", ImageIterator_call<Prefix##Traits> },   \
    { "__gc", ImageIterator_gc<Prefix##Traits> },       \
    { nullptr, nullptr }                                \
  }

DEFINE_METHODS(Rgb);
DEFINE_METHODS(Grayscale);
DEFINE_METHODS(Indexed);

} // anonymous namespace

DEF_MTNAME(ImageIteratorObj<RgbTraits>);
DEF_MTNAME(ImageIteratorObj<GrayscaleTraits>);
DEF_MTNAME(ImageIteratorObj<IndexedTraits>);

void register_image_iterator_class(lua_State* L)
{
  REG_CLASS(L, RgbImageIterator);
  REG_CLASS(L, GrayscaleImageIterator);
  REG_CLASS(L, IndexedImageIterator);
}

template<typename ImageTrais>
static int image_iterator_step_closure(lua_State* L)
{
  int idx = lua_upvalueindex(1);
  auto obj = get_obj<ImageIteratorObj<ImageTrais>>(L, idx);
  obj->begin = obj->next;
  if (obj->begin != obj->end) {
    ++obj->next;
    lua_pushvalue(L, idx);
  }
  else {
    lua_pushnil(L);
  }
  return 1;
}

// Used to when an area outside the image canvas is specified, so
// there is pixel to be iterated.
static int image_iterator_do_nothing(lua_State* L)
{
  lua_pushnil(L);
  return 1;
}

int push_image_iterator_function(lua_State* L, const doc::Image* image, int extraArgIndex)
{
  gfx::Rect bounds = image->bounds();

  if (!lua_isnone(L, extraArgIndex)) {
    auto specificBounds = convert_args_into_rect(L, extraArgIndex);
    if (!specificBounds.isEmpty())
      bounds &= specificBounds;
  }

  if (bounds.isEmpty()) {
    lua_pushcclosure(L, image_iterator_do_nothing, 0);
    return 1;
  }

  switch (image->pixelFormat()) {
    case IMAGE_RGB:
      push_new<RgbImageIterator>(L, image, bounds);
      lua_pushcclosure(L, image_iterator_step_closure<doc::RgbTraits>, 1);
      return 1;
    case IMAGE_GRAYSCALE:
      push_new<GrayscaleImageIterator>(L, image, bounds);
      lua_pushcclosure(L, image_iterator_step_closure<doc::GrayscaleTraits>, 1);
      return 1;
    case IMAGE_INDEXED:
      push_new<IndexedImageIterator>(L, image, bounds);
      lua_pushcclosure(L, image_iterator_step_closure<doc::IndexedTraits>, 1);
      return 1;
    default:
      return 0;
  }
}

} // namespace script
} // namespace app
