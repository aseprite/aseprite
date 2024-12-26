// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "base/config.h"
#include "base/platform.h"
#include "updater/user_agent.h"

namespace app { namespace script {

namespace {

struct AppOS {};

int AppOS_get_name(lua_State* L)
{
#if LAF_WINDOWS
  lua_pushstring(L, "Windows");
#elif LAF_MACOS
  lua_pushstring(L, "macOS");
#elif LAF_LINUX
  lua_pushstring(L, "Linux");
#else
  lua_pushnil(L);
#endif
  return 1;
}

int AppOS_get_version(lua_State* L)
{
  base::Platform p = base::get_platform();
  push_version(L, p.osVer);
  return 1;
}

int AppOS_get_fullName(lua_State* L)
{
  lua_pushstring(L, updater::getFullOSString().c_str());
  return 1;
}

int AppOS_get_windows(lua_State* L)
{
  lua_pushboolean(L, base::Platform::os == base::Platform::OS::Windows);
  return 1;
}

int AppOS_get_macos(lua_State* L)
{
  lua_pushboolean(L, base::Platform::os == base::Platform::OS::macOS);
  return 1;
}

int AppOS_get_linux(lua_State* L)
{
  lua_pushboolean(L, base::Platform::os == base::Platform::OS::Linux);
  return 1;
}

int AppOS_get_x64(lua_State* L)
{
  lua_pushboolean(L, base::Platform::arch == base::Platform::Arch::x64);
  return 1;
}

int AppOS_get_x86(lua_State* L)
{
  lua_pushboolean(L, base::Platform::arch == base::Platform::Arch::x86);
  return 1;
}

int AppOS_get_arm64(lua_State* L)
{
  lua_pushboolean(L, base::Platform::arch == base::Platform::Arch::arm64);
  return 1;
}

const Property AppOS_properties[] = {
  { "name",     AppOS_get_name,     nullptr },
  { "version",  AppOS_get_version,  nullptr },
  { "fullName", AppOS_get_fullName, nullptr },
  { "windows",  AppOS_get_windows,  nullptr },
  { "macos",    AppOS_get_macos,    nullptr },
  { "linux",    AppOS_get_linux,    nullptr },
  { "x64",      AppOS_get_x64,      nullptr },
  { "x86",      AppOS_get_x86,      nullptr },
  { "arm64",    AppOS_get_arm64,    nullptr },
  { nullptr,    nullptr,            nullptr }
};

const luaL_Reg AppOS_methods[] = {
  { nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(AppOS);

void register_app_os_object(lua_State* L)
{
  REG_CLASS(L, AppOS);
  REG_CLASS_PROPERTIES(L, AppOS);

  lua_getglobal(L, "app");
  lua_pushstring(L, "os");
  push_new<AppOS>(L);
  lua_rawset(L, -3);
  lua_pop(L, 1);
}

}} // namespace app::script
