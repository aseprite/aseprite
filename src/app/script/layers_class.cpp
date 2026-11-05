// Aseprite
// Copyright (C) 2018-present  Igara Studio S.A.
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

namespace app { namespace script {

using namespace doc;

namespace {

struct LayersObj {
  ObjectIds layers;

  LayersObj(Sprite* sprite)
  {
    for (const Layer* layer : sprite->root()->layers())
      layers.push_back(layer->id());
  }
  LayersObj(Layer* group)
  {
    for (const Layer* layer : group->layers())
      layers.push_back(layer->id());
  }
  LayersObj(const ObjectIds& layers) : layers(layers) {}

  LayersObj(const LayersObj&) = delete;
  LayersObj& operator=(const LayersObj&) = delete;
};

LUA_FUNCTION(Layers_gc)
{
  get_obj<LayersObj>(L, 1)->~LayersObj();
  return 0;
}

LUA_FUNCTION(Layers_len)
{
  auto obj = get_obj<LayersObj>(L, 1);
  lua_pushinteger(L, obj->layers.size());
  return 1;
}

LUA_FUNCTION(Layers_index)
{
  auto obj = get_obj<LayersObj>(L, 1);

  // Index by layer name
  if (lua_type(L, 2) == LUA_TSTRING) {
    if (const char* name = lua_tostring(L, 2)) {
      for (ObjectId layerId : obj->layers) {
        Layer* layer = doc::get<Layer>(layerId);
        if (layer && base::utf8_icmp(layer->name(), name) == 0) {
          push_docobj<Layer>(L, layerId);
          return 1;
        }
      }
    }
  }

  const int i = lua_tonumber(L, 2);
  if (i >= 1 && i <= int(obj->layers.size()))
    push_docobj<Layer>(L, obj->layers[i - 1]);
  else
    lua_pushnil(L);
  return 1;
}

const luaL_Reg Layers_methods[] = {
  { "__gc",    Layers_gc    },
  { "__len",   Layers_len   },
  { "__index", Layers_index },
  { nullptr,   nullptr      }
};

} // anonymous namespace

DEF_MTNAME(LayersObj);

LUA_FUNCTION(register_layers_class)
{
  using Layers = LayersObj;
  REG_CLASS(L, Layers);
  return 0;
}

void push_sprite_layers(lua_State* L, Sprite* sprite)
{
  push_new<LayersObj>(L, sprite);
}

void push_group_layers(lua_State* L, Layer* group)
{
  push_new<LayersObj>(L, group);
}

void push_layers(lua_State* L, const ObjectIds& layers)
{
  push_new<LayersObj>(L, layers);
}

}} // namespace app::script
