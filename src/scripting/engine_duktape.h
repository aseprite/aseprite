// Aseprite Scripting Library
// Copyright (c) 2015 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "scripting/engine.h"

#include "base/convert_to.h"
#include "base/exception.h"
#include "base/memory.h"
#include "scripting/engine_delegate.h"

#define DUK_OPT_PANIC_HANDLER scripting_engine_panic_handler
#include <duktape.h>

class ScriptingEngineException : public base::Exception {
public:
  ScriptingEngineException(duk_errcode_t code, const char* msg)
    : Exception(std::string(msg) + " (code " + base::convert_to<std::string>(code) + ")") {
  }
};

static void scripting_engine_panic_handler(duk_errcode_t code, const char* msg)
{
  throw ScriptingEngineException(code, msg);
}

extern "C" {
  #include <duktape.c>
}

class scripting::Engine::EngineImpl {
private:

public:
  EngineImpl(EngineDelegate* delegate)
    : m_delegate(delegate) {
    m_ctx = duk_create_heap_default();
  }

  ~EngineImpl() {
    duk_destroy_heap(m_ctx);
  }

  void eval(const std::string& scriptString) {
    try {
      duk_eval_string(m_ctx, scriptString.c_str());
      m_delegate->onConsolePrint(duk_safe_to_string(m_ctx, -1));
      duk_pop(m_ctx);
    }
    catch (const std::exception& ex) {
      std::string err = "Error: ";
      err += ex.what();
      m_delegate->onConsolePrint(err.c_str());
    }
  }

private:
  duk_context* m_ctx;
  EngineDelegate* m_delegate;
};
