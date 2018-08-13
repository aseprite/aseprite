// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_SELECT_ACCELERATOR_H_INCLUDED
#define APP_UI_SELECT_ACCELERATOR_H_INCLUDED
#pragma once

#include "app/ui/key_context.h"
#include "ui/accelerator.h"
#include "ui/tooltips.h"

#include "select_accelerator.xml.h"

namespace app {
  class KeyboardShortcuts;

  class SelectAccelerator : public app::gen::SelectAccelerator {
  public:
    SelectAccelerator(const ui::Accelerator& accelerator,
                      const KeyContext keyContext,
                      const KeyboardShortcuts& currentKeys);

    bool isOK() const { return m_ok; }
    bool isModified() const { return m_modified; }
    const ui::Accelerator& accel() const { return m_accel; }

  private:
    void onModifierChange(ui::KeyModifiers modifier, ui::CheckBox* checkbox);
    void onAccelChange(const ui::Accelerator* accel);
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
    ui::Accelerator m_origAccel;
    ui::Accelerator m_accel;
    bool m_ok;
    bool m_modified;
  };

} // namespace app

#endif
