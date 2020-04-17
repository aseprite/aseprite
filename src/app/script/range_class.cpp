// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/context.h"
#include "app/doc_range.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/site.h"
#include "app/util/range_utils.h"
#include "doc/cel.h"
#include "doc/layer.h"
#include "doc/object_ids.h"
#include "doc/sprite.h"

#include <set>
#include <vector>

namespace app {
namespace script {

namespace {

struct RangeObj { // This is like DocRange but referencing objects with IDs
  DocRange::Type type;
  ObjectId spriteId;
  std::set<ObjectId> layers;
  std::vector<frame_t> frames;
  std::set<ObjectId> cels;
  std::vector<color_t> colors;

  RangeObj(Site& site) {
    updateFromSite(site);
  }
  RangeObj(const RangeObj&) = delete;
  RangeObj& operator=(const RangeObj&) = delete;

  void updateFromSite(const Site& site) {
    if (!site.sprite()) {
      type = DocRange::kNone;
      spriteId = NullId;
      return;
    }

    const DocRange& range = site.range();

    spriteId = site.sprite()->id();
    type = range.type();

    layers.clear();
    frames.clear();
    cels.clear();
    colors.clear();

    if (range.enabled()) {
      for (const Layer* layer : range.selectedLayers())
        layers.insert(layer->id());
      for (const frame_t frame : range.selectedFrames())
        frames.push_back(frame);

      // TODO improve this, in the best case we should defer layers,
      // frames, and cels vectors when the properties are accessed, but
      // it might not be possible because we have to save the IDs of the
      // objects (and we cannot store the DocRange because it contains
      // pointers instead of IDs).
      for (const Cel* cel : get_cels(site.sprite(), range))
        cels.insert(cel->id());
    }
    else {
      // Put the active frame/layer/cel information in the range
      frames.push_back(site.frame());
      if (site.layer()) layers.insert(site.layer()->id());
      if (site.cel()) cels.insert(site.cel()->id());
    }

    if (site.selectedColors().picks() > 0)
      colors = site.selectedColors().toVectorOfIndexes();
  }

  Sprite* sprite(lua_State* L) { return check_docobj(L, doc::get<Sprite>(spriteId)); }

  bool contains(const Layer* layer) const {
    return layers.find(layer->id()) != layers.end();
  }
  bool contains(const frame_t frame) const {
    return std::find(frames.begin(), frames.end(), frame) != frames.end();
  }
  bool contains(const Cel* cel) const {
    return cels.find(cel->id()) != cels.end();
  }
  bool containsColor(const color_t color) const {
    return (std::find(colors.begin(), colors.end(), color) != colors.end());
  }
};

int Range_gc(lua_State* L)
{
  get_obj<RangeObj>(L, 1)->~RangeObj();
  return 0;
}

int Range_get_sprite(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  push_docobj<Sprite>(L, obj->spriteId);
  return 1;
}

int Range_get_type(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  lua_pushinteger(L, int(obj->type));
  return 1;
}

int Range_contains(lua_State* L)
{
  bool result = false;
  auto obj = get_obj<RangeObj>(L, 1);
  if (Layer* layer = may_get_docobj<Layer>(L, 2)) {
    result = obj->contains(layer);
  }
  else if (Cel* cel = may_get_docobj<Cel>(L, 2)) {
    result = obj->contains(cel);
  }
  else {
    frame_t frame = get_frame_number_from_arg(L, 2);
    result = obj->contains(frame);
  }
  lua_pushboolean(L, result);
  return 1;
}

int Range_containsColor(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  color_t color = lua_tointeger(L, 2);
  lua_pushboolean(L, obj->containsColor(color));
  return 1;
}

int Range_clear(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  auto ctx = App::instance()->context();

  // Set an empty range
  DocRange range;
  ctx->setRange(range);

  // Set empty palette picks
  doc::PalettePicks picks;
  ctx->setSelectedColors(picks);

  obj->updateFromSite(ctx->activeSite());
  return 0;
}

int Range_get_isEmpty(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  lua_pushboolean(L, obj->type == DocRange::kNone);
  return 1;
}

int Range_get_layers(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  ObjectIds layers(obj->layers.size());
  int i = 0;
  for (auto id : obj->layers)
    layers[i++] = id;
  push_layers(L, layers);
  return 1;
}

int Range_get_frames(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  push_sprite_frames(L, obj->sprite(L), obj->frames);
  return 1;
}

int Range_get_cels(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  ObjectIds cels(obj->cels.size());
  int i = 0;
  for (auto id : obj->cels)
    cels[i++] = id;
  push_cels(L, cels);
  return 1;
}

int Range_get_images(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  std::set<ObjectId> imagesSet;
  for (auto celId : obj->cels) {
    Cel* cel = check_docobj(L, doc::get<Cel>(celId));
    imagesSet.insert(cel->image()->id());
  }
  ObjectIds images;
  for (auto imageId : imagesSet)
    images.push_back(imageId);
  push_images(L, images);
  return 1;
}

int Range_get_editableImages(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  std::set<ObjectId> imagesSet;
  for (auto celId : obj->cels) {
    Cel* cel = check_docobj(L, doc::get<Cel>(celId));
    if (cel->layer()->isEditable())
      imagesSet.insert(cel->image()->id());
  }
  ObjectIds images;
  for (auto imageId : imagesSet)
    images.push_back(imageId);
  push_images(L, images);
  return 1;
}

int Range_get_colors(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  lua_newtable(L);
  int j = 1;
  for (color_t i : obj->colors) {
    lua_pushinteger(L, i);
    lua_rawseti(L, -2, j++);
  }
  return 1;
}

int Range_set_layers(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  app::Context* ctx = App::instance()->context();
  DocRange range = ctx->activeSite().range();

  doc::SelectedLayers layers;
  if (lua_istable(L, 2)) {
    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
      if (Layer* layer = may_get_docobj<Layer>(L, -1))
        layers.insert(layer);
      lua_pop(L, 1);
    }
  }
  range.setSelectedLayers(layers);

