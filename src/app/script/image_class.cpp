// Aseprite
// Copyright (C) 2015-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/image_class.h"

#include "app/script/image_wrap.h"
#include "doc/image.h"

namespace app {

namespace {

script::result_t Image_ctor(script::ContextHandle handle)
{
  return 0;
}

script::result_t Image_putPixel(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int x = ctx.requireInt(0);
  int y = ctx.requireInt(1);
  doc::color_t color = ctx.requireUInt(2);

  auto wrap = (ImageWrap*)ctx.getThis();
  if (wrap) {
    wrap->modifyRegion(gfx::Region(gfx::Rect(x, y, 1, 1)));
    wrap->image()->putPixel(x, y, color);
  }

  return 0;
}

script::result_t Image_getPixel(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int x = ctx.requireInt(0);
  int y = ctx.requireInt(1);

  auto wrap = (ImageWrap*)ctx.getThis();
  if (wrap) {
    doc::color_t color = wrap->image()->getPixel(x, y);
    ctx.pushUInt(color);
    return 1;
  }
  else
    return 0;
}

script::result_t Image_get_width(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (ImageWrap*)ctx.getThis();
  if (wrap) {
    ctx.pushInt(wrap->image()->width());
    return 1;
  }
  else
    return 0;
}

script::result_t Image_get_height(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (ImageWrap*)ctx.getThis();
  if (wrap) {
    ctx.pushInt(wrap->image()->height());
    return 1;
  }
  else
    return 0;
}

const script::FunctionEntry Image_methods[] = {
  { "getPixel", Image_getPixel, 2 },
  { "putPixel", Image_putPixel, 3 },
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Image_props[] = {
  { "width", Image_get_width, nullptr },
  { "height", Image_get_height, nullptr },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_image_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, "Image", Image_ctor, 0, Image_methods, Image_props);
}

} // namespace app
