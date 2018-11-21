// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
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
#include "app/script/docobj.h"
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
  ObjectId maskId;
  ObjectId spriteId;
  SelectionObj(Mask* mask, Sprite* sprite)
    : maskId(mask ? mask->id(): 0)
    , spriteId(sprite ? sprite->id(): 0) {
  }
  SelectionObj(const SelectionObj&) = delete;
  SelectionObj& operator=(const SelectionObj&) = delete;

  ~SelectionObj() {
    ASSERT(!maskId);
  }

  void gc(lua_State* L) {
    if (!spriteId)
      delete this->mask(L);
    maskId = 0;
  }

  Mask* mask(lua_State* L) {
    if (maskId)
      return check_docobj(L, doc::get<Mask>(maskId));
    else {
      auto doc = static_cast<Doc*>(sprite(L)->document());
      ASSERT(doc);

      // The selection might be invisible but has something (e.g. when
      // the user calls "Select > Deselect" option). In this case we
      // want to show to the script an empty selection, so we'll clear
      // the invisible selection so the script sees it empty.
      //
      // This breaks the "Select > Reselect" command, but it looks
      // like the expected behavior for script authors.
      if (!doc->isMaskVisible())
        doc->mask()->clear();

      return doc->mask();
    }
  }
  Sprite* sprite(lua_State* L) {
    if (spriteId)
      return check_docobj(L, doc::get<Sprite>(spriteId));
    else
      return nullptr;
  }
};

int Selection_new(lua_State* L)
{
  gfx::Rect bounds = convert_args_into_rect(L, 1);

  auto mask = new Mask;
  if (!bounds.isEmpty())
    mask->replace(bounds);

  push_new<SelectionObj>(L, mask, nullptr);
  return 1;
}

int Selection_gc(lua_State* L)
{
  auto obj = get_obj<SelectionObj>(L, 1);
  obj->gc(L);
  obj->~SelectionObj();
  return 0;
}

int Selection_eq(lua_State* L)
{
  auto a = get_obj<SelectionObj>(L, 1);
  auto b = get_obj<SelectionObj>(L, 2);
  auto aMask = a->mask(L);
  auto bMask = b->mask(L);
  const bool result =
    (aMask->isEmpty() && bMask->isEmpty()) ||
    (!aMask->isEmpty() && !bMask->isEmpty() &&
     is_same_image(aMask->bitmap(), bMask->bitmap()));
  lua_pushboolean(L, result);
  return 1;
}

int Selection_deselect(lua_State* L)
{
  auto obj = get_obj<SelectionObj>(L, 1);
  if (auto sprite = obj->sprite(L)) {
    Doc* doc = static_cast<Doc*>(sprite->document());
    ASSERT(doc);

    Tx tx;
    tx(new cmd::DeselectMask(doc));
    tx.commit();
  }
  else {
    auto mask = obj->mask(L);
    mask->clear();
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
  auto mask = obj->mask(L);
  auto sprite = obj->sprite(L);
  auto otherObj = may_get_obj<SelectionObj>(L, 2);
  if (otherObj) {
    auto otherMask = otherObj->mask(L);
    if (sprite) {
      Doc* doc = static_cast<Doc*>(sprite->document());
      ASSERT(doc);

      Mask newMask;
      opMask(newMask, *mask, *otherMask);

      Tx tx;
      tx(new cmd::SetMask(doc, &newMask));
      tx.commit();
    }
    else {
      opMask(*mask, *mask, *otherMask);
    }
  }
  // Try with a rectangle
  else {
    gfx::Rect bounds = convert_args_into_rect(L, 2);
    if (sprite) {
      Doc* doc = static_cast<Doc*>(sprite->document());
      ASSERT(doc);

      Mask newMask;
      opRect(newMask, *mask, bounds);

      Tx tx;
      tx(new cmd::SetMask(doc, &newMask));
      tx.commit();
    }
    else {
      opRect(*mask, *mask, bounds);
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
  if (auto sprite = obj->sprite(L)) {
    Doc* doc = static_cast<Doc*>(sprite->document());

    Mask newMask;
    newMask.replace(sprite->bounds());

    Tx tx;
    tx(new cmd::SetMask(doc, &newMask));
    tx.commit();
  }
  else {
    auto mask = obj->mask(L);
    gfx::Rect bounds = mask->bounds();
    if (!bounds.isEmpty())
      mask->replace(bounds);
  }
  return 0;
}

int Selection_contains(lua_State* L)
{
  const auto obj = get_obj<SelectionObj>(L, 1);
  const auto mask = obj->mask(L);
  auto pt = convert_args_into_point(L, 2);
  lua_pushboolean(L, mask->containsPoint(pt.x, pt.y));
  return 1;
}

int Selection_get_bounds(lua_State* L)
{
  const auto obj = get_obj<SelectionObj>(L, 1);
  if (auto sprite = obj->sprite(L)) {
    Doc* doc = static_cast<Doc*>(sprite->document());
    if (doc->isMaskVisible()) {
      push_obj(L, doc->mask()->bounds());
    }
    else {   // Empty rectangle
      push_new<gfx::Rect>(L, 0, 0, 0, 0);
    }
  }
  else {
    const auto mask = obj->mask(L);
    push_obj(L, mask->bounds());
  }
  return 1;
}

int Selection_get_isEmpty(lua_State* L)
{
  const auto obj = get_obj<SelectionObj>(L, 1);
  const auto mask = obj->mask(L);
  lua_pushboolean(L, mask->isEmpty());
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
DEF_MTNAME_ALIAS(SelectionObj, Mask);

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
