// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/replace_image.h"
#include "app/cmd/set_cel_opacity.h"
#include "app/cmd/set_cel_position.h"
#include "app/cmd/set_cel_zindex.h"
#include "app/doc_api.h"
#include "app/script/docobj.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/script/userdata.h"
#include "doc/cel.h"
#include "doc/sprite.h"

namespace app {
namespace script {

using namespace doc;

namespace {

int Cel_eq(lua_State* L)
{
  const auto a = may_get_docobj<Cel>(L, 1);
  const auto b = may_get_docobj<Cel>(L, 2);
  lua_pushboolean(L, (!a && !b) || (a && b && a->id() == b->id()));
  return 1;
}

int Cel_get_sprite(lua_State* L)
{
  const auto cel = get_docobj<Cel>(L, 1);
  push_docobj(L, cel->sprite());
  return 1;
}

int Cel_get_layer(lua_State* L)
{
  const auto cel = get_docobj<Cel>(L, 1);
  push_docobj(L, (Layer*)cel->layer());
  return 1;
}

int Cel_get_frame(lua_State* L)
{
  const auto cel = get_docobj<Cel>(L, 1);
  if (auto sprite = cel->sprite())
    push_sprite_frame(L, sprite, cel->frame());
  else
    lua_pushnil(L);
  return 1;
}

int Cel_get_frameNumber(lua_State* L)
{
  const auto cel = get_docobj<Cel>(L, 1);
  lua_pushinteger(L, cel->frame()+1);
  return 1;
}

int Cel_get_image(lua_State* L)
{
  auto cel = get_docobj<Cel>(L, 1);
  push_cel_image(L, cel);
  return 1;
}

int Cel_get_position(lua_State* L)
{
  const auto cel = get_docobj<Cel>(L, 1);
  push_new<gfx::Point>(L, cel->position());
  return 1;
}

int Cel_get_bounds(lua_State* L)
{
  const auto cel = get_docobj<Cel>(L, 1);
  push_new<gfx::Rect>(L, cel->bounds());
  return 1;
}

int Cel_get_opacity(lua_State* L)
{
  const auto cel = get_docobj<Cel>(L, 1);
  lua_pushinteger(L, cel->opacity());
  return 1;
}

int Cel_get_zIndex(lua_State* L)
{
  const auto cel = get_docobj<Cel>(L, 1);
  lua_pushinteger(L, cel->zIndex());
  return 1;
}

int Cel_set_frame(lua_State* L)
{
  const auto cel = get_docobj<Cel>(L, 1);
  doc::frame_t frame = get_frame_number_from_arg(L, 2);

  if (cel->frame() == frame)
    return 0;

  Tx tx;
  Doc* doc = static_cast<Doc*>(cel->document());
  DocApi api = doc->getApi(tx);
  api.moveCel(cel->layer(), cel->frame(),
              cel->layer(), frame);
  tx.commit();
  return 0;
}

int Cel_set_image(lua_State* L)
{
  auto cel = get_docobj<Cel>(L, 1);
  auto srcImage = get_image_from_arg(L, 2);
  ImageRef newImage(Image::createCopy(srcImage));

  Tx tx;
  tx(new cmd::ReplaceImage(cel->sprite(),
                           cel->imageRef(),
                           newImage));
  tx.commit();
  return 0;
}

int Cel_set_position(lua_State* L)
{
  auto cel = get_docobj<Cel>(L, 1);
  const gfx::Point pos = convert_args_into_point(L, 2);
  Tx tx;
  tx(new cmd::SetCelPosition(cel, pos.x, pos.y));
  tx.commit();
  return 0;
}

int Cel_set_opacity(lua_State* L)
{
  auto cel = get_docobj<Cel>(L, 1);
  Tx tx;
  tx(new cmd::SetCelOpacity(cel, lua_tointeger(L, 2)));
  tx.commit();
  return 0;
}

int Cel_set_zIndex(lua_State* L)
{
  auto cel = get_docobj<Cel>(L, 1);
  Tx tx;
  tx(new cmd::SetCelZIndex(cel, lua_tointeger(L, 2)));
  tx.commit();
  return 0;
}

const luaL_Reg Cel_methods[] = {
  { "__eq", Cel_eq },
  { nullptr, nullptr }
};

const Property Cel_properties[] = {
  { "sprite", Cel_get_sprite, nullptr },
  { "layer", Cel_get_layer, nullptr },
  { "frame", Cel_get_frame, Cel_set_frame },
  { "frameNumber", Cel_get_frameNumber, Cel_set_frame },
  { "image", Cel_get_image, Cel_set_image },
  { "bounds", Cel_get_bounds, nullptr },
  { "position", Cel_get_position, Cel_set_position },
  { "opacity", Cel_get_opacity, Cel_set_opacity },
  { "zIndex", Cel_get_zIndex, Cel_set_zIndex },
  { "color", UserData_get_color<Cel>, UserData_set_color<Cel> },
  { "data", UserData_get_text<Cel>, UserData_set_text<Cel> },
  { "properties", UserData_get_properties<Cel>, UserData_set_properties<Cel> },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(Cel);

void register_cel_class(lua_State* L)
{
  using Cel = doc::Cel;
  REG_CLASS(L, Cel);
  REG_CLASS_PROPERTIES(L, Cel);
}

void push_sprite_cel(lua_State* L, Cel* cel)
{
  push_docobj(L, cel);
}

} // namespace script
} // namespace app
