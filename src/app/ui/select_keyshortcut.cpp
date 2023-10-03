// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/select_keyshortcut.h"

#include "app/ui/key.h"
#include "app/ui/keyboard_shortcuts.h"
#include "obs/signal.h"
#include "ui/entry.h"
#include "ui/message.h"

#include <cctype>

namespace app {

using namespace ui;

class SelectKeyShortcut::KeyField : public ui::Entry {
public:
  KeyField(const KeyShortcut& keyshortcut) : ui::Entry(256, "") {
    setTranslateDeadKeys(false);
    setExpansive(true);
    setFocusMagnet(true);
    setKeyShortcut(keyshortcut);
  }

  void setKeyShortcut(const KeyShortcut& keyshortcut) {
    m_keyshortcut = keyshortcut;
    updateText();
  }

  obs::signal<void(const ui::KeyShortcut*)> KeyShortcutChange;

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

          m_keyshortcut = KeyShortcut(
            modifiers,
            keymsg->scancode(),
            keymsg->unicodeChar() > 32 ?
              std::tolower(keymsg->unicodeChar()): 0);

          // Convert the key shortcut to a string, and parse it
          // again. Just to obtain the exact key shortcut we'll read
          // when we import the gui.xml file or an .aseprite-keys file.
          m_keyshortcut = KeyShortcut(m_keyshortcut.toString());

          updateText();

          KeyShortcutChange(&m_keyshortcut);
          return true;
        }
        break;
    }
    return Entry::onProcessMessage(msg);
  }

  void updateText() {
    setText(
      KeyShortcut(
        kKeyNoneModifier,
        m_keyshortcut.scancode(),
        m_keyshortcut.unicodeChar()).toString().c_str());
  }

  KeyShortcut m_keyshortcut;
};

SelectKeyShortcut::SelectKeyShortcut(const ui::KeyShortcut& keyshortcut,
                                     const KeyContext keyContext,
                                     const KeyboardShortcuts& currentKeys)
  : m_keyField(new KeyField(keyshortcut))
  , m_keyContext(keyContext)
  , m_currentKeys(currentKeys)
  , m_keyshortcut(keyshortcut)
  , m_ok(false)
  , m_modified(false)
{
  updateModifiers();
  updateAssignedTo();

  keyPlaceholder()->addChild(m_keyField);

  alt()->Click.connect([this]{ onModifierChange(kKeyAltModifier, alt()); });
  cmd()->Click.connect([this]{ onModifierChange(kKeyCmdModifier, cmd()); });
  ctrl()->Click.connect([this]{ onModifierChange(kKeyCtrlModifier, ctrl()); });
  shift()->Click.connect([this]{ onModifierChange(kKeyShiftModifier, shift()); });
  space()->Click.connect([this]{ onModifierChange(kKeySpaceModifier, space()); });
  win()->Click.connect([this]{ onModifierChange(kKeyWinModifier, win()); });

  m_keyField->KeyShortcutChange.connect(&SelectKeyShortcut::onKeyShortcutChange, this);
  clearButton()->Click.connect([this]{ onClear(); });
  okButton()->Click.connect([this]{ onOK(); });
  cancelButton()->Click.connect([this]{ onCancel(); });

  addChild(&m_tooltipManager);
}

void SelectKeyShortcut::onModifierChange(KeyModifiers modifier, CheckBox* checkbox)
{
  bool state = (checkbox->isSelected());
  KeyModifiers modifiers = m_keyshortcut.modifiers();
  KeyScancode scancode = m_keyshortcut.scancode();
  int unicodeChar = m_keyshortcut.unicodeChar();

  modifiers = (KeyModifiers)((modifiers & ~modifier) | (state ? modifier : 0));
  if (modifiers == kKeySpaceModifier && scancode == kKeySpace)
    modifiers = kKeyNoneModifier;

  m_keyshortcut = KeyShortcut(modifiers, scancode, unicodeChar);

  m_keyField->setKeyShortcut(m_keyshortcut);
  m_keyField->requestFocus();
  updateAssignedTo();
}

void SelectKeyShortcut::onKeyShortcutChange(const ui::KeyShortcut* keyshortcut)
{
  m_keyshortcut = *keyshortcut;
  updateModifiers();
  updateAssignedTo();
}

void SelectKeyShortcut::onClear()
{
  m_keyshortcut = KeyShortcut(kKeyNoneModifier, kKeyNil, 0);

  m_keyField->setKeyShortcut(m_keyshortcut);
  updateModifiers();
  updateAssignedTo();

  m_keyField->requestFocus();
}

void SelectKeyShortcut::onOK()
{
  m_ok = true;
  m_modified = (m_origKeyShortcut != m_keyshortcut);
  closeWindow(NULL);
}

void SelectKeyShortcut::onCancel()
{
  closeWindow(NULL);
}

void SelectKeyShortcut::updateModifiers()
{
  alt()->setSelected(m_keyshortcut.modifiers() & kKeyAltModifier ? true: false);
  ctrl()->setSelected(m_keyshortcut.modifiers() & kKeyCtrlModifier ? true: false);
  shift()->setSelected(m_keyshortcut.modifiers() & kKeyShiftModifier ? true: false);
  space()->setSelected(m_keyshortcut.modifiers() & kKeySpaceModifier ? true: false);
#if __APPLE__
  win()->setVisible(false);
  cmd()->setSelected(m_keyshortcut.modifiers() & kKeyCmdModifier ? true: false);
#else
  #if __linux__
    win()->setText(kWinKeyName);
    m_tooltipManager.addTooltipFor(win(), "Also known as Windows key, logo key,\ncommand key, or system key.", TOP);
  #endif
  win()->setSelected(m_keyshortcut.modifiers() & kKeyWinModifier ? true: false);
  cmd()->setVisible(false);
#endif
}

void SelectKeyShortcut::updateAssignedTo()
{
  std::string res = "None";

  for (const KeyPtr& key : m_currentKeys) {
    if (key->keycontext() == m_keyContext &&
        key->hasKeyShortcut(m_keyshortcut)) {
      res = key->triggerString();
      break;
    }
  }

  assignedTo()->setText(res);
}

} // namespace app
