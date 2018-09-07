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
#include "app/script/engine.h"
#include "app/script/luacpp.h"
#include "app/transaction.h"
#include "app/tx.h"
#include "doc/mask.h"

namespace app {
namespace script {

using namespace doc;

namespace {

struct SelectionObj {
  Mask* mask;
  Sprite* sprite;
  SelectionObj(Mask* mask, Sprite* sprite)
    : mask(mask)
    , sprite(sprite) {
  }
  ~SelectionObj() {
    if (!sprite && mask)
      delete mask;
  }
  SelectionObj(const SelectionObj&) = delete;
  SelectionObj& operator=(const SelectionObj&) = delete;
};

int Selection_new(lua_State* L)
{
  push_new<SelectionObj>(L, new Mask, nullptr);
  return 1;
}

int Selection_gc(lua_State* L)
{
  get_obj<SelectionObj>(L, 1)->~SelectionObj();
  return 0;
}

int Selection_eq(lua_State* L)
{
  auto a = get_obj<SelectionObj>(L, 1);
  auto b = get_obj<SelectionObj>(L, 2);
  const bool result =
    (a->mask->isEmpty() && b->mask->isEmpty()) ||
    (!a->mask->isEmpty() && !b->mask->isEmpty() &&
     count_diff_between_images(a->mask->bitmap(), b->mask->bitmap()) == 0);
  lua_pushboolean(L, result);
  return 1;
}

int Selection_deselect(lua_State* L)
{
  auto obj = get_obj<SelectionObj>(L, 1);
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
  return 0;
}

template<typename T>
void replace(Mask& dst, const Mask& a, const T& b) {
  if (b.isEmpty())
    dst.clear();
  else
    dst.replace(b);
}

template<typename T>
void add(Mask& dst, const Mask& a, const T& b) {
  if (&dst != &a)
    dst.replace(a);
  if (!b.isEmpty())
    dst.add(b);
}

template<typename T>
void subtract(Mask& dst, const Mask& a, const T& b) {
  if (&dst != &a)
    dst.replace(a);
  if (!b.isEmpty())
    dst.subtract(b);
}

template<typename T>
void intersect(Mask& dst, const Mask& a, const T& b) {
  if (b.isEmpty())
    dst.clear();
  else {
    if (&dst != &a)
      dst.replace(a);
    dst.intersect(b);
  }
}

template<typename OpMask, typename OpRect>
int Selection_op(lua_State* L, OpMask opMask, OpRect opRect)
{
  auto obj = get_obj<SelectionObj>(L, 1);
  auto otherObj = may_get_obj<SelectionObj>(L, 2);
  if (otherObj) {
    if (obj->sprite) {
      Doc* doc = static_cast<Doc*>(obj->sprite->document());
      ASSERT(doc);

      Mask newMask;
      opMask(newMask, *obj->mask, *otherObj->mask);

      Tx tx;
      tx(new cmd::SetMask(doc, &newMask));
      tx.commit();
    }
    else {
      opMask(*obj->mask, *obj->mask, *otherObj->mask);
    }
  }
  // Try with a rectangle
  else {
    gfx::Rect bounds = convert_args_into_rect(L, 2);
    if (obj->sprite) {
      Doc* doc = static_cast<Doc*>(obj->sprite->document());
      ASSERT(doc);

      Mask newMask;
      opRect(newMask, *obj->mask, bounds);

      Tx tx;
      tx(new cmd::SetMask(doc, &newMask));
      tx.commit();
    }
    else {
      opRect(*obj->mask, *obj->mask, bounds);
    }
  }
  return 0;
}

int Selection_select(lua_State* L) { return Selection_op(L, replace<Mask>, replace<gfx::Rect>); }
int Selection_add(lua_State* L) { return Selection_op(L, add<Mask>, add<gfx::Rect>); }
int Selection_subtract(lua_State* L) { return Selection_op(L, subtract<Mask>, subtract<gfx::Rect>); }
int Selection_intersect(lua_State* L) { return Selection_op(L, intersect<Mask>, intersect<gfx::Rect>); }

int Selection_selectAll(lua_State* L)
{
  auto obj = get_obj<SelectionObj>(L, 1);
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
  return 0;
}

int Selection_contains(lua_State* L)
{
  const auto obj = get_obj<SelectionObj>(L, 1);
  auto pt = convert_args_into_point(L, 2);
  lua_pushboolean(L, obj->mask->containsPoint(pt.x, pt.y));
  return 1;
}

int Selection_get_bounds(lua_State* L)
{
  const auto obj = get_obj<SelectionObj>(L, 1);
  if (obj->sprite) {
    Doc* doc = static_cast<Doc*>(obj->sprite->document());
    if (doc->isMaskVisible()) {
      push_obj(L, doc->mask()->bounds());
    }
    else {   // Empty rectangle
      push_new<gfx::Rect>(L, 0, 0, 0, 0);
    }
  }
  else {
    push_obj(L, obj->mask->bounds());
  }
  return 1;
}

int Selection_get_isEmpty(lua_State* L)
{
  const auto obj = get_obj<SelectionObj>(L, 1);
  lua_pushboolean(L, obj->mask->isEmpty());
  return 1;
}

const luaL_Reg Selection_methods[] = {
  { "deselect", Selection_deselect },
  { "select", Selection_select },
  { "selectAll", Selection_selectAll },
  { "add", Selection_add },
  { "subtract", Selection_subtract },
  { "intersect", Selection_intersect },
  { "contains", Selection_contains },
  { "__gc", Selection_gc },
  { "__eq", Selection_eq },
  { nullptr, nullptr }
};

const Property Selection_properties[] = {
  { "bounds", Selection_get_bounds, nullptr },
  { "isEmpty", Selection_get_isEmpty, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(SelectionObj);

void register_selection_class(lua_State* L)
{
  using Selection = SelectionObj;
  REG_CLASS(L, Selection);
  REG_CLASS_NEW(L, Selection);
  REG_CLASS_PROPERTIES(L, Selection);
}

void push_sprite_selection(lua_State* L, Sprite* sprite)
{
  push_new<SelectionObj>(L, nullptr, sprite);
}

} // namespace script
} // namespace app
