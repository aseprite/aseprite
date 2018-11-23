// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/docobj.h"
#include "app/script/luacpp.h"
#include "doc/cel.h"
#include "doc/cels_range.h"
#include "doc/layer.h"
#include "doc/object_ids.h"
#include "doc/sprite.h"

#include <algorithm>
#include <iterator>

namespace app {
namespace script {

using namespace doc;

namespace {

struct CelsObj {
  ObjectIds cels;
  CelsObj(CelsRange& range) {
    for (const Cel* cel : range)
      cels.push_back(cel->id());
  }
  CelsObj(CelList& list) {
    for (Cel* cel : list)
      cels.push_back(cel->id());
  }
  CelsObj(const ObjectIds& cels)
    : cels(cels) {
  }
  CelsObj(const CelsObj&) = delete;
  CelsObj& operator=(const CelsObj&) = delete;
};

int Cels_gc(lua_State* L)
{
  get_obj<CelsObj>(L, 1)->~CelsObj();
  return 0;
}

int Cels_len(lua_State* L)
{
  auto obj = get_obj<CelsObj>(L, 1);
  lua_pushinteger(L, obj->cels.size());
  return 1;
}

int Cels_index(lua_State* L)
{
  auto obj = get_obj<CelsObj>(L, 1);
  const int i = lua_tointeger(L, 2);
  if (i >= 1 && i <= obj->cels.size())
    push_docobj<Cel>(L, obj->cels[i-1]);
  else
    lua_pushnil(L);
  return 1;
}

const luaL_Reg Cels_methods[] = {
  { "__gc", Cels_gc },
  { "__len", Cels_len },
  { "__index", Cels_index },
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(CelsObj);

void register_cels_class(lua_State* L)
{
  using Cels = CelsObj;
  REG_CLASS(L, Cels);
}

void push_cels(lua_State* L, Sprite* sprite)
{
  CelsRange cels = sprite->cels();
  push_new<CelsObj>(L, cels);
}

void push_cels(lua_State* L, Layer* layer)
{
  CelList cels;
  if (layer->isImage())
    static_cast<LayerImage*>(layer)->getCels(cels);
  push_new<CelsObj>(L, cels);
}

void push_cels(lua_State* L, const ObjectIds& cels)
{
  push_new<CelsObj>(L, cels);
}

} // namespace script
} // namespace app
