// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "doc/color.h"
#include "script/engine.h"

namespace app {

namespace {

const char* kTag = "PixelColor";

void PixelColor_new(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUndefined(); // Cannot create new pixel colors (just subobject of "app")
}

void PixelColor_rgba(script::ContextHandle handle)
{
  script::Context ctx(handle);
  const int r = ctx.requireUInt(1);
  const int g = ctx.requireUInt(2);
  const int b = ctx.requireUInt(3);
  const int a = (ctx.isUndefined(4) ? 255: ctx.requireUInt(4));
  ctx.pushUInt(doc::rgba(r, g, b, a));
}

void PixelColor_rgbaR(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getr(ctx.requireUInt(1)));
}

void PixelColor_rgbaG(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getg(ctx.requireUInt(1)));
}

void PixelColor_rgbaB(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::rgba_getb(ctx.requireUInt(1)));
}

void PixelColor_rgbaA(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::rgba_geta(ctx.requireUInt(1)));
}

void PixelColor_graya(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int v = ctx.requireUInt(1);
  int a = (ctx.isUndefined(2) ? 255: ctx.requireUInt(2));
  ctx.pushUInt(doc::graya(v, a));
}

void PixelColor_grayaV(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::graya_getv(ctx.requireUInt(1)));
}

void PixelColor_grayaA(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.pushUInt(doc::graya_geta(ctx.requireUInt(1)));
}

const script::FunctionEntry PixelColor_methods[] = {
  { "rgba", PixelColor_rgba, 4 },
  { "rgbaR", PixelColor_rgbaR, 1 },
  { "rgbaG", PixelColor_rgbaG, 1 },
  { "rgbaB", PixelColor_rgbaB, 1 },
  { "rgbaA", PixelColor_rgbaA, 1 },
  { "graya", PixelColor_graya, 2 },
  { "grayaV", PixelColor_grayaV, 1 },
  { "grayaA", PixelColor_grayaA, 1 },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_pixel_color_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, kTag,
                    PixelColor_new, 0,
                    PixelColor_methods, nullptr);
}

} // namespace app
