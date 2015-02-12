// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#include "scripting/engine.h"

#include <v8.h>

using namespace v8;

class scripting::Engine::EngineImpl
{
public:
  EngineImpl() { }

  bool supportEval() const {
    return true;
  }

  void eval(const std::string& scriptString) {
    HandleScope handle_scope;

    Persistent<Context> context = Context::New();
    Context::Scope context_scope(context);

    Handle<String> source = String::New(scriptString.c_str());
    Handle<Script> script = Script::Compile(source);
    Handle<Value> result = script->Run();
    context.Dispose();

    String::AsciiValue ascii(result);
    printf("%s\n", *ascii);
  }

};
