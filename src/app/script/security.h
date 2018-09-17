// Aseprite
// Copyright (C) 2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_SECURITY_H_INCLUDED
#define APP_SCRIPT_SECURITY_H_INCLUDED
#pragma once

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/script/engine.h"

namespace app {
namespace script {

  int secure_io_open(lua_State* L);
  int secure_os_execute(lua_State* L);

  bool ask_access(lua_State* L,
                  const char* filename,
                  const FileAccessMode mode,
                  const bool canOpenFile);

} // namespace script
} // namespace app

#endif
