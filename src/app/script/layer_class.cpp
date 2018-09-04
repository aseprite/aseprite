// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_layer_name.h"
#include "app/cmd/set_layer_opacity.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/tx.h"
#include "doc/layer.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

int Layer_get_sprite(lua_State* L)
{
  auto layer = get_ptr<Layer>(L, 1);
  push_ptr<Sprite>(L, layer->sprite());
  return 1;
}

int Layer_get_name(lua_State* L)
{
  auto layer = get_ptr<Layer>(L, 1);
  lua_pushstring(L, layer->name().c_str());
  return 1;
}

int Layer_get_opacity(lua_State* L)
{
  auto layer = get_ptr<Layer>(L, 1);
  if (layer->isImage()) {
    lua_pushinteger(L, static_cast<LayerImage*>(layer)->opacity());
    return 1;
  }
  else
    return 0;
}

int Layer_get_userData(lua_State* L)
{
  auto layer = get_ptr<Layer>(L, 1);
  push_userdata(L, layer);
  return 1;
}

int Layer_set_name(lua_State* L)
{
  auto layer = get_ptr<Layer>(L, 1);
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
  auto layer = get_ptr<Layer>(L, 1);
  const int opacity = lua_tointeger(L, 2);
  if (layer->isImage()) {
    Tx tx;
    tx(new cmd::SetLayerOpacity(static_cast<LayerImage*>(layer), opacity));
    tx.commit();
  }
  return 0;
}

const luaL_Reg Layer_methods[] = {
  { nullptr, nullptr }
};

const Property Layer_properties[] = {
  { "sprite", Layer_get_sprite, nullptr },
  { "name", Layer_get_name, Layer_set_name },
  { "opacity", Layer_get_opacity, Layer_set_opacity },
  { "userData", Layer_get_userData, nullptr },
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
