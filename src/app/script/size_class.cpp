// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gfx/size.h"
#include "script/engine.h"

namespace app {

namespace {

const char* kTag = "Size";

void Size_finalize(script::ContextHandle handle, void* data)
{
  auto sz = (gfx::Size*)data;
  delete sz;
}

void Size_new(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto sz = new gfx::Size(0, 0);

  // Copy other size
  if (ctx.isUserData(1, kTag)) {
    auto sz2 = (gfx::Size*)ctx.toUserData(1, kTag);
    *sz = *sz2;
  }
  // Convert { width, height } into a Size
  else if (ctx.isObject(1)) {
    ctx.getProp(1, "width");
    ctx.getProp(1, "height");
    sz->w = ctx.toInt(-2);
    sz->h = ctx.toInt(-1);
    ctx.pop(4);
  }
  else if (ctx.isNumber(1)) {
    sz->w = ctx.toInt(1);
    sz->h = ctx.toInt(2);
  }

  ctx.newObject(kTag, sz, Size_finalize);
}

void Size_get_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto sz = (gfx::Size*)ctx.toUserData(0, kTag);
  ASSERT(sz);
  ctx.pushInt(sz->w);
}

void Size_get_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto sz = (gfx::Size*)ctx.toUserData(0, kTag);
  ASSERT(sz);
  ctx.pushInt(sz->h);
}

void Size_set_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto sz = (gfx::Size*)ctx.toUserData(0, kTag);
  ASSERT(sz);
  sz->w = ctx.toInt(1);
}

void Size_set_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto sz = (gfx::Size*)ctx.toUserData(0, kTag);
  ASSERT(sz);
  sz->h = ctx.toInt(1);
}

const script::FunctionEntry Size_methods[] = {
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Size_props[] = {
  { "width", Size_get_width, Size_set_width },
  { "height", Size_get_height, Size_set_height },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_size_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, kTag,
                    Size_new, 3,
                    Size_methods,
                    Size_props);
}

void push_new_size(script::Context& ctx, const gfx::Size& sz)
{
  ctx.newObject(kTag, new gfx::Size(sz), Size_finalize);
}

gfx::Size convert_args_into_size(script::Context& ctx)
{
  gfx::Size result;
  Size_new(ctx.handle());
  auto sz = (gfx::Size*)ctx.toUserData(-1, kTag);
  if (sz)
    result = *sz;
  ctx.pop(1);
  return result;
}

} // namespace app
