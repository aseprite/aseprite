/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef APP_UI_SELECT_ACCELERATOR_H_INCLUDED
#define APP_UI_SELECT_ACCELERATOR_H_INCLUDED
#pragma once

#include "app/ui/keyboard_shortcuts.h"
#include "ui/accelerator.h"

#include "generated_select_accelerator.h"

namespace app {

  class SelectAccelerator : public app::gen::SelectAccelerator {
  public:
    SelectAccelerator(const ui::Accelerator& accelerator, KeyContext keyContext);

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

    KeyField* m_keyField;
    KeyContext m_keyContext;
    ui::Accelerator m_origAccel;
    ui::Accelerator m_accel;
    bool m_modified;
  };

} // namespace app

#endif
