// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SELECT_KEYSHORTCUT_H_INCLUDED
#define APP_UI_SELECT_KEYSHORTCUT_H_INCLUDED
#pragma once

#include "app/ui/key_context.h"
#include "ui/keyshortcut.h"
#include "ui/tooltips.h"

#include "select_keyshortcut.xml.h"

namespace app {
  class KeyboardShortcuts;

  class SelectKeyShortcut : public app::gen::SelectKeyShortcut {
  public:
    SelectKeyShortcut(const ui::KeyShortcut& keyshortcut,
                      const KeyContext keyContext,
                      const KeyboardShortcuts& currentKeys);

    bool isOK() const { return m_ok; }
    bool isModified() const { return m_modified; }
    const ui::KeyShortcut& keyshortcut() const { return m_keyshortcut; }

  private:
    void onModifierChange(ui::KeyModifiers modifier, ui::CheckBox* checkbox);
    void onKeyShortcutChange(const ui::KeyShortcut* keyshortcut);
    void onClear();
    void onOK();
    void onCancel();
    void updateModifiers();
    void updateAssignedTo();

    class KeyField;

    ui::TooltipManager m_tooltipManager;
    KeyField* m_keyField;
    KeyContext m_keyContext;
    const KeyboardShortcuts& m_currentKeys;
    ui::KeyShortcut m_origKeyShortcut;
    ui::KeyShortcut m_keyshortcut;
    bool m_ok;
    bool m_modified;
  };

} // namespace app

#endif
