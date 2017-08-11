// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/point.h"
#include "script/engine.h"

namespace app {

namespace {

const char* kTag = "Point";

void Point_finalize(script::ContextHandle handle, void* data)
{
  auto pt = (gfx::Point*)data;
  delete pt;
}

void Point_new(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto pt = new gfx::Point(0, 0);

  // Copy other rectangle
  if (ctx.isUserData(1, kTag)) {
    auto pt2 = (gfx::Point*)ctx.toUserData(1, kTag);
    *pt = *pt2;
  }
  // Convert { x, y } into a Point
  else if (ctx.isObject(1)) {
    ctx.getProp(1, "x");
    ctx.getProp(1, "y");
    pt->x = ctx.toInt(-2);
    pt->y = ctx.toInt(-1);
    ctx.pop(2);
  }
  else if (ctx.isNumber(1)) {
    pt->x = ctx.toInt(1);
    pt->y = ctx.toInt(2);
  }

  ctx.newObject(kTag, pt, Point_finalize);
}

void Point_get_x(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto pt = (gfx::Point*)ctx.toUserData(0, kTag);
  ASSERT(pt);
  ctx.pushInt(pt->x);
}

void Point_get_y(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto pt = (gfx::Point*)ctx.toUserData(0, kTag);
  ctx.pushInt(pt->y);
}

void Point_set_x(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto pt = (gfx::Point*)ctx.toUserData(0, kTag);
  ASSERT(pt);
  pt->x = ctx.toInt(1);
}

void Point_set_y(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto pt = (gfx::Point*)ctx.toUserData(0, kTag);
  pt->y = ctx.toInt(1);
}

const script::FunctionEntry Point_methods[] = {
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Point_props[] = {
  { "x", Point_get_x, Point_set_x },
  { "y", Point_get_y, Point_set_y },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_point_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, kTag,
                    Point_new, 3,
                    Point_methods,
                    Point_props);
}

void push_new_point(script::Context& ctx, const gfx::Point& pt)
{
  ctx.newObject(kTag, new gfx::Point(pt), Point_finalize);
}

gfx::Point convert_args_into_point(script::Context& ctx)
{
  gfx::Point result;
  Point_new(ctx.handle());
  auto pt = (gfx::Point*)ctx.toUserData(-1, kTag);
  if (pt)
    result = *pt;
  ctx.pop(1);
  return result;
}

} // namespace app
