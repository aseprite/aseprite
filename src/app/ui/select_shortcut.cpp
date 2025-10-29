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
#include "base/time.h"
#include "obs/signal.h"
#include "ui/entry.h"
#include "ui/message.h"

#include <cctype>
#include <vector>

namespace app {

using namespace ui;

class SelectShortcut::KeyField : public ui::Entry {
public:
  static constexpr int kMaxSequenceKeys = 4;
  
  KeyField(const Shortcut& shortcut) : ui::Entry(256, ""), m_sequenceKeys(), m_isFirstKeypress(true), m_lastKeyTime(0), m_sequenceTimeout(500.0)
  {
    setTranslateDeadKeys(false);
    setExpansive(true);
    setFocusMagnet(true);
    setShortcut(shortcut);
  }

  void setShortcut(const Shortcut& shortcut)
  {
    m_shortcut = shortcut;
    
    // Always populate sequence keys for consistency
    m_sequenceKeys.clear();
    if (!shortcut.isEmpty()) {
      if (shortcut.isSequence()) {
        for (std::size_t i = 0; i < shortcut.sequenceSize(); ++i) {
          m_sequenceKeys.push_back(shortcut.getKeyAt(i));
        }
      }
      else {
        // Single key - add it to sequence keys
        m_sequenceKeys.push_back(shortcut);
      }
    }
    
    // Reset the firstKeypress flag and timer when setting a shortcut programmatically
    m_isFirstKeypress = true;
    m_lastKeyTime = 0;
    
    updateText();
  }

  const std::vector<Shortcut>& sequenceKeys() const
  {
    return m_sequenceKeys;
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

          // Ignore pure modifier keys (Alt, Ctrl, Shift, Win, etc.)
          if (keymsg->scancode() >= kKeyFirstModifierScancode)
            break;

          KeyModifiers modifiers = keymsg->modifiers();

          // Special handling for Space key:
          // If Space scancode with Space modifier, it means Space was just pressed as a modifier key
          // We should ignore it (user is holding Space to use as modifier for next key)
          if (keymsg->scancode() == kKeySpace && (modifiers & kKeySpaceModifier)) {
            // Ignore Space when pressed as a modifier key
            break;
          }

          // Get current time
          double currentTime = base::current_tick();
          
          // Check if this is a new sequence or continuation
          bool isNewSequence = false;
          if (m_isFirstKeypress || m_lastKeyTime == 0 || (currentTime - m_lastKeyTime) > m_sequenceTimeout) {
            isNewSequence = true;
          }

          Shortcut newKey(
            modifiers,
            keymsg->scancode(),
            keymsg->unicodeChar() > 32 ? std::tolower(keymsg->unicodeChar()) : 0);

          // Start new sequence or continue existing one
          if (isNewSequence) {
            m_sequenceKeys.clear();
            m_sequenceKeys.push_back(newKey);
            m_isFirstKeypress = false;
          } else {
            // Add to sequence (up to max keys)
            if (m_sequenceKeys.size() < kMaxSequenceKeys) {
              m_sequenceKeys.push_back(newKey);
            }
            else {
              // At max capacity - replace the last key
              m_sequenceKeys.back() = newKey;
            }
          }

          m_lastKeyTime = currentTime;

          // Build the shortcut
          if (m_sequenceKeys.size() > 1) {
            m_shortcut = Shortcut::MakeSequence(m_sequenceKeys);
          }
          else {
            // For single key, normalize by converting to string and back
            // This ensures consistency with how shortcuts are loaded from files
            Shortcut normalized(m_sequenceKeys[0].toString());
            m_shortcut = normalized;
          }

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
    if (m_shortcut.isSequence()) {
      // Display the sequence with modifiers removed from the text
      // (modifiers are shown in checkboxes)
      std::string text;
      for (std::size_t i = 0; i < m_shortcut.sequenceSize(); ++i) {
        if (i > 0)
          text += " ";
        const Shortcut& key = m_shortcut.getKeyAt(i);
        text += Shortcut(kKeyNoneModifier, key.scancode(), key.unicodeChar()).toString();
      }
      setText(text);
    }
    else {
      // Single key - display without modifiers
      setText(Shortcut(kKeyNoneModifier, m_shortcut.scancode(), m_shortcut.unicodeChar()).toString());
    }
  }

private:
  Shortcut m_shortcut;
  std::vector<Shortcut> m_sequenceKeys;
  bool m_isFirstKeypress;
  double m_lastKeyTime;
  double m_sequenceTimeout;
};

SelectShortcut::SelectShortcut(const ui::Shortcut& shortcut,
                               const KeyContext keyContext,
                               const KeyboardShortcuts& currentKeys)
  : m_keyField(new KeyField(shortcut))
  , m_keyContext(keyContext)
  , m_currentKeys(currentKeys)
  , m_origShortcut(shortcut)
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
  
