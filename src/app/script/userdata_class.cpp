// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/set_user_data.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/tx.h"
#include "doc/sprite.h"
#include "doc/with_user_data.h"

namespace app {
namespace script {

using namespace doc;

namespace {

int UserData_get_text(lua_State* L)
{
  auto obj = get_ptr<WithUserData>(L, 1);
  lua_pushstring(L, obj->userData().text().c_str());
  return 1;
}

int UserData_get_color(lua_State* L)
{
  auto obj = get_ptr<WithUserData>(L, 1);
  doc::color_t docColor = obj->userData().color();
  app::Color appColor = app::Color::fromRgb(doc::rgba_getr(docColor),
                                            doc::rgba_getg(docColor),
                                            doc::rgba_getb(docColor),
                                            doc::rgba_geta(docColor));
  push_obj<app::Color>(L, appColor);
  return 1;
}

int UserData_set_text(lua_State* L)
{
  auto obj = get_ptr<WithUserData>(L, 1);
  const char* text = lua_tostring(L, 2);
  UserData ud = obj->userData();
  ud.setText(text);
  Tx tx;
  tx(new cmd::SetUserData(obj, ud));
  tx.commit();
  return 0;
}

int UserData_set_color(lua_State* L)
{
  auto obj = get_ptr<WithUserData>(L, 1);
  doc::color_t docColor = convert_args_into_pixel_color(L, 2);
  UserData ud = obj->userData();
  ud.setColor(docColor);
  Tx tx;
  tx(new cmd::SetUserData(obj, ud));
  tx.commit();
  return 0;
}

const luaL_Reg UserData_methods[] = {
  { nullptr, nullptr }
};

const Property UserData_properties[] = {
  { "text", UserData_get_text, UserData_set_text },
  { "color", UserData_get_color, UserData_set_color },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(doc::WithUserData);

void register_userdata_class(lua_State* L)
{
  using UserData = doc::WithUserData;
  REG_CLASS(L, UserData);
  REG_CLASS_PROPERTIES(L, UserData);
}

void push_userdata(lua_State* L, WithUserData* userData)
{
  push_ptr<WithUserData>(L, userData);
}

} // namespace script
} // namespace app
