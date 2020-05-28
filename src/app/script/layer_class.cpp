// Aseprite
// Copyright (C) 2018-2019  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_layer_blend_mode.h"
#include "app/cmd/set_layer_name.h"
#include "app/cmd/set_layer_opacity.h"
#include "app/doc.h"
#include "app/doc_api.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/userdata.h"
#include "app/tx.h"
#include "base/clamp.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

int Layer_eq(lua_State* L)
{
  const auto a = get_docobj<Layer>(L, 1);
  const auto b = get_docobj<Layer>(L, 2);
  lua_pushboolean(L, a->id() == b->id());
  return 1;
}

int Layer_cel(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  auto cel = layer->cel(get_frame_number_from_arg(L, 2));
  if (cel)
    push_docobj<Cel>(L, cel);
  else
    lua_pushnil(L);
  return 1;
}

int Layer_get_sprite(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  push_docobj<Sprite>(L, layer->sprite());
  return 1;
}

int Layer_get_parent(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  if (layer->parent() == layer->sprite()->root())
    push_docobj<Sprite>(L, layer->sprite());
  else
    push_docobj<Layer>(L, layer->parent());
  return 1;
}

int Layer_get_layers(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  if (layer->isGroup())
    push_group_layers(L, static_cast<LayerGroup*>(layer));
  else
    lua_pushnil(L);
  return 1;
}

int Layer_get_stackIndex(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  const auto& layers = layer->parent()->layers();
  auto it = std::find(layers.begin(), layers.end(), layer);
  ASSERT(it != layers.end());
  if (it != layers.end())
    lua_pushinteger(L, it - layers.begin() + 1);
  else
    lua_pushnil(L);
  return 1;
}

int Layer_get_previous(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  if (auto previous = layer->getPrevious())
    push_docobj<Layer>(L, previous);
  else
    lua_pushnil(L);
  return 1;
}

int Layer_get_next(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  if (auto next = layer->getNext())
    push_docobj<Layer>(L, next);
  else
    lua_pushnil(L);
  return 1;
}

int Layer_get_name(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushstring(L, layer->name().c_str());
  return 1;
}

int Layer_get_opacity(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  if (layer->isImage()) {
    lua_pushinteger(L, static_cast<LayerImage*>(layer)->opacity());
    return 1;
  }
  else
    return 0;
}

int Layer_get_blendMode(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  if (layer->isImage()) {
    lua_pushinteger(L, (int)static_cast<LayerImage*>(layer)->blendMode());
    return 1;
  }
  else
    return 0;
}

int Layer_get_isImage(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushboolean(L, layer->isImage());
  return 1;
}

int Layer_get_isGroup(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushboolean(L, layer->isGroup());
  return 1;
}

int Layer_get_isTransparent(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushboolean(L, layer->isTransparent());
  return 1;
}

int Layer_get_isBackground(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushboolean(L, layer->isBackground());
  return 1;
}

int Layer_get_isEditable(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushboolean(L, layer->isEditable());
  return 1;
}

int Layer_get_isVisible(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushboolean(L, layer->isVisible());
  return 1;
}

int Layer_get_isContinuous(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushboolean(L, layer->isContinuous());
  return 1;
}

int Layer_get_isCollapsed(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushboolean(L, layer->isCollapsed());
  return 1;
}

int Layer_get_isExpanded(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  lua_pushboolean(L, layer->isExpanded());
  return 1;
}

int Layer_get_cels(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  push_cels(L, layer);
  return 1;
}

int Layer_set_name(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  const char* name = lua_tostring(L, 2);
  if (name) {
    Tx tx;
    tx(new cmd::SetLayerName(layer, name));
    tx.commit();
  }
  return 0;
}

int Layer_set_opacity(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  const int opacity = lua_tointeger(L, 2);
  if (layer->isImage()) {
    Tx tx;
    tx(new cmd::SetLayerOpacity(static_cast<LayerImage*>(layer), opacity));
    tx.commit();
  }
  return 0;
}

int Layer_set_blendMode(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  const int blendMode = lua_tointeger(L, 2);
  if (layer->isImage()) {
    Tx tx;
    tx(new cmd::SetLayerBlendMode(static_cast<LayerImage*>(layer),
                                  (doc::BlendMode)blendMode));
    tx.commit();
  }
  return 0;
}

