// Aseprite Scripting Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "scripting/engine.h"

#include "scripting/engine_duktape.h"

namespace scripting {

  Engine::Engine(EngineDelegate* delegate)
    : m_impl(new EngineImpl(delegate)) {
  }

  Engine::~Engine() {
    delete m_impl;
  }

  void Engine::eval(const std::string& script) {
    m_impl->eval(script);
  }

} // namespace scripting
