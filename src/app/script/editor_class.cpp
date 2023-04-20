// Aseprite
// Copyright (c) 2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/console.h"
#include "app/script/docobj.h"
#include "app/script/luacpp.h"
#include "app/script/registry.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/select_box_state.h"

#ifdef ENABLE_UI

namespace app {
namespace script {

using namespace doc;

namespace {

// ui::Property attached to an app::Editor to re-use same EditorObj
// instances for the same app::Editor.
class LuaValueProperty : public ui::Property {
public:
  static constexpr const char* Name = "LuaValue";

  LuaValueProperty(lua_State* L)
    : Property(Name)
    , L(L) {
    // The LuaValueProperty() ctor pop a value from the Lua stack and
    // store that reference in the registry for the future.
    m_ref.ref(L);
  }

  ~LuaValueProperty() {
    m_ref.unref(L);
  }

  RegistryRef& ref() { return m_ref; }

private:
  lua_State* L;
  RegistryRef m_ref;
};

// Information used when the app.editor:askPoint() API is called.
struct AskPoint {
  lua_State* L;
  std::string title;
  RegistryRef onclick;
  RegistryRef oncancel;

  AskPoint(lua_State* L) : L(L) { }
  ~AskPoint() {
    onclick.unref(L);
    oncancel.unref(L);
  }
};

class EditorObj : public EditorObserver,
                  public SelectBoxDelegate {
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

  void gc(lua_State*) {
    m_askPoint.reset();
    removeEditor();
  }

  Editor* editor() const { return m_editor; }

  void askPoint(lua_State* L,
                const std::string& title,
                RegistryRef&& onclick,
                RegistryRef&& oncancel) {
    // Cancel previous askPoint()
    if (m_askPoint) {
      onQuickboxCancel(m_editor);
      ASSERT(!m_askPoint);
    }

    m_askPoint = std::make_unique<AskPoint>(L);
    m_askPoint->title = title;
    m_askPoint->onclick = std::move(onclick);
    m_askPoint->oncancel = std::move(oncancel);

    m_editor->setState(
      std::make_shared<SelectBoxState>(
        this, m_editor->sprite()->bounds(),
        SelectBoxState::Flags(int(SelectBoxState::Flags::Rulers) |
                              int(SelectBoxState::Flags::DarkOutside) |
                              int(SelectBoxState::Flags::QuickBox) |
                              int(SelectBoxState::Flags::QuickPoint))));
  }

  // EditorObserver impl
  void onDestroyEditor(Editor* editor) override {
    ASSERT(editor == m_editor);
    removeEditor();
  }

  // SelectBoxDelegate impl
  void onQuickboxEnd(Editor* editor, const gfx::Rect& rect, ui::MouseButton button) override {
    lua_State* L = m_askPoint->L;

    if (m_askPoint && m_askPoint->onclick.get(L)) {
      lua_newtable(L);       // Create "ev" argument with ev.point
      push_obj(L, rect.origin());
      lua_setfield(L, -2, "point");
      if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        if (const char* s = lua_tostring(L, -1))
          Console().printf("%s\n", s);
      }
    }
    m_askPoint.reset();
    editor->backToPreviousState();
  }

  void onQuickboxCancel(Editor* editor) override {
    lua_State* L = m_askPoint->L;

    if (m_askPoint && m_askPoint->oncancel.get(L)) {
      lua_newtable(L);       // Create empty "ev" table as argument
      if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
        if (const char* s = lua_tostring(L, -1))
          Console().printf("%s\n", s);
      }
    }
    m_askPoint.reset();
    editor->backToPreviousState();
  }

  std::string onGetContextBarHelp() override {
    return (m_askPoint ? m_askPoint->title: std::string());
  }

private:
  void removeEditor() {
    if (m_editor) {
      m_editor->removeProperty(LuaValueProperty::Name);
      m_editor->remove_observer(this);
      m_editor = nullptr;
    }
  }

  Editor* m_editor = nullptr;
  std::unique_ptr<AskPoint> m_askPoint;
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

int Editor_askPoint(lua_State* L)
{
  auto obj = get_obj<EditorObj>(L, 1);
  if (!lua_istable(L, 2))
    return luaL_error(L, "app.askPoint() must be called with a table as its first argument");

  std::string title;

  int type = lua_getfield(L, 2, "title");
  if (type != LUA_TNIL)
    title = lua_tostring(L, -1);
  lua_pop(L, 1);

  RegistryRef onclick;
  type = lua_getfield(L, 2, "onclick");
  if (type == LUA_TFUNCTION)
    onclick.ref(L);
  else
    lua_pop(L, 1);

  RegistryRef oncancel;
  type = lua_getfield(L, 2, "oncancel");
  if (type == LUA_TFUNCTION)
    oncancel.ref(L);
  else
    lua_pop(L, 1);

  obj->askPoint(L, title, std::move(onclick), std::move(oncancel));
  return 0;
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
  { "askPoint", Editor_askPoint },
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
  if (editor->hasProperty(LuaValueProperty::Name)) {
    std::shared_ptr<LuaValueProperty> prop =
      std::static_pointer_cast<LuaValueProperty>(
        editor->getProperty(LuaValueProperty::Name));
    prop->ref().get(L);
  }
  else {
    push_new<EditorObj>(L, editor);

    lua_pushvalue(L, -1);
    editor->setProperty(std::make_shared<LuaValueProperty>(L));
  }
}

} // namespace script
} // namespace app

#endif  // ENABLE_UI
