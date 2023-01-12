// Aseprite
// Copyright (C) 2019-2023  Igara Studio S.A.
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/security.h"

#include "app/app.h"
#include "app/context.h"
#include "app/i18n/strings.h"
#include "app/ini_file.h"
#include "app/launcher.h"
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "base/convert_to.h"
#include "base/fs.h"
#include "base/sha1.h"
#include "fmt/format.h"

#include "script_access.xml.h"

#include <algorithm>
#include <cstring>
#include <unordered_map>

namespace app {
namespace script {

#ifdef ENABLE_UI
namespace {

// Map from .lua file name -> sha1
std::unordered_map<std::string, std::string> g_keys;

std::string get_key(const std::string& source)
{
  auto it = g_keys.find(source);
  if (it != g_keys.end())
    return it->second;
  else
    return g_keys[source] = base::convert_to<std::string>(
      base::Sha1::calculateFromString(source));
}

std::string get_script_filename(lua_State* L)
{
  // Get script name
  lua_getglobal(L, "debug");
  lua_getfield(L, -1, "getinfo");
  lua_remove(L, -2);
  lua_pushinteger(L, 2);
  lua_pushstring(L, "S");
  lua_call(L, 2, 1);
  lua_getfield(L, -1, "source");
  const char* source = lua_tostring(L, -1);
  std::string script;
  if (source && *source)
    script = source+1;
  lua_pop(L, 2);
  return script;
}

} // anonymous namespace
#endif // ENABLE_UI

int secure_io_open(lua_State* L)
{
  int n = lua_gettop(L);

  std::string absFilename = base::get_absolute_path(lua_tostring(L, 1));

  FileAccessMode mode = FileAccessMode::Read; // Read is the default access
  if (lua_tostring(L, 2) &&
      std::strchr(lua_tostring(L, 2), 'w') != nullptr) {
    mode = FileAccessMode::Write;
  }

  if (!ask_access(L, absFilename.c_str(), mode, ResourceType::File)) {
    return luaL_error(L, "the script doesn't have access to file '%s'",
                      absFilename.c_str());
  }

  lua_pushvalue(L, lua_upvalueindex(1));
  lua_pushstring(L, absFilename.c_str());
  for (int i=2; i<=n; ++i)
    lua_pushvalue(L, i);
  lua_call(L, n, 1);
  return 1;
}

int secure_os_execute(lua_State* L)
{
  int n = lua_gettop(L);
  if (n == 0)
    return 0;

  const char* cmd = lua_tostring(L, 1);
  if (!ask_access(L, cmd, FileAccessMode::Execute, ResourceType::Command)) {
    // Stop script
    return luaL_error(L, "the script doesn't have access to execute the command: '%s'",
                      cmd);
  }

  lua_pushvalue(L, lua_upvalueindex(1));
  for (int i=1; i<=n; ++i)
    lua_pushvalue(L, i);
  lua_call(L, n, 1);
  return 1;
}

bool ask_access(lua_State* L,
                const char* filename,
                const FileAccessMode mode,
                const ResourceType resourceType)
{
#ifdef ENABLE_UI
  // Ask for permission to open the file
  if (App::instance()->context()->isUIAvailable()) {
    std::string script = get_script_filename(L);
    if (script.empty()) // No script
      return luaL_error(L, "no debug information (script filename) to secure io.open() call");

    const char* section = "script_access";
    std::string key = get_key(script);

    int access = get_config_int(section, key.c_str(), 0);

    // Has the correct access
    if ((access & int(mode)) == int(mode))
      return true;

    std::string allowButtonText =
      mode == FileAccessMode::OpenSocket ?
        Strings::script_access_allow_open_conn_access():
      mode == FileAccessMode::Execute ?
        Strings::script_access_allow_execute_access():
      mode == FileAccessMode::Write ?
        Strings::script_access_allow_write_access():
        Strings::script_access_allow_read_access();

    app::gen::ScriptAccess dlg;
    dlg.script()->setText(script);

    {
      std::string label;
      switch (resourceType) {
        case ResourceType::File: label = Strings::script_access_file_label(); break;
        case ResourceType::Command: label = Strings::script_access_command_label(); break;
        case ResourceType::WebSocket: label = Strings::script_access_websocket_label(); break;
      }
      dlg.fileLabel()->setText(label);
    }

    dlg.file()->setText(filename);
    dlg.allow()->setText(allowButtonText);
    dlg.allow()->processMnemonicFromText();

    dlg.script()->Click.connect(
      [&dlg]{
        app::launcher::open_folder(dlg.script()->text());
      });

    dlg.full()->Click.connect(
      [&dlg, &allowButtonText](){
        if (dlg.full()->isSelected()) {
          dlg.dontShow()->setSelected(true);
          dlg.dontShow()->setEnabled(false);
          dlg.allow()->setText(Strings::script_access_give_full_access());
          dlg.allow()->processMnemonicFromText();
          dlg.layout();
        }
        else {
          dlg.dontShow()->setEnabled(true);
          dlg.allow()->setText(allowButtonText);
          dlg.allow()->processMnemonicFromText();
          dlg.layout();
        }
      });

    if (resourceType == ResourceType::File) {
      dlg.file()->Click.connect(
        [&dlg]{
          std::string fn = dlg.file()->text();
          if (base::is_file(fn))
            app::launcher::open_folder(fn);
          else
            app::launcher::open_folder(base::get_file_path(fn));
        });
    }

    dlg.openWindowInForeground();
    const bool allow = (dlg.closer() == dlg.allow());

    // Save selected option
    if (allow && dlg.dontShow()->isSelected()) {
      if (dlg.full()->isSelected())
        set_config_int(section, key.c_str(), access | int(FileAccessMode::Full));
      else
        set_config_int(section, key.c_str(), access | int(mode));
      flush_config_file();
    }

    if (!allow)
      return false;
  }
#endif
  return true;
}

} // namespace script
} // namespace app