int Layer_set_stackIndex(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  const auto& layers = layer->parent()->layers();
  int newStackIndex = lua_tointeger(L, 2);
  int stackIndex = 1;
  auto parent = layer->parent();

  // First we need to know this layer stackIndex because we'll use
  auto it = std::find(layers.begin(), layers.end(), layer);
  ASSERT(it != layers.end());
  if (it != layers.end())
    stackIndex = it - layers.begin() + 1;

  Layer* beforeThis = nullptr;
  if (newStackIndex > stackIndex) {
    ++newStackIndex;
  }

  if (newStackIndex-1 < int(parent->layers().size())) {
    beforeThis = parent->layers()[base::clamp(newStackIndex-1, 0, (int)parent->layers().size())];
  }
  else {
    beforeThis = nullptr;
  }

  // Do nothing
  if (beforeThis == layer)
    return 0;

  Doc* doc = static_cast<Doc*>(layer->sprite()->document());
  Tx tx;
  DocApi(doc, tx).restackLayerBefore(layer, parent, beforeThis);
  tx.commit();
  return 0;
}

int Layer_set_isEditable(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  layer->setEditable(lua_toboolean(L, 2));
  return 0;
}

int Layer_set_isVisible(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  layer->setVisible(lua_toboolean(L, 2));
  return 0;
}

int Layer_set_isContinuous(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  layer->setContinuous(lua_toboolean(L, 2));
  return 0;
}

int Layer_set_isCollapsed(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  layer->setCollapsed(lua_toboolean(L, 2));
  return 0;
}

int Layer_set_isExpanded(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  layer->setCollapsed(!lua_toboolean(L, 2));
  return 0;
}

int Layer_set_parent(lua_State* L)
{
  auto layer = get_docobj<Layer>(L, 1);
  LayerGroup* parent = nullptr;
  if (auto sprite = may_get_docobj<Sprite>(L, 2)) {
    parent = sprite->root();
  }
  else if (auto parentLayer = may_get_docobj<Layer>(L, 2)) {
    if (parentLayer->isGroup())
      parent = static_cast<LayerGroup*>(parentLayer);
    else
      return luaL_error(L, "the given parent is not a layer group or sprite");
  }

  if (!parent)
    return luaL_error(L, "parent cannot be nil");
  else if (parent == layer)
    return luaL_error(L, "the parent of a layer cannot be the layer itself");

  // TODO Why? should we be able to do this? It would require some hard work:
  // 1. convert color modes
  // 2. multiple transactions for both modified sprites?
  if (parent->sprite() != layer->sprite())
    return luaL_error(L, "you cannot move layers between sprites");

  if (parent) {
    Doc* doc = static_cast<Doc*>(layer->sprite()->document());
    Tx tx;
    DocApi(doc, tx).restackLayerAfter(
      layer, parent, parent->lastLayer());
    tx.commit();
  }
  return 0;
}

const luaL_Reg Layer_methods[] = {
  { "__eq", Layer_eq },
  { "cel", Layer_cel },
  { nullptr, nullptr }
};

const Property Layer_properties[] = {
  { "sprite", Layer_get_sprite, nullptr },
  { "parent", Layer_get_parent, Layer_set_parent },
  { "layers", Layer_get_layers, nullptr },
  { "stackIndex", Layer_get_stackIndex, Layer_set_stackIndex },
  { "previous", Layer_get_previous, nullptr },
  { "next", Layer_get_next, nullptr },
  { "name", Layer_get_name, Layer_set_name },
  { "opacity", Layer_get_opacity, Layer_set_opacity },
  { "blendMode", Layer_get_blendMode, Layer_set_blendMode },
  { "isImage", Layer_get_isImage, nullptr },
  { "isGroup", Layer_get_isGroup, nullptr },
  { "isTransparent", Layer_get_isTransparent, nullptr },
  { "isBackground", Layer_get_isBackground, nullptr },
  { "isEditable", Layer_get_isEditable, Layer_set_isEditable },
  { "isVisible", Layer_get_isVisible, Layer_set_isVisible },
  { "isContinuous", Layer_get_isContinuous, Layer_set_isContinuous },
  { "isCollapsed", Layer_get_isCollapsed, Layer_set_isCollapsed },
  { "isExpanded", Layer_get_isExpanded, Layer_set_isExpanded },
  { "cels", Layer_get_cels, nullptr },
  { "color", UserData_get_color<Layer>, UserData_set_color<Layer> },
  { "data", UserData_get_text<Layer>, UserData_set_text<Layer> },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Layer);

void register_layer_class(lua_State* L)
{
  using Layer = doc::Layer;
  REG_CLASS(L, Layer);
  REG_CLASS_PROPERTIES(L, Layer);
}

} // namespace script
} // namespace app
