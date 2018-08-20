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

struct SelectionObject {
  Mask* mask;
  Sprite* sprite;
  SelectionObject(Mask* mask, Sprite* sprite)
    : mask(mask)
    , sprite(sprite) {
  }
  SelectionObject(SelectionObject&& that)
    : mask(that.mask)
    , sprite(that.sprite) {
    that.mask = nullptr;
    that.sprite = nullptr;
  }
  ~SelectionObject() {
    if (!sprite && mask)
      delete mask;
  }
  SelectionObject(const SelectionObject&) = delete;
  SelectionObject& operator=(const SelectionObject&) = delete;
};

int Selection_new(lua_State* L)
{
  push_obj(L, SelectionObject(new Mask, nullptr));
  return 1;
}

int Selection_gc(lua_State* L)
{
  get_obj<SelectionObject>(L, 1)->~SelectionObject();
  return 0;
}

int Selection_deselect(lua_State* L)
{
  auto obj = get_obj<SelectionObject>(L, 1);
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
  return 0;
}

int Selection_select(lua_State* L)
{
  gfx::Rect bounds = convert_args_into_rect(L, 2);
  if (bounds.isEmpty())
    return Selection_deselect(L);

  auto obj = get_obj<SelectionObject>(L, 1);
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
  return 0;
}

int Selection_selectAll(lua_State* L)
{
  auto obj = get_obj<SelectionObject>(L, 1);
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

int Selection_get_bounds(lua_State* L)
{
  auto obj = get_obj<SelectionObject>(L, 1);
  if (obj->sprite) {
    Doc* doc = static_cast<Doc*>(obj->sprite->document());
    if (doc->isMaskVisible()) {
      push_obj(L, doc->mask()->bounds());
    }
    else {   // Empty rectangle
      push_obj(L, gfx::Rect(0, 0, 0, 0));
    }
  }
  else {
    push_obj(L, obj->mask->bounds());
  }
  return 1;
}

const luaL_Reg Selection_methods[] = {
  { "deselect",  Selection_deselect },
  { "select",    Selection_select },
  { "selectAll", Selection_selectAll },
  { "__gc",      Selection_gc },
  { nullptr,     nullptr }
};

const Property Selection_properties[] = {
  { "bounds", Selection_get_bounds, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(SelectionObject);

void register_selection_class(lua_State* L)
{
  using Selection = SelectionObject;
  REG_CLASS(L, Selection);
  REG_CLASS_NEW(L, Selection);
  REG_CLASS_PROPERTIES(L, Selection);
}

void push_sprite_selection(lua_State* L, Sprite* sprite)
{
  push_obj(L, SelectionObject(nullptr, sprite));
}

} // namespace script
} // namespace app
