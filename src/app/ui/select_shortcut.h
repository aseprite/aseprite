// Aseprite
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SELECT_SHORTCUT_H_INCLUDED
#define APP_UI_SELECT_SHORTCUT_H_INCLUDED
#pragma once

#include "app/ui/key_context.h"
#include "ui/shortcut.h"
#include "ui/tooltips.h"

#include "select_shortcut.xml.h"

namespace app {
class KeyboardShortcuts;

class SelectShortcut : public app::gen::SelectShortcut {
public:
  SelectShortcut(const ui::Shortcut& shortcut,
                 KeyContext keyContext,
                 const KeyboardShortcuts& currentKeys);

  bool isOK() const { return m_ok; }
  bool isModified() const { return m_modified; }
  const ui::Shortcut& shortcut() const { return m_shortcut; }

private:
  void onModifierChange(ui::KeyModifiers modifier, ui::CheckBox* checkbox);
  void onShortcutChange(const ui::Shortcut* shortcut);
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
  ui::Shortcut m_origShortcut;
  ui::Shortcut m_shortcut;
  bool m_ok;
  bool m_modified;
};

} // namespace app

#endif
