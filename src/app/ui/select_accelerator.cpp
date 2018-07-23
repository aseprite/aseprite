// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/select_accelerator.h"

#include "app/ui/key.h"
#include "app/ui/keyboard_shortcuts.h"
#include "base/bind.h"
#include "obs/signal.h"
#include "ui/entry.h"
#include "ui/message.h"

#include <cctype>

namespace app {

using namespace ui;

class SelectAccelerator::KeyField : public ui::Entry {
public:
  KeyField(const Accelerator& accel) : ui::Entry(256, "") {
    setTranslateDeadKeys(false);
    setExpansive(true);
    setFocusMagnet(true);
    setAccel(accel);
  }

  void setAccel(const Accelerator& accel) {
    m_accel = accel;
    updateText();
  }

  obs::signal<void(const ui::Accelerator*)> AccelChange;

protected:
  bool onProcessMessage(Message* msg) override {
    switch (msg->type()) {

      case kKeyDownMessage:
        if (hasFocus() && !isReadOnly()) {
          KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
          if (!keymsg->scancode() && keymsg->unicodeChar() < 32)
            break;

          KeyModifiers modifiers = keymsg->modifiers();

          if (keymsg->scancode() == kKeySpace)
            modifiers = (KeyModifiers)(modifiers & ~kKeySpaceModifier);

          m_accel = Accelerator(
            modifiers,
            keymsg->scancode(),
            keymsg->unicodeChar() > 32 ?
              std::tolower(keymsg->unicodeChar()): 0);

          // Convert the accelerator to a string, and parse it
          // again. Just to obtain the exact accelerator we'll read
          // when we import the gui.xml file or an .aseprite-keys file.
          m_accel = Accelerator(m_accel.toString());

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

SelectAccelerator::SelectAccelerator(const ui::Accelerator& accel,
                                     const KeyContext keyContext,
                                     const KeyboardShortcuts& currentKeys)
  : m_keyField(new KeyField(accel))
  , m_keyContext(keyContext)
  , m_currentKeys(currentKeys)
  , m_accel(accel)
  , m_ok(false)
  , m_modified(false)
{
  updateModifiers();
  updateAssignedTo();

  keyPlaceholder()->addChild(m_keyField);

  alt()->Click.connect(base::Bind<void>(&SelectAccelerator::onModifierChange, this, kKeyAltModifier, alt()));
  cmd()->Click.connect(base::Bind<void>(&SelectAccelerator::onModifierChange, this, kKeyCmdModifier, cmd()));
  ctrl()->Click.connect(base::Bind<void>(&SelectAccelerator::onModifierChange, this, kKeyCtrlModifier, ctrl()));
  shift()->Click.connect(base::Bind<void>(&SelectAccelerator::onModifierChange, this, kKeyShiftModifier, shift()));
  space()->Click.connect(base::Bind<void>(&SelectAccelerator::onModifierChange, this, kKeySpaceModifier, space()));
  win()->Click.connect(base::Bind<void>(&SelectAccelerator::onModifierChange, this, kKeyWinModifier, win()));

  m_keyField->AccelChange.connect(&SelectAccelerator::onAccelChange, this);
  clearButton()->Click.connect(base::Bind<void>(&SelectAccelerator::onClear, this));
  okButton()->Click.connect(base::Bind<void>(&SelectAccelerator::onOK, this));
  cancelButton()->Click.connect(base::Bind<void>(&SelectAccelerator::onCancel, this));

  addChild(&m_tooltipManager);
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
  m_keyField->requestFocus();
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
  m_ok = true;
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
  win()->setVisible(false);
  cmd()->setSelected(m_accel.modifiers() & kKeyCmdModifier ? true: false);
#else
  #if __linux__
    win()->setText(kWinKeyName);
    m_tooltipManager.addTooltipFor(win(), "Also known as Windows key, logo key,\ncommand key, or system key.", TOP);
  #endif
  win()->setSelected(m_accel.modifiers() & kKeyWinModifier ? true: false);
  cmd()->setVisible(false);
#endif
}

void SelectAccelerator::updateAssignedTo()
{
  std::string res = "None";

  for (const KeyPtr& key : m_currentKeys) {
    if (key->keycontext() == m_keyContext &&
        key->hasAccel(m_accel)) {
      res = key->triggerString();
      break;
    }
  }

  assignedTo()->setText(res);
}

} // namespace app
