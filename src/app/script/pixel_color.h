// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "doc/color.h"

namespace {

script::result_t pixelColor_rgba(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int r = ctx.requireInt(0);
  int g = ctx.requireInt(1);
  int b = ctx.requireInt(2);
  int a = (ctx.isUndefined(3) ? 255: ctx.requireInt(3));
  ctx.pushUInt(doc::rgba(r, g, b, a));
  return 1;
}

script::result_t pixelColor_rgbaR(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getr(ctx.requireUInt(0)));
  return 1;
}

script::result_t pixelColor_rgbaG(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getg(ctx.requireUInt(0)));
  return 1;
}

script::result_t pixelColor_rgbaB(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getb(ctx.requireUInt(0)));
  return 1;
}

script::result_t pixelColor_rgbaA(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::rgba_geta(ctx.requireUInt(0)));
  return 1;
}

script::result_t pixelColor_graya(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int v = ctx.requireInt(0);
  int a = (ctx.isUndefined(1) ? 255: ctx.requireInt(1));
  ctx.pushUInt(doc::graya(v, a));
  return 1;
}

script::result_t pixelColor_grayaV(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::graya_getv(ctx.requireUInt(0)));
  return 1;
}

script::result_t pixelColor_grayaA(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::graya_geta(ctx.requireUInt(0)));
  return 1;
}

const script::FunctionEntry pixelColor_methods[] = {
  { "rgba", pixelColor_rgba, 4 },
  { "rgbaR", pixelColor_rgbaR, 1 },
  { "rgbaG", pixelColor_rgbaG, 1 },
  { "rgbaB", pixelColor_rgbaB, 1 },
  { "rgbaA", pixelColor_rgbaA, 1 },
  { "graya", pixelColor_graya, 2 },
  { "grayaV", pixelColor_grayaV, 1 },
  { "grayaA", pixelColor_grayaA, 1 },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace
