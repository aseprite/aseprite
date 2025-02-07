// Aseprite
// Copyright (C) 2018-2023  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_USERDATA_HELPER_H_INCLUDED
#define APP_SCRIPT_USERDATA_HELPER_H_INCLUDED
#pragma once

#include "app/cmd/set_user_data.h"
#include "app/cmd/set_user_data_properties.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/script/luacpp.h"
#include "app/script/values.h"
#include "app/tx.h"
#include "doc/cel.h"
#include "doc/with_user_data.h"

namespace app { namespace script {

template<typename T>
inline doc::WithUserData* get_WithUserData(T* obj)
{
  return static_cast<doc::WithUserData*>(obj);
}

template<>
inline doc::WithUserData* get_WithUserData<doc::Cel>(doc::Cel* obj)
{
  return obj->data();
}

template<typename T>
int UserData_get_text(lua_State* L)
{
  auto obj = get_docobj<T>(L, 1);
  lua_pushstring(L, get_WithUserData<T>(obj)->userData().text().c_str());
  return 1;
}

template<typename T>
int UserData_get_color(lua_State* L)
{
  auto obj = get_docobj<T>(L, 1);
  doc::color_t docColor = get_WithUserData<T>(obj)->userData().color();
  app::Color appColor = app::Color::fromRgb(doc::rgba_getr(docColor),
                                            doc::rgba_getg(docColor),
                                            doc::rgba_getb(docColor),
                                            doc::rgba_geta(docColor));
  if (appColor.getAlpha() == 0)
    appColor = app::Color::fromMask();
  push_obj<app::Color>(L, appColor);
  return 1;
}

template<typename T>
int UserData_get_properties(lua_State* L)
{
  auto obj = get_docobj<T>(L, 1);
  push_properties(L, get_WithUserData<T>(obj), std::string());
  return 1;
}

template<typename T>
int UserData_set_text(lua_State* L)
{
  auto obj = get_docobj<T>(L, 1);
  const char* text = lua_tostring(L, 2);
  auto wud = get_WithUserData<T>(obj);
  UserData ud = wud->userData();
  ud.setText(text ? std::string(text) : std::string());
  if (auto spr = obj->sprite()) {
    Tx tx(spr);
    tx(new cmd::SetUserData(wud, ud, static_cast<Doc*>(spr->document())));
    tx.commit();
  }
  else {
    wud->setUserData(ud);
  }
  return 0;
}

template<typename T>
int UserData_set_color(lua_State* L)
{
  auto obj = get_docobj<T>(L, 1);
  doc::color_t docColor = convert_args_into_pixel_color(L, 2, doc::IMAGE_RGB);
  auto wud = get_WithUserData<T>(obj);
  UserData ud = wud->userData();
  ud.setColor(docColor);
  if (auto spr = obj->sprite()) {
    Tx tx(spr);
    tx(new cmd::SetUserData(wud, ud, static_cast<Doc*>(spr->document())));
    tx.commit();
  }
  else {
    wud->setUserData(ud);
  }
  return 0;
}

template<typename T>
int UserData_set_properties(lua_State* L)
{
  auto obj = get_docobj<T>(L, 1);
  auto wud = get_WithUserData<T>(obj);
  auto newProperties = get_value_from_lua<doc::UserData::Properties>(L, 2);
  if (auto spr = obj->sprite()) {
    Tx tx(spr);
    tx(new cmd::SetUserDataProperties(wud, std::string(), std::move(newProperties)));
    tx.commit();
  }
  else {
    auto& properties = wud->userData().properties();
    properties = std::move(newProperties);
  }
  return 0;
}

}} // namespace app::script

#endif
