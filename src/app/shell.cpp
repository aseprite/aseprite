// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef ENABLE_SCRIPTING
  #error ENABLE_SCRIPTING must be defined
#endif

#include "app/shell.h"

#include "script/engine.h"

#include <iostream>
#include <string>

namespace app {

Shell::Shell()
{
}

Shell::~Shell()
{
}

void Shell::run(script::Engine& engine)
{
  std::cout << "Welcome to " PACKAGE " v" VERSION " Interactive Console" << std::endl;
  std::string line;
  while (std::getline(std::cin, line)) {
    engine.evalCode(line);
  }
  std::cout << "Done\n";
}

} // namespace app
