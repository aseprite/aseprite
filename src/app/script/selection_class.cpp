// Aseprite
// Copyright (C) 2015-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/selection_class.h"

#include "app/cmd/deselect_mask.h"
#include "app/cmd/set_mask.h"
#include "app/document.h"
#include "app/script/sprite_wrap.h"
#include "app/transaction.h"
#include "doc/mask.h"

namespace app {

using namespace doc;

namespace {

script::result_t Selection_ctor(script::ContextHandle handle)
{
  return 0;
}

script::result_t Selection_select(script::ContextHandle handle)
{
  script::Context ctx(handle);
  int x = ctx.requireInt(0);
  int y = ctx.requireInt(1);
  int w = ctx.requireInt(2);
  int h = ctx.requireInt(3);

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (wrap) {
    Document* doc = wrap->document();

    Mask newMask;
    if (w > 0 && h > 0)
      newMask.replace(gfx::Rect(x, y, w, h));

    wrap->transaction().execute(new cmd::SetMask(doc, &newMask));
  }

  return 0;
}

script::result_t Selection_selectAll(script::ContextHandle handle)
{
  script::Context ctx(handle);

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (wrap) {
    Document* doc = wrap->document();

    Mask newMask;
    newMask.replace(doc->sprite()->bounds());

    wrap->transaction().execute(new cmd::SetMask(doc, &newMask));
  }

  return 0;
}

script::result_t Selection_deselect(script::ContextHandle handle)
{
  script::Context ctx(handle);

  auto wrap = (SpriteWrap*)ctx.getThis();
  if (wrap) {
    Document* doc = wrap->document();
    wrap->transaction().execute(new cmd::DeselectMask(doc));
  }

  return 0;
}

script::result_t Selection_get_bounds(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (SpriteWrap*)ctx.getThis();
  if (wrap) {
    Document* doc = wrap->document();
    if (doc->isMaskVisible()) {
      gfx::Rect bounds = doc->mask()->bounds();
      script::index_t obj = ctx.pushObject();
      ctx.pushNumber(bounds.x); ctx.setProp(obj, "x");
      ctx.pushNumber(bounds.y); ctx.setProp(obj, "y");
      ctx.pushNumber(bounds.w); ctx.setProp(obj, "width");
      ctx.pushNumber(bounds.h); ctx.setProp(obj, "height");
      return 1;
    }
  }
  return 0;
}

const script::FunctionEntry Selection_methods[] = {
  { "select", Selection_select, 4 },
  { "selectAll", Selection_selectAll, 0 },
  { "deselect", Selection_deselect, 1 },
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Selection_props[] = {
  { "bounds", Selection_get_bounds, nullptr },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_selection_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, "Selection", Selection_ctor, 3, Selection_methods, Selection_props);
}

} // namespace app
