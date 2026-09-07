// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "base/base64.h"

namespace app::script {
namespace {
struct AppBase64 {};

int App_Base64_decode(lua_State* L)
{
  const char* input = luaL_checkstring(L, 1);
  auto out = base::decode_base64s(input);
  lua_pushlstring(L, out.c_str(), out.size());
  return 1;
}

int App_Base64_encode(lua_State* L)
{
  size_t len;
  const auto* bytes = lua_tolstring(L, 1, &len);
  lua_pushstring(L, base::encode_base64(std::string(bytes, len)).c_str());
  return 1;
}

const luaL_Reg AppBase64_methods[] = {
  { "encode", App_Base64_encode },
  { "decode", App_Base64_decode },
  { nullptr,  nullptr           }
};
} // namespace
DEF_MTNAME(AppBase64);

void register_app_base64_object(lua_State* L)
{
  REG_CLASS(L, AppBase64);
  lua_getglobal(L, "app");
  lua_pushstring(L, "base64");
  push_new<AppBase64>(L);
  lua_rawset(L, -3);
  lua_pop(L, 1);
}

} // namespace app::script
