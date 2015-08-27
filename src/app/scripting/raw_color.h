// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

namespace {

scripting::result_t rgba(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  int r = ctx.requireInt(0);
  int g = ctx.requireInt(1);
  int b = ctx.requireInt(2);
  int a = (ctx.isUndefined(3) ? 255: ctx.requireInt(3));
  ctx.pushUInt(doc::rgba(r, g, b, a));
  return 1;
}

scripting::result_t rgbaR(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getr(ctx.requireUInt(0)));
  return 1;
}

scripting::result_t rgbaG(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getg(ctx.requireUInt(0)));
  return 1;
}

scripting::result_t rgbaB(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getb(ctx.requireUInt(0)));
  return 1;
}

scripting::result_t rgbaA(scripting::ContextHandle handle)
{
  scripting::Context ctx(handle);
  ctx.pushUInt(doc::rgba_geta(ctx.requireUInt(0)));
  return 1;
}

} // anonymous namespace
