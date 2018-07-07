// Aseprite
// Copyright (C) 2015-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/cmd/deselect_mask.h"
#include "app/cmd/set_mask.h"
#include "app/doc.h"
#include "app/script/app_scripting.h"
#include "app/script/sprite_wrap.h"
#include "app/transaction.h"
#include "doc/mask.h"
#include "script/engine.h"

namespace app {

using namespace doc;

namespace {

const char* kTag = "Selection";

struct MaskWrap {
  Mask* mask;
  SpriteWrap* sprite;
  MaskWrap(Mask* mask, SpriteWrap* sprite)
    : mask(mask)
    , sprite(sprite) { }
};

void Selection_finalize(script::ContextHandle handle, void* data)
{
  auto wrap = (MaskWrap*)data;
  delete wrap;
}

void Selection_new(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.newObject(kTag, new MaskWrap(new Mask, nullptr), Selection_finalize);
}

void Selection_deselect(script::ContextHandle handle)
{
  script::Context ctx(handle);

  auto wrap = (MaskWrap*)ctx.toUserData(0, kTag);
  if (wrap) {
    if (wrap->sprite) {
      Doc* doc = wrap->sprite->document();
      ASSERT(doc);
      wrap->sprite->transaction().execute(
        new cmd::DeselectMask(doc));
    }
    else {
      ASSERT(wrap->mask);
      wrap->mask->clear();
    }
  }

  ctx.pushUndefined();
}

void Selection_select(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (MaskWrap*)ctx.toUserData(0, kTag);
  gfx::Rect bounds = convert_args_into_rectangle(ctx);

  if (wrap) {
    if (bounds.isEmpty()) {
      Selection_deselect(handle);
      return;
    }
    else {
      if (wrap->sprite) {
        Mask newMask;
        newMask.replace(bounds);
        wrap->sprite->transaction().execute(
          new cmd::SetMask(wrap->sprite->document(), &newMask));
      }
      else {
        wrap->mask->replace(bounds);
      }
    }
  }

  ctx.pushUndefined();
}

void Selection_selectAll(script::ContextHandle handle)
{
  script::Context ctx(handle);

  auto wrap = (MaskWrap*)ctx.toUserData(0, kTag);
  if (wrap) {
    if (wrap->sprite) {
      Doc* doc = wrap->sprite->document();

      Mask newMask;
      newMask.replace(doc->sprite()->bounds());

      wrap->sprite->transaction().execute(
        new cmd::SetMask(doc, &newMask));
    }
    else {
      gfx::Rect bounds = wrap->mask->bounds();
      if (!bounds.isEmpty())
        wrap->mask->replace(bounds);
    }
  }

  ctx.pushUndefined();
}

void Selection_get_bounds(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto wrap = (MaskWrap*)ctx.toUserData(0, kTag);
  if (wrap) {
    if (wrap->sprite) {
      Doc* doc = wrap->sprite->document();
      if (doc->isMaskVisible())
        push_new_rectangle(ctx, doc->mask()->bounds());
      else                        // Empty rectangle
        push_new_rectangle(ctx, gfx::Rect(0, 0, 0, 0));
    }
    else {
      push_new_rectangle(ctx, wrap->mask->bounds());
    }
  }
  else
    ctx.pushUndefined();
}

const script::FunctionEntry Selection_methods[] = {
  { "deselect", Selection_deselect, 1 },
  { "select", Selection_select, 4 },
  { "selectAll", Selection_selectAll, 0 },
  { nullptr, nullptr, 0 }
};

const script::PropertyEntry Selection_props[] = {
  { "bounds", Selection_get_bounds, nullptr },
  { nullptr, nullptr, 0 }
};

} // anonymous namespace

void register_selection_class(script::index_t idx, script::Context& ctx)
{
  ctx.registerClass(idx, kTag,
                    Selection_new, 3,
                    Selection_methods,
                    Selection_props);
}

void push_new_selection(script::Context& ctx, SpriteWrap* spriteWrap)
{
  ctx.newObject(kTag, new MaskWrap(nullptr, spriteWrap), Selection_finalize);
}

} // namespace app
