// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/rect.h"
#include "script/engine.h"

namespace app {

namespace {

const char* kTag = "Rectangle";

void Rectangle_finalize(script::ContextHandle handle, void* data)
{
  auto rc = (gfx::Rect*)data;
  delete rc;
}

void Rectangle_new(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto rc = new gfx::Rect(0, 0, 0, 0);

  // Copy other rectangle
  if (ctx.isUserData(1, kTag)) {
    auto rc2 = (gfx::Rect*)ctx.toUserData(1, kTag);
    *rc = *rc2;
  }
  // Convert { x, y, width, height } into a Rectangle
  else if (ctx.isObject(1)) {
    ctx.getProp(1, "x");
    ctx.getProp(1, "y");
    ctx.getProp(1, "width");
    ctx.getProp(1, "height");
    rc->x = ctx.toInt(-4);
    rc->y = ctx.toInt(-3);
    rc->w = ctx.toInt(-2);
    rc->h = ctx.toInt(-1);
    ctx.pop(4);
  }
  else if (ctx.isNumber(1)) {
    rc->x = ctx.toInt(1);
    rc->y = ctx.toInt(2);
    rc->w = ctx.toInt(3);
    rc->h = ctx.toInt(4);
  }

  ctx.newObject(kTag, rc, Rectangle_finalize);
}

void Rectangle_get_x(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto rc = (gfx::Rect*)ctx.toUserData(0, kTag);
  ASSERT(rc);
  ctx.pushInt(rc->x);
}

void Rectangle_get_y(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto rc = (gfx::Rect*)ctx.toUserData(0, kTag);
  ctx.pushInt(rc->y);
}

void Rectangle_get_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto rc = (gfx::Rect*)ctx.toUserData(0, kTag);
  ASSERT(rc);
  ctx.pushInt(rc->w);
}

void Rectangle_get_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto rc = (gfx::Rect*)ctx.toUserData(0, kTag);
  ASSERT(rc);
  ctx.pushInt(rc->h);
}

void Rectangle_set_x(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto rc = (gfx::Rect*)ctx.toUserData(0, kTag);
  ASSERT(rc);
  rc->x = ctx.toInt(1);
}

void Rectangle_set_y(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto rc = (gfx::Rect*)ctx.toUserData(0, kTag);
  rc->y = ctx.toInt(1);
}

void Rectangle_set_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto rc = (gfx::Rect*)ctx.toUserData(0, kTag);
  ASSERT(rc);
  rc->w = ctx.toInt(1);
}

void Rectangle_set_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto rc = (gfx::Rect*)ctx.toUserData(0, kTag);
  ASSERT(rc);
  rc->h = ctx.toInt(1);
}

const script::FunctionEntry Rectangle_methods[] = {
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Rectangle_props[] = {
  { "x", Rectangle_get_x, Rectangle_set_x },
  { "y", Rectangle_get_y, Rectangle_set_y },
  { "width", Rectangle_get_width, Rectangle_set_width },
  { "height", Rectangle_get_height, Rectangle_set_height },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_rectangle_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, kTag,
                    Rectangle_new, 3,
                    Rectangle_methods,
                    Rectangle_props);
}

void push_new_rectangle(script::Context& ctx, const gfx::Rect& rc)
{
  ctx.newObject(kTag, new gfx::Rect(rc), Rectangle_finalize);
}

gfx::Rect convert_args_into_rectangle(script::Context& ctx)
{
  gfx::Rect result;
  Rectangle_new(ctx.handle());
  auto rc = (gfx::Rect*)ctx.toUserData(-1, kTag);
  if (rc)
    result = *rc;
  ctx.pop(1);
  return result;
}

} // namespace app
