// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "base/string.h"
#include "doc/layer.h"
#include "doc/object_ids.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

struct LayersObj {
  ObjectIds layers;

  LayersObj(Sprite* sprite) {
    for (const Layer* layer : sprite->root()->layers())
      layers.push_back(layer->id());
  }
  LayersObj(LayerGroup* group) {
    for (const Layer* layer : group->layers())
      layers.push_back(layer->id());
  }
  LayersObj(const ObjectIds& layers)
    : layers(layers) {
  }

  LayersObj(const LayersObj&) = delete;
  LayersObj& operator=(const LayersObj&) = delete;
};

int Layers_gc(lua_State* L)
{
  get_obj<LayersObj>(L, 1)->~LayersObj();
  return 0;
}

int Layers_len(lua_State* L)
{
  auto obj = get_obj<LayersObj>(L, 1);
  lua_pushinteger(L, obj->layers.size());
  return 1;
}

int Layers_index(lua_State* L)
{
  auto obj = get_obj<LayersObj>(L, 1);

  // Index by layer name
  if (lua_isstring(L, 2)) {
    if (const char* name = lua_tostring(L, 2)) {
      for (ObjectId layerId : obj->layers) {
        Layer* layer = doc::get<Layer>(layerId);
        if (layer &&
            base::utf8_icmp(layer->name(), name) == 0) {
          push_docobj<Layer>(L, layerId);
          return 1;
        }
      }
    }
  }

  const int i = lua_tonumber(L, 2);
  if (i >= 1 && i <= int(obj->layers.size()))
    push_docobj<Layer>(L, obj->layers[i-1]);
  else
    lua_pushnil(L);
  return 1;
}

const luaL_Reg Layers_methods[] = {
  { "__gc", Layers_gc },
  { "__len", Layers_len },
  { "__index", Layers_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(LayersObj);

void register_layers_class(lua_State* L)
{
  using Layers = LayersObj;
  REG_CLASS(L, Layers);
}

void push_sprite_layers(lua_State* L, Sprite* sprite)
{
  push_new<LayersObj>(L, sprite);
}

void push_group_layers(lua_State* L, LayerGroup* group)
{
  push_new<LayersObj>(L, group);
}

void push_layers(lua_State* L, const ObjectIds& layers)
{
  push_new<LayersObj>(L, layers);
}

} // namespace script
} // namespace app
