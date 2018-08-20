// Aseprite
// Copyright (C) 2015-2018  David Capello
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
#include "app/transaction.h"
#include "app/tx.h"
#include "doc/mask.h"
#include "script/engine.h"

namespace app {

using namespace doc;

namespace {

const char* kTag = "Selection";

struct SelectionObject {
  Mask* mask;
  Sprite* sprite;
  SelectionObject(Mask* mask, Sprite* sprite)
    : mask(mask)
    , sprite(sprite) {
  }
  ~SelectionObject() {
    if (!sprite && mask)
      delete mask;
  }
};

void Selection_finalize(script::ContextHandle handle, void* data)
{
  auto obj = (SelectionObject*)data;
  delete obj;
}

void Selection_new(script::ContextHandle handle)
{
  script::Context ctx(handle);
  ctx.newObject(kTag, new SelectionObject(new Mask, nullptr), Selection_finalize);
}

void Selection_deselect(script::ContextHandle handle)
{
  script::Context ctx(handle);

  auto obj = (SelectionObject*)ctx.toUserData(0, kTag);
  if (obj) {
    if (obj->sprite) {
      Doc* doc = static_cast<Doc*>(obj->sprite->document());
      ASSERT(doc);

      Tx tx;
      tx(new cmd::DeselectMask(doc));
      tx.commit();
    }
    else {
      ASSERT(obj->mask);
      obj->mask->clear();
    }
  }
  ctx.pushUndefined();
}

void Selection_select(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto obj = (SelectionObject*)ctx.toUserData(0, kTag);
  gfx::Rect bounds = convert_args_into_rectangle(ctx);

  if (obj) {
    if (bounds.isEmpty()) {
      Selection_deselect(handle);
      return;
    }
    else {
      if (obj->sprite) {
        Doc* doc = static_cast<Doc*>(obj->sprite->document());
        ASSERT(doc);

        Mask newMask;
        newMask.replace(bounds);

        Tx tx;
        tx(new cmd::SetMask(doc, &newMask));
        tx.commit();
      }
      else {
        obj->mask->replace(bounds);
      }
    }
  }
  ctx.pushUndefined();
}

void Selection_selectAll(script::ContextHandle handle)
{
  script::Context ctx(handle);

  auto obj = (SelectionObject*)ctx.toUserData(0, kTag);
  if (obj) {
    if (obj->sprite) {
      Doc* doc = static_cast<Doc*>(obj->sprite->document());

      Mask newMask;
      newMask.replace(obj->sprite->bounds());

      Tx tx;
      tx(new cmd::SetMask(doc, &newMask));
      tx.commit();
    }
    else {
      gfx::Rect bounds = obj->mask->bounds();
      if (!bounds.isEmpty())
        obj->mask->replace(bounds);
    }
  }
  ctx.pushUndefined();
}

void Selection_get_bounds(script::ContextHandle handle)
{
  script::Context ctx(handle);
  auto obj = (SelectionObject*)ctx.toUserData(0, kTag);
  if (obj) {
    if (obj->sprite) {
      Doc* doc = static_cast<Doc*>(obj->sprite->document());
      if (doc->isMaskVisible()) {
        push_new_rectangle(ctx, doc->mask()->bounds());
      }
      else {                        // Empty rectangle
        push_new_rectangle(ctx, gfx::Rect(0, 0, 0, 0));
      }
    }
    else {
      push_new_rectangle(ctx, obj->mask->bounds());
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

void push_sprite_selection(script::Context& ctx, Sprite* sprite)
{
  ctx.newObject(kTag,
                new SelectionObject(nullptr, sprite),
                Selection_finalize);
}

} // namespace app
