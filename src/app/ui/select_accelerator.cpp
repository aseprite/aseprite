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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/select_accelerator.h"

#include "app/ui/keyboard_shortcuts.h"
#include "base/bind.h"
#include "base/signal.h"

#include <cctype>

namespace app {

using namespace ui;

class SelectAccelerator::KeyField : public ui::Entry {
public:
  KeyField(const Accelerator& accel) : ui::Entry(256, "") {
    setFocusMagnet(true);
    setAccel(accel);
  }

  void setAccel(const Accelerator& accel) {
    m_accel = accel;
    updateText();
  }

  Signal1<void, const ui::Accelerator*> AccelChange;

protected:
  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {
      case kKeyDownMessage:
        if (hasFocus() && !isReadOnly()) {
          KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
          KeyModifiers modifiers = keymsg->keyModifiers();

          if (keymsg->scancode() == kKeySpace)
            modifiers = (KeyModifiers)(modifiers & ~kKeySpaceModifier);

          m_accel = Accelerator(
            modifiers,
            keymsg->scancode(),
            keymsg->unicodeChar() > 32 ?
              std::tolower(keymsg->unicodeChar()): 0);
          updateText();

          AccelChange(&m_accel);
          return true;
        }
        break;
    }
    return Entry::onProcessMessage(msg);
  }

  void updateText() {
    setText(
      Accelerator(
        kKeyNoneModifier,
        m_accel.scancode(),
        m_accel.unicodeChar()).toString().c_str());
  }

  Accelerator m_accel;
};

SelectAccelerator::SelectAccelerator(const ui::Accelerator& accel)
  : m_keyField(new KeyField(accel))
  , m_accel(accel)
  , m_modified(false)
{
  updateModifiers();
  updateAssignedTo();

  keyPlaceholder()->addChild(m_keyField);

  alt()->Click.connect(Bind<void>(&SelectAccelerator::onModifierChange, this, kKeyAltModifier, alt()));
  cmd()->Click.connect(Bind<void>(&SelectAccelerator::onModifierChange, this, kKeyCmdModifier, cmd()));
  ctrl()->Click.connect(Bind<void>(&SelectAccelerator::onModifierChange, this, kKeyCtrlModifier, ctrl()));
  shift()->Click.connect(Bind<void>(&SelectAccelerator::onModifierChange, this, kKeyShiftModifier, shift()));
  space()->Click.connect(Bind<void>(&SelectAccelerator::onModifierChange, this, kKeySpaceModifier, space()));

  m_keyField->AccelChange.connect(&SelectAccelerator::onAccelChange, this);
  clearButton()->Click.connect(Bind<void>(&SelectAccelerator::onClear, this));
  okButton()->Click.connect(Bind<void>(&SelectAccelerator::onOK, this));
  cancelButton()->Click.connect(Bind<void>(&SelectAccelerator::onCancel, this));
}

void SelectAccelerator::onModifierChange(KeyModifiers modifier, CheckBox* checkbox)
{
  bool state = (checkbox->isSelected());
  KeyModifiers modifiers = m_accel.modifiers();
  KeyScancode scancode = m_accel.scancode();
  int unicodeChar = m_accel.unicodeChar();

  modifiers = (KeyModifiers)((modifiers & ~modifier) | (state ? modifier : 0));
  if (modifiers == kKeySpaceModifier && scancode == kKeySpace)
    modifiers = kKeyNoneModifier;

  m_accel = Accelerator(modifiers, scancode, unicodeChar);

  m_keyField->setAccel(m_accel);
  updateAssignedTo();
}

void SelectAccelerator::onAccelChange(const ui::Accelerator* accel)
{
  m_accel = *accel;
  updateModifiers();
  updateAssignedTo();
}

void SelectAccelerator::onClear()
{
  m_accel = Accelerator(kKeyNoneModifier, kKeyNil, 0);

  m_keyField->setAccel(m_accel);
  updateModifiers();
  updateAssignedTo();

  m_keyField->requestFocus();
}

void SelectAccelerator::onOK()
{
  m_modified = (m_origAccel != m_accel);
  closeWindow(NULL);
}

void SelectAccelerator::onCancel()
{
  closeWindow(NULL);
}

void SelectAccelerator::updateModifiers()
{
  alt()->setSelected(m_accel.modifiers() & kKeyAltModifier ? true: false);
  ctrl()->setSelected(m_accel.modifiers() & kKeyCtrlModifier ? true: false);
  shift()->setSelected(m_accel.modifiers() & kKeyShiftModifier ? true: false);
  space()->setSelected(m_accel.modifiers() & kKeySpaceModifier ? true: false);
#if __APPLE__
  cmd()->setSelected(m_accel.modifiers() & kKeyCmdModifier ? true: false);
#else
  cmd()->setVisible(false);
#endif
}

void SelectAccelerator::updateAssignedTo()
{
  std::string res = "None";

  for (Key* key : *KeyboardShortcuts::instance()) {
    if (key->hasAccel(m_accel)) {
      res = key->triggerString();
      break;
    }
  }

  assignedTo()->setText("Assigned to: " + res);
}

} // namespace app
