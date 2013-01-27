/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "scripting/engine.h"

#ifdef HAVE_V8_LIBRARY
#include "scripting/engine_v8.h"
#else
#include "scripting/engine_none.h"
#endif

namespace scripting {

  Engine::Engine() : m_impl(new EngineImpl) {
  }

  Engine::~Engine() {
    delete m_impl;
  }

  bool Engine::supportEval() const {
    return m_impl->supportEval();
  }

  void Engine::eval(const std::string& script) {
    m_impl->eval(script);
  }

} // namespace scripting
