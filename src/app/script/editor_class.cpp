// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/script/docobj.h"
#include "app/script/luacpp.h"
#include "app/ui/editor/editor.h"

#ifdef ENABLE_UI

namespace app {
namespace script {

using namespace doc;

namespace {

class EditorObj : public EditorObserver {
public:
  EditorObj(Editor* editor) : m_editor(editor) {
    if (m_editor)
      m_editor->add_observer(this);
  }

  EditorObj(const EditorObj&) = delete;
  EditorObj& operator=(const EditorObj&) = delete;

  ~EditorObj() {
    ASSERT(!m_editor);
  }

  void gc(lua_State* L) {
    removeEditor();
  }

  Editor* editor() const { return m_editor; }

  // EditorObserver
  void onDestroyEditor(Editor* editor) override {
    ASSERT(editor == m_editor);
    removeEditor();
  }

private:
  void removeEditor() {
    if (m_editor) {
      m_editor->remove_observer(this);
      m_editor = nullptr;
    }
  }

  Editor* m_editor = nullptr;
};

int Editor_gc(lua_State* L)
{
  auto obj = get_obj<EditorObj>(L, 1);
  obj->gc(L);
  obj->~EditorObj();
  return 0;
}

int Editor_eq(lua_State* L)
{
  auto a = get_obj<EditorObj>(L, 1);
  auto b = get_obj<EditorObj>(L, 2);
  lua_pushboolean(L, a->editor() == b->editor());
  return 1;
}

int Editor_get_sprite(lua_State* L)
{
  auto obj = get_obj<EditorObj>(L, 1);
  push_docobj(L, obj->editor()->sprite());
  return 1;
}

const luaL_Reg Editor_methods[] = {
  { "__gc", Editor_gc },
  { "__eq", Editor_eq },
  { nullptr, nullptr }
};

const Property Editor_properties[] = {
  { "sprite", Editor_get_sprite, nullptr },
  { nullptr, nullptr, nullptr }
};

} // anonymous namespace

DEF_MTNAME(EditorObj);

void register_editor_class(lua_State* L)
{
  using Editor = EditorObj;
  REG_CLASS(L, Editor);
  REG_CLASS_PROPERTIES(L, Editor);
}

void push_editor(lua_State* L, Editor* editor)
{
  push_new<EditorObj>(L, editor);
}

} // namespace script
} // namespace app

#endif  // ENABLE_UI
