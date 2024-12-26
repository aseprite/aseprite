// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/app.h"
#include "app/resource_finder.h"
#include "app/script/luacpp.h"
#include "app/script/security.h"
#include "base/fs.h"

namespace app { namespace script {

namespace {

struct AppFS {};

int AppFS_pathSeparator(lua_State* L)
{
  const char sep[2] = { base::path_separator, 0 };
  lua_pushstring(L, sep);
  return 1;
}

template<std::string (*Func)(const std::string& path)>
int AppFS_pathElement(lua_State* L)
{
  const char* fn = lua_tostring(L, 1);
  if (fn)
    lua_pushstring(L, Func(fn).c_str());
  else
    lua_pushnil(L);
  return 1;
}

int AppFS_joinPath(lua_State* L)
{
  const char* fn = lua_tostring(L, 1);
  if (!fn) {
    lua_pushnil(L);
    return 1;
  }

  std::string result(fn);
  const int n = lua_gettop(L);
  for (int i = 2; i <= n; ++i) {
    const char* part = lua_tostring(L, i);
    if (part)
      result = base::join_path(result, part);
  }
  lua_pushstring(L, result.c_str());
  return 1;
}

template<std::string (*Func)()>
int AppFS_get_specialPath(lua_State* L)
{
  lua_pushstring(L, Func().c_str());
  return 1;
}

int AppFS_get_userConfigPath(lua_State* L)
{
  ResourceFinder rf(false);
  rf.includeUserDir("");
  std::string dir = rf.defaultFilename();
  lua_pushstring(L, dir.c_str());
  return 1;
}

int AppFS_isFile(lua_State* L)
{
  const char* fn = lua_tostring(L, 1);
  if (fn)
    lua_pushboolean(L, base::is_file(fn));
  else
    lua_pushboolean(L, false);
  return 1;
}

int AppFS_isDirectory(lua_State* L)
{
  const char* fn = lua_tostring(L, 1);
  if (fn)
    lua_pushboolean(L, base::is_directory(fn));
  else
    lua_pushboolean(L, false);
  return 1;
}

int AppFS_fileSize(lua_State* L)
{
  const char* fn = lua_tostring(L, 1);
  if (fn)
    lua_pushinteger(L, base::file_size(fn));
  else
    lua_pushnil(L);
  return 1;
}

int AppFS_listFiles(lua_State* L)
{
  const char* path = lua_tostring(L, 1);
  lua_newtable(L);
  if (path) {
    int i = 0;
    for (const auto& fn : base::list_files(path)) {
      lua_pushstring(L, fn.c_str());
      lua_seti(L, -2, ++i);
    }
  }
  return 1;
}

int AppFS_makeDirectory(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  if (base::is_directory(path)) {
    lua_pushboolean(L, true);
    return 1;
  }

  if (!ask_access(L, path, FileAccessMode::Full, ResourceType::File))
    return luaL_error(L, "the script doesn't have access to create the directory '%s'", path);

  try {
    // TODO don't throw exception from base::make_directory() function
    base::make_directory(path);
  }
  catch (const std::exception&) {
    // Do nothing
  }
  lua_pushboolean(L, base::is_directory(path));
  return 1;
}

int AppFS_makeAllDirectories(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  if (base::is_directory(path)) {
    lua_pushboolean(L, true);
    return 1;
  }

  if (!ask_access(L, path, FileAccessMode::Write, ResourceType::File))
    return luaL_error(L, "the script doesn't have access to create all directories '%s'", path);

  try {
    base::make_all_directories(path);
  }
  catch (const std::exception&) {
    // Do nothing
  }
  lua_pushboolean(L, base::is_directory(path));
  return 1;
}

int AppFS_removeDirectory(lua_State* L)
{
  const char* path = luaL_checkstring(L, 1);
  if (!base::is_directory(path)) {
    lua_pushboolean(L,
                    (base::is_file(path) ? false : // Cannot remove files
                                           true));                     // The directory is already removed
    return 1;
  }

  if (!ask_access(L, path, FileAccessMode::Write, ResourceType::File))
    return luaL_error(L, "the script doesn't have access to remove the directory '%s'", path);

  try {
    base::remove_directory(path);
  }
  catch (const std::exception&) {
    // do nothing...
  }
  lua_pushboolean(L, !base::is_directory(path));
  return 1;
}

const Property AppFS_properties[] = {
  { "pathSeparator",  AppFS_pathSeparator,                               nullptr },
  // Special folder names
  { "currentPath",    AppFS_get_specialPath<base::get_current_path>,     nullptr },
  { "appPath",        AppFS_get_specialPath<base::get_app_path>,         nullptr },
  { "tempPath",       AppFS_get_specialPath<base::get_temp_path>,        nullptr },
  { "userDocsPath",   AppFS_get_specialPath<base::get_user_docs_folder>, nullptr },
  { "userConfigPath", AppFS_get_userConfigPath,                          nullptr },
  { nullptr,          nullptr,                                           nullptr }
};

const luaL_Reg AppFS_methods[] = {
  // Path manipulation
  { "filePath",           AppFS_pathElement<base::get_file_path>            },
  { "fileName",           AppFS_pathElement<base::get_file_name>            },
  { "fileExtension",      AppFS_pathElement<base::get_file_extension>       },
  { "fileTitle",          AppFS_pathElement<base::get_file_title>           },
  { "filePathAndTitle",   AppFS_pathElement<base::get_file_title_with_path> },
  { "normalizePath",      AppFS_pathElement<base::normalize_path>           },
  { "joinPath",           AppFS_joinPath                                    },
  // File system information
  { "isFile",             AppFS_isFile                                      },
  { "isDirectory",        AppFS_isDirectory                                 },
  { "fileSize",           AppFS_fileSize                                    },
  { "listFiles",          AppFS_listFiles                                   },
  // Manipulate directories
  { "makeDirectory",      AppFS_makeDirectory                               },
  { "makeAllDirectories", AppFS_makeAllDirectories                          },
  { "removeDirectory",    AppFS_removeDirectory                             },
  { nullptr,              nullptr                                           }
};

} // anonymous namespace

DEF_MTNAME(AppFS);

void register_app_fs_object(lua_State* L)
{
  REG_CLASS(L, AppFS);
  REG_CLASS_PROPERTIES(L, AppFS);

  lua_getglobal(L, "app");
  lua_pushstring(L, "fs");
  push_new<AppFS>(L);
  lua_rawset(L, -3);
  lua_pop(L, 1);
}

}} // namespace app::script