  updateSequenceInfo();
}

void SelectShortcut::onModifierChange(KeyModifiers modifier, CheckBox* checkbox)
{
  bool state = (checkbox->isSelected());
  
  // If this is a sequence, apply the modifier change to all keys in the sequence
  if (m_shortcut.isSequence()) {
    std::vector<Shortcut> updatedSequence;
    for (std::size_t i = 0; i < m_shortcut.sequenceSize(); ++i) {
      const Shortcut& key = m_shortcut.getKeyAt(i);
      KeyModifiers modifiers = key.modifiers();
      KeyScancode scancode = key.scancode();
      int unicodeChar = key.unicodeChar();
      
      modifiers = (KeyModifiers)((modifiers & ~modifier) | (state ? modifier : 0));
      if (modifiers == kKeySpaceModifier && scancode == kKeySpace)
        modifiers = kKeyNoneModifier;
      
      updatedSequence.push_back(Shortcut(modifiers, scancode, unicodeChar));
    }
    m_shortcut = Shortcut::MakeSequence(updatedSequence);
  }
  else {
    // Single key shortcut
    KeyModifiers modifiers = m_shortcut.modifiers();
    KeyScancode scancode = m_shortcut.scancode();
    int unicodeChar = m_shortcut.unicodeChar();

    modifiers = (KeyModifiers)((modifiers & ~modifier) | (state ? modifier : 0));
    if (modifiers == kKeySpaceModifier && scancode == kKeySpace)
      modifiers = kKeyNoneModifier;

    m_shortcut = Shortcut(modifiers, scancode, unicodeChar);
  }

  // Manually clicking modifiers resets the sequence (user wants to start fresh)
  m_keyField->setShortcut(m_shortcut);
  m_keyField->requestFocus();
  updateAssignedTo();
  updateSequenceInfo();
}

void SelectShortcut::onShortcutChange(const ui::Shortcut* shortcut)
{
  m_shortcut = *shortcut;
  updateModifiers();
  updateAssignedTo();
  updateSequenceInfo();
}

void SelectShortcut::onClear()
{
  m_shortcut = Shortcut(kKeyNoneModifier, kKeyNil, 0);

  m_keyField->setShortcut(m_shortcut);
  updateModifiers();
  updateAssignedTo();
  updateSequenceInfo();

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
  // For sequences, show the modifiers of the first key
  // (since all keys in the sequence share the same modifiers)
  KeyModifiers modifiers;
  if (m_shortcut.isSequence() && m_shortcut.sequenceSize() > 0) {
    modifiers = m_shortcut.getKeyAt(0).modifiers();
  }
  else {
    modifiers = m_shortcut.modifiers();
  }
  
  alt()->setSelected((modifiers & kKeyAltModifier) == kKeyAltModifier);
  ctrl()->setSelected((modifiers & kKeyCtrlModifier) == kKeyCtrlModifier);
  shift()->setSelected((modifiers & kKeyShiftModifier) == kKeyShiftModifier);
  space()->setSelected((modifiers & kKeySpaceModifier) == kKeySpaceModifier);
#if __APPLE__
  win()->setVisible(false);
  cmd()->setSelected((modifiers & kKeyCmdModifier) == kKeyCmdModifier);
#else
  #if __linux__
  win()->setText(kWinKeyName);
  m_tooltipManager.addTooltipFor(
    win(),
    "Also known as Windows key, logo key,\ncommand key, or system key.",
    TOP);
  #endif
  win()->setSelected((modifiers & kKeyWinModifier) == kKeyWinModifier);
  cmd()->setVisible(false);
#endif
}

void SelectShortcut::updateAssignedTo()
{
  std::string res = "None";

  // Check against the full shortcut (handles both single and multi-key)
  for (const KeyPtr& key : m_currentKeys) {
    if (key->keycontext() == m_keyContext && key->hasShortcut(m_shortcut)) {
      res = key->triggerString();
      break;
    }
  }

  assignedTo()->setText(res);
}

const ui::Shortcut& SelectShortcut::shortcut() const
{
  // Return the full shortcut (which might be a sequence)
  return m_shortcut;
}

const std::vector<ui::Shortcut>& SelectShortcut::shortcutSequence() const
{
  return m_keyField->sequenceKeys();
}

void SelectShortcut::updateSequenceInfo()
{
  if (m_shortcut.isSequence() && m_shortcut.sequenceSize() > 1) {
    std::string info = "Multi-key: " + m_shortcut.toString() + " (Press another key to continue)";
    sequenceInfo()->setText(info);
    sequenceInfo()->setVisible(true);
  } else if (!m_shortcut.isEmpty()) {
    sequenceInfo()->setText("Single key (Press another key to create multi-key shortcut)");
    sequenceInfo()->setVisible(true);
  } else {
    sequenceInfo()->setText("");
    sequenceInfo()->setVisible(false);
  }
}

} // namespace app
