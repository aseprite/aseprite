// Aseprite Scripting Library
// Copyright (c) 2015-2016 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "script/engine_delegate.h"

#include <cstdio>

namespace script {

void StdoutEngineDelegate::onConsolePrint(const char* text)
{
  std::printf("%s\n", text);
}

} // namespace script
