// Aseprite Scripting Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SCRIPTING_ENGINE_H_INCLUDED
#define SCRIPTING_ENGINE_H_INCLUDED
#pragma once

#include <string>

namespace scripting {
  class EngineDelegate;

  class Engine {
  public:
    Engine(EngineDelegate* delegate);
    ~Engine();

    void eval(const std::string& script);

  private:
    class EngineImpl;
    EngineImpl* m_impl;
  };

}

#endif
