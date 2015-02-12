// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef SCRIPTING_ENGINE_H_INCLUDED
#define SCRIPTING_ENGINE_H_INCLUDED
#pragma once

#include <string>

namespace scripting {

  class Engine {
  public:
    Engine();
    ~Engine();

    bool supportEval() const;
    void eval(const std::string& script);

  private:
    class EngineImpl;
    EngineImpl* m_impl;
  };

}

#endif
