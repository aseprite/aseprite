// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/select_shortcut.h"

#include "app/ui/key.h"
#include "app/ui/keyboard_shortcuts.h"
#include "obs/signal.h"
#include "ui/entry.h"
#include "ui/message.h"

#include <cctype>

namespace app {

using namespace ui;

class SelectShortcut::KeyField : public ui::Entry {
public:
  KeyField(const Shortcut& shortcut) : ui::Entry(256, "")
  {
    setId("key_field");
    setTranslateDeadKeys(false);
    setExpansive(true);
    setFocusMagnet(true);
    setShortcut(shortcut);
  }

  void setShortcut(const Shortcut& shortcut)
  {
    m_shortcut = shortcut;
    updateText();
  }

  obs::signal<void(const ui::Shortcut*)> ShortcutChange;

protected:
  bool onProcessMessage(Message* msg) override
  {
    switch (msg->type()) {
      case kKeyDownMessage:
        if (hasFocus() && !isReadOnly()) {
          KeyMessage* keymsg = static_cast<KeyMessage*>(msg);
          if (!keymsg->scancode() && keymsg->unicodeChar() < 32)
            break;

          KeyModifiers modifiers = keymsg->modifiers();

          if (keymsg->scancode() == kKeySpace)
            modifiers = (KeyModifiers)(modifiers & ~kKeySpaceModifier);

          m_shortcut = Shortcut(
            modifiers,
            keymsg->scancode(),
            keymsg->unicodeChar() > 32 ? std::tolower(keymsg->unicodeChar()) : 0);

          // Convert the shortcut to a string, and parse it
          // again. Just to obtain the exact shortcut we'll read
          // when we import the gui.xml file or an .aseprite-keys file.
          m_shortcut = Shortcut(m_shortcut.toString());

          updateText();

          ShortcutChange(&m_shortcut);
          return true;
        }
        break;
    }
    return Entry::onProcessMessage(msg);
  }

  void updateText()
  {
    setText(Shortcut(kKeyNoneModifier, m_shortcut.scancode(), m_shortcut.unicodeChar()).toString());
  }

private:
  Shortcut m_shortcut;
};

SelectShortcut::SelectShortcut(const ui::Shortcut& shortcut,
                               const KeyContext keyContext,
                               const KeyboardShortcuts& currentKeys)
  : m_keyField(new KeyField(shortcut))
  , m_keyContext(keyContext)
  , m_currentKeys(currentKeys)
  , m_shortcut(shortcut)
  , m_ok(false)
  , m_modified(false)
{
  updateModifiers();
  updateAssignedTo();

  keyPlaceholder()->addChild(m_keyField);

  alt()->Click.connect([this] { onModifierChange(kKeyAltModifier, alt()); });
  cmd()->Click.connect([this] { onModifierChange(kKeyCmdModifier, cmd()); });
  ctrl()->Click.connect([this] { onModifierChange(kKeyCtrlModifier, ctrl()); });
  shift()->Click.connect([this] { onModifierChange(kKeyShiftModifier, shift()); });
  space()->Click.connect([this] { onModifierChange(kKeySpaceModifier, space()); });
  win()->Click.connect([this] { onModifierChange(kKeyWinModifier, win()); });

  m_keyField->ShortcutChange.connect(&SelectShortcut::onShortcutChange, this);
  clearButton()->Click.connect([this] { onClear(); });
  okButton()->Click.connect([this] { onOK(); });
  cancelButton()->Click.connect([this] { onCancel(); });

  addChild(&m_tooltipManager);
}

void SelectShortcut::onModifierChange(KeyModifiers modifier, CheckBox* checkbox)
{
  bool state = (checkbox->isSelected());
  KeyModifiers modifiers = m_shortcut.modifiers();
  KeyScancode scancode = m_shortcut.scancode();
  int unicodeChar = m_shortcut.unicodeChar();

  modifiers = (KeyModifiers)((modifiers & ~modifier) | (state ? modifier : 0));
  if (modifiers == kKeySpaceModifier && scancode == kKeySpace)
    modifiers = kKeyNoneModifier;

  m_shortcut = Shortcut(modifiers, scancode, unicodeChar);

  m_keyField->setShortcut(m_shortcut);
  m_keyField->requestFocus();
  updateAssignedTo();
}

void SelectShortcut::onShortcutChange(const ui::Shortcut* shortcut)
{
  m_shortcut = *shortcut;
  updateModifiers();
  updateAssignedTo();
}

void SelectShortcut::onClear()
{
  m_shortcut = Shortcut(kKeyNoneModifier, kKeyNil, 0);

  m_keyField->setShortcut(m_shortcut);
  updateModifiers();
  updateAssignedTo();

  m_keyField->requestFocus();
}

void SelectShortcut::onOK()
{
  m_ok = true;
  m_modified = (m_origShortcut != m_shortcut);
  closeWindow(NULL);
}

void SelectShortcut::onCancel()
{
  closeWindow(NULL);
}

void SelectShortcut::updateModifiers()
{
  alt()->setSelected((m_shortcut.modifiers() & kKeyAltModifier) == kKeyAltModifier);
  ctrl()->setSelected((m_shortcut.modifiers() & kKeyCtrlModifier) == kKeyCtrlModifier);
  shift()->setSelected((m_shortcut.modifiers() & kKeyShiftModifier) == kKeyShiftModifier);
  space()->setSelected((m_shortcut.modifiers() & kKeySpaceModifier) == kKeySpaceModifier);
#if __APPLE__
  win()->setVisible(false);
  cmd()->setSelected((m_shortcut.modifiers() & kKeyCmdModifier) == kKeyCmdModifier);
#else
  #if __linux__
  win()->setText(kWinKeyName);
  m_tooltipManager.addTooltipFor(
    win(),
    "Also known as Windows key, logo key,\ncommand key, or system key.",
    TOP);
  #endif
  win()->setSelected((m_shortcut.modifiers() & kKeyWinModifier) == kKeyWinModifier);
  cmd()->setVisible(false);
#endif
}

void SelectShortcut::updateAssignedTo()
{
  std::string res = "None";

  for (const KeyPtr& key : m_currentKeys) {
    if (key->keycontext() == m_keyContext && key->hasShortcut(m_shortcut)) {
      res = key->triggerString();
      break;
    }
  }

  assignedTo()->setText(res);
}

} // namespace app