  ctx->setRange(range);
  obj->updateFromSite(ctx->activeSite());
  return 0;
}

int Range_set_frames(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  app::Context* ctx = App::instance()->context();
  DocRange range = ctx->activeSite().range();

  doc::SelectedFrames frames;
  if (lua_istable(L, 2)) {
    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
      doc::frame_t f = get_frame_number_from_arg(L, -1);
      frames.insert(f);
      lua_pop(L, 1);
    }
  }
  range.setSelectedFrames(frames);

  ctx->setRange(range);
  obj->updateFromSite(ctx->activeSite());
  return 0;
}

int Range_set_colors(lua_State* L)
{
  auto obj = get_obj<RangeObj>(L, 1);
  app::Context* ctx = App::instance()->context();
  doc::PalettePicks picks;
  if (lua_istable(L, 2)) {
    lua_pushnil(L);
    while (lua_next(L, 2) != 0) {
      int i = lua_tointeger(L, -1);
      if (i >= picks.size())
        picks.resize(i+1);
      picks[i] = true;
      lua_pop(L, 1);
    }
  }

  ctx->setSelectedColors(picks);
  obj->updateFromSite(ctx->activeSite());
  return 0;
}

const luaL_Reg Range_methods[] = {
  { "__gc", Range_gc },
  { "contains", Range_contains },
  { "containsColor", Range_containsColor },
  { "clear", Range_clear },
  { nullptr, nullptr }
};

const Property Range_properties[] = {
  { "sprite", Range_get_sprite, nullptr },
  { "type", Range_get_type, nullptr },
  { "isEmpty", Range_get_isEmpty, nullptr },
  { "layers", Range_get_layers, Range_set_layers },
  { "frames", Range_get_frames, Range_set_frames },
  { "cels", Range_get_cels, nullptr },
  { "images", Range_get_images, nullptr },
  { "editableImages", Range_get_editableImages, nullptr },
  { "colors", Range_get_colors, Range_set_colors },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(RangeObj);

void register_range_class(lua_State* L)
{
  using Range = RangeObj;
  REG_CLASS(L, Range);
  REG_CLASS_PROPERTIES(L, Range);
}

void push_doc_range(lua_State* L, Site& site)
{
  push_new<RangeObj>(L, site);
}

} // namespace script
} // namespace app
