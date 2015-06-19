// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "scripting/engine.h"

#include <duktape.h>

class scripting::Engine::EngineImpl
{
public:
  EngineImpl() { }

  bool supportEval() const {
    return true;
  }

  void eval(const std::string& scriptString) {
    duk_context *ctx = duk_create_heap_default();
    duk_eval_string(ctx, scriptString.c_str());
    duk_pop(ctx);
    printf("%s\n", duk_get_string(ctx, -1));
    duk_destroy_heap(ctx);
  }

};
