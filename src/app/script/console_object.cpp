// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/console.h"
#include "script/engine.h"

#include <iostream>

namespace app {

namespace {

void print(const char* str)
{
  if (str) {
    std::cout << str << std::endl; // New line + flush

    if (App::instance()->isGui()) {
      Console().printf("%s\n", str);
    }
  }
}

void Console_assert(script::ContextHandle handle)
{
  script::Context ctx(handle);
  if (!ctx.toBool(1))
    ctx.error(ctx.toString(1));
  ctx.pushUndefined();
}

void Console_log(script::ContextHandle handle)
{
  script::Context ctx(handle);
  std::string output;
  int top = ctx.top();
  const char* s;
  for (int n=1; n<top; ++n) {
    s = ctx.toString(n);
    if (s == nullptr)
      break;
    if (n > 1)
      output += " ";
    output += s;
  }
  if (!output.empty())
    print(output.c_str());
  ctx.pushUndefined();
}

const script::FunctionEntry Console_methods[] = {
  { "assert", Console_assert, 2 },
  { "log", Console_log, 1 },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_console_object(script::Context& ctx)
{
  ctx.pushGlobalObject();
  ctx.registerObject(-1, "console", Console_methods, nullptr);
  ctx.pop();
}

} // namespace app
