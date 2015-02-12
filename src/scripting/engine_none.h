// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "scripting/engine.h"

class scripting::Engine::EngineImpl
{
public:
  EngineImpl() { }

  bool supportEval() const {
    return false;
  }

  void eval(const std::string& scriptString) {
  }

};
