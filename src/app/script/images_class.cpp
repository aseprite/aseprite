// Aseprite
// Copyright (C) 2018-2021  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "doc/cel.h"
#include "doc/object_ids.h"

namespace app { namespace script {

using namespace doc;

namespace {

struct ImagesObj {
  ObjectIds cels;

  ImagesObj(const ObjectIds& cels) : cels(cels) {}

  ImagesObj(const ImagesObj&) = delete;
  ImagesObj& operator=(const ImagesObj&) = delete;
};

int Images_gc(lua_State* L)
{
  get_obj<ImagesObj>(L, 1)->~ImagesObj();
  return 0;
}

int Images_len(lua_State* L)
{
  auto obj = get_obj<ImagesObj>(L, 1);
  lua_pushinteger(L, obj->cels.size());
  return 1;
}

int Images_index(lua_State* L)
{
  auto obj = get_obj<ImagesObj>(L, 1);
  const int i = lua_tointeger(L, 2);
  if (i >= 1 && i <= obj->cels.size()) {
    if (auto cel = doc::get<doc::Cel>(obj->cels[i - 1])) {
      push_cel_image(L, cel);
      return 1;
    }
  }
  lua_pushnil(L);
  return 1;
}

const luaL_Reg Images_methods[] = {
  { "__gc",    Images_gc    },
  { "__len",   Images_len   },
  { "__index", Images_index },
  { nullptr,   nullptr      }
};

} // anonymous namespace

DEF_MTNAME(ImagesObj);

void register_images_class(lua_State* L)
{
  using Images = ImagesObj;
  REG_CLASS(L, Images);
}

void push_cel_images(lua_State* L, const ObjectIds& cels)
{
  push_new<ImagesObj>(L, cels);
}

}} // namespace app::script
