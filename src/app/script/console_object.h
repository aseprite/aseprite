// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_SCRIPT_CONSOLE_OBJECT_H_INCLUDED
#define APP_SCRIPT_CONSOLE_OBJECT_H_INCLUDED
#pragma once

namespace script {
  class Context;
}

namespace app {

  void register_console_object(script::Context& ctx);

} // namespace app

#endif
