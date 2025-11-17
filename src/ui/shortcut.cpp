// Aseprite UI Library
// Copyright (C) 2020-2025  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "ui/shortcut.h"

#include "base/debug.h"
#include "base/replace_string.h"
#include "base/split_string.h"
#include "base/string.h"
#include "base/trim_string.h"
#include "os/system.h"

#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace ui {

#ifdef _WIN32
const char* kWinKeyName = "Win";
#else
const char* kWinKeyName = "Super";
#endif

namespace {

const char* scancode_to_string[] = {
  // Same order that os::KeyScancode
  nullptr,
  "A",
  "B",
  "C",
  "D",
  "E",
  "F",
  "G",
  "H",
  "I",
  "J",
  "K",
  "L",
  "M",
  "N",
  "O",
  "P",
  "Q",
  "R",
  "S",
  "T",
  "U",
  "V",
  "W",
  "X",
  "Y",
  "Z",
  "0",
  "1",
  "2",
  "3",
  "4",
  "5",
  "6",
  "7",
  "8",
  "9",
  "0 Pad",
  "1 Pad",
  "2 Pad",
  "3 Pad",
  "4 Pad",
  "5 Pad",
  "6 Pad",
  "7 Pad",
  "8 Pad",
  "9 Pad",
  "F1",
  "F2",
  "F3",
  "F4",
  "F5",
  "F6",
  "F7",
  "F8",
  "F9",
  "F10",
  "F11",
  "F12",
  "Esc",
  "~",
  "-",
  "=",
  "Backspace",
  "Tab",
  "[",
  "]",
  "Enter",
  ";",
  "\'",
  "\\",
  "KEY_BACKSLASH2",
  ",",
  ".",
  "/",
  "Space",
  "Ins",
  "Del",
  "Home",
  "End",
  "PgUp",
  "PgDn",
  "Left",
  "Right",
  "Up",
  "Down",
  "/ Pad",
  "* Pad",
  "- Pad",
  "+ Pad",
  "Del Pad",
  "Enter Pad",
  "PrtScr",
  "Pause",
  "KEY_ABNT_C1",
  "Yen",
  "Kana",
  "KEY_CONVERT",
  "KEY_NOCONVERT",
  "KEY_AT",
  "KEY_CIRCUMFLEX",
  "KEY_COLON2",
  "Kanji",
};
int scancode_to_string_size = sizeof(scancode_to_string) / sizeof(scancode_to_string[0]);

} // anonymous namespace

Shortcut::Shortcut() : m_modifiers(kKeyNoneModifier), m_scancode(kKeyNil), m_unicodeChar(0), m_sequence()
{
}

Shortcut::Shortcut(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar)
  : m_modifiers(modifiers)
  , m_scancode(scancode)
  , m_unicodeChar(unicodeChar)
  , m_sequence()
{
}

Shortcut::Shortcut(const std::string& str)
  : m_modifiers(kKeyNoneModifier)
  , m_scancode(kKeyNil)
  , m_unicodeChar(0)
  , m_sequence()
{
  // Check if this is a sequence (contains space separator between keys)
  // Format: "Ctrl+K Ctrl+O"
  std::vector<std::string> parts;
  std::size_t pos = 0, lastPos = 0;
  
  // Look for space separators that aren't part of "Space" keyword
  while (pos < str.size()) {
    // Check if we found a space that's not part of a key name
    if (str[pos] == ' ') {
      // Check if this space is not part of "Space", "Page Up", "Page Down", etc.
      std::string beforeSpaceLower = base::string_to_lower(str.substr(lastPos, pos - lastPos));
      std::string beforeSpace;
      base::trim_string(beforeSpaceLower, beforeSpace);
      
      // If the last token ends with a valid key, this is a sequence separator
      bool isSequenceSeparator = false;
      if (!beforeSpace.empty()) {
        // Check if it ends with a complete key (not "page" or "delete" alone)
        if (beforeSpace.back() == '+' || 
            beforeSpace.find("page ") == std::string::npos) {
          isSequenceSeparator = true;
        }
      }
      
      if (isSequenceSeparator) {
        std::string part;
        base::trim_string(str.substr(lastPos, pos - lastPos), part);
        if (!part.empty())
          parts.push_back(part);
        lastPos = pos + 1;
      }
    }
    ++pos;
  }
  
  // Add the last part
  if (lastPos < str.size()) {
    std::string part;
    base::trim_string(str.substr(lastPos), part);
    if (!part.empty())
      parts.push_back(part);
  }
  
  // If we found multiple parts, create a sequence
  if (parts.size() > 1) {
    for (const std::string& part : parts) {
      m_sequence.push_back(Shortcut(part));
    }
    return;
  }
  
  // Otherwise, parse as a single shortcut
  // Special case: plus sign
  if (str == "+") {
    m_unicodeChar = '+';
    return;
  }

  std::size_t i, j;
  for (i = 0; i < str.size(); i = j + 1) {
    // i+1 because the first character can be '+' sign
    for (j = i + 1; j < str.size() && str[j] != '+'; ++j)
      ;
    std::string tok = base::string_to_lower(str.substr(i, j - i));

    if (m_scancode == kKeySpace) {
      m_modifiers = (KeyModifiers)((int)m_modifiers | (int)kKeySpaceModifier);
      m_scancode = kKeyNil;
    }

    // Modifiers
    if (tok == "shift") {
      m_modifiers = (KeyModifiers)((int)m_modifiers | (int)kKeyShiftModifier);
    }
    else if (tok == "alt") {
      m_modifiers = (KeyModifiers)((int)m_modifiers | (int)kKeyAltModifier);
    }
    else if (tok == "ctrl") {
      m_modifiers = (KeyModifiers)((int)m_modifiers | (int)kKeyCtrlModifier);
    }
    else if (tok == "cmd") {
      m_modifiers = (KeyModifiers)((int)m_modifiers | (int)kKeyCmdModifier);
    }
    else if (tok == base::string_to_lower(kWinKeyName)) {
      m_modifiers = (KeyModifiers)((int)m_modifiers | (int)kKeyWinModifier);
    }
    // Word with one character
    else if (base::utf8_length(tok) == 1) {
      std::wstring wstr = base::from_utf8(tok);
      if (wstr.size() != 1) {
        ASSERT(false && "Something wrong converting utf-8 to wchar string");
        continue;
      }
      m_unicodeChar = std::tolower(wstr[0]);
      m_scancode = kKeyNil;
    }
    // F1, F2, ..., F11, F12
    else if (tok[0] == 'f' && (tok.size() <= 3)) {
      int num = std::strtol(tok.c_str() + 1, nullptr, 10);
      if ((num >= 1) && (num <= 12))
        m_scancode = (KeyScancode)((int)kKeyF1 + num - 1);
    }
    else if ((tok == "escape") || (tok == "esc"))
      m_scancode = kKeyEsc;
    else if (tok == "backspace")
      m_scancode = kKeyBackspace;
    else if (tok == "tab")
      m_scancode = kKeyTab;
    else if (tok == "enter")
      m_scancode = kKeyEnter;
    else if (tok == "space")
      m_scancode = kKeySpace;
    else if ((tok == "insert") || (tok == "ins"))
      m_scancode = kKeyInsert;
    else if ((tok == "delete") || (tok == "del"))
      m_scancode = kKeyDel;
    else if (tok == "home")
      m_scancode = kKeyHome;
    else if (tok == "end")
      m_scancode = kKeyEnd;
    else if ((tok == "page up") || (tok == "pgup"))
      m_scancode = kKeyPageUp;
    else if ((tok == "page down") || (tok == "pgdn"))
      m_scancode = kKeyPageDown;
    else if (tok == "left")
      m_scancode = kKeyLeft;
    else if (tok == "right")
      m_scancode = kKeyRight;
    else if (tok == "up")
      m_scancode = kKeyUp;
    else if (tok == "down")
      m_scancode = kKeyDown;
    else if (tok == "0 pad")
      m_scancode = kKey0Pad;
    else if (tok == "1 pad")
      m_scancode = kKey1Pad;
    else if (tok == "2 pad")
      m_scancode = kKey2Pad;
    else if (tok == "3 pad")
      m_scancode = kKey3Pad;
    else if (tok == "4 pad")
      m_scancode = kKey4Pad;
    else if (tok == "5 pad")
      m_scancode = kKey5Pad;
    else if (tok == "6 pad")
      m_scancode = kKey6Pad;
    else if (tok == "7 pad")
      m_scancode = kKey7Pad;
    else if (tok == "8 pad")
      m_scancode = kKey8Pad;
    else if (tok == "9 pad")
      m_scancode = kKey9Pad;
    else if (tok == "/ pad" || tok == "slash pad")
      m_scancode = kKeySlashPad;
    else if (tok == "* pad" || tok == "asterisk pad" || tok == "asterisk")
      m_scancode = kKeyAsterisk;
    else if (tok == "- pad" || tok == "minus pad")
      m_scancode = kKeyMinusPad;
    else if (tok == "+ pad" || tok == "plus pad")
      m_scancode = kKeyPlusPad;
    else if (tok == "del pad" || tok == "delete pad")
      m_scancode = kKeyDelPad;
    else if (tok == "enter pad")
      m_scancode = kKeyEnterPad;
  }
}

const Shortcut& Shortcut::getKeyAt(std::size_t index) const
{
  if (!m_sequence.empty()) {
    ASSERT(index < m_sequence.size());
    return m_sequence[index];
  }
  ASSERT(index == 0);
  return *this;
}

// static
Shortcut Shortcut::MakeSequence(const std::vector<Shortcut>& keys)
{
  Shortcut result;
  if (keys.empty())
    return result;
  
  if (keys.size() == 1)
    return keys[0];
  
  // Store sequence
  result.m_sequence = keys;
  return result;
}

bool Shortcut::operator==(const Shortcut& other) const
{
  // If both are sequences, compare sequences
  if (isSequence() && other.isSequence()) {
    if (m_sequence.size() != other.m_sequence.size())
      return false;
    for (std::size_t i = 0; i < m_sequence.size(); ++i) {
      if (m_sequence[i] != other.m_sequence[i])
        return false;
    }
    return true;
  }
  
  // If one is sequence and other is not, they're different
  if (isSequence() != other.isSequence())
    return false;
  
  // Both are single shortcuts
  // TODO improve this, avoid conversion to std::string
  return toString() == other.toString();
}

bool Shortcut::isEmpty() const
{
  if (isSequence())
    return m_sequence.empty();
  return (m_modifiers == kKeyNoneModifier && m_scancode == kKeyNil && m_unicodeChar == 0);
}

std::string Shortcut::toString() const
{
  // If this is a sequence, concatenate all parts
  if (isSequence()) {
    std::string result;
    for (std::size_t i = 0; i < m_sequence.size(); ++i) {
      if (i > 0)
        result += " ";
      result += m_sequence[i].toString();
    }
    return result;
  }
  
  // Single shortcut
  std::string buf;

  // Shifts
  if (m_modifiers & kKeyCtrlModifier)
    buf += "Ctrl+";
  if (m_modifiers & kKeyCmdModifier)
    buf += "Cmd+";
  if (m_modifiers & kKeyAltModifier)
    buf += "Alt+";
  if (m_modifiers & kKeyShiftModifier)
    buf += "Shift+";
  if ((m_modifiers & kKeySpaceModifier) && (m_scancode != kKeySpace) && (m_unicodeChar != ' ')) {
    buf += "Space+";
  }
  if (m_modifiers & kKeyWinModifier) {
    buf += kWinKeyName;
    buf += "+";
  }

  // Key
  if (m_unicodeChar) {
    std::wstring wideUnicodeChar;
    wideUnicodeChar.push_back((wchar_t)std::toupper(m_unicodeChar));
    buf += base::to_utf8(wideUnicodeChar);
  }
  else if (m_scancode > 0 && m_scancode < scancode_to_string_size && scancode_to_string[m_scancode])
    buf += scancode_to_string[m_scancode];
  else if (!buf.empty() && buf[buf.size() - 1] == '+')
    buf.erase(buf.size() - 1);

  return buf;
}

bool Shortcut::isPressed(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar) const
{
  return ((scancode && *this == Shortcut(modifiers, scancode, 0)) ||
          (unicodeChar && *this == Shortcut(modifiers, kKeyNil, unicodeChar)));
}

bool Shortcut::isPressed() const
{
  os::SystemRef sys = os::System::instance();
  if (!sys)
    return false;

  KeyModifiers pressedModifiers = sys->keyModifiers();

  // Check if this shortcut is only
  if (m_scancode == kKeyNil && m_unicodeChar == 0)
    return (m_modifiers == pressedModifiers);

  // Compare with all pressed scancodes
  for (int s = int(kKeyNil); s < int(kKeyFirstModifierScancode); ++s) {
    if (sys->isKeyPressed(KeyScancode(s)) &&
        isPressed(pressedModifiers, KeyScancode(s), sys->getUnicodeFromScancode(KeyScancode(s))))
      return true;
  }

  return false;
}

bool Shortcut::isLooselyPressed() const
{
  os::SystemRef sys = os::System::instance();
  if (!sys)
    return false;

  KeyModifiers pressedModifiers = sys->keyModifiers();

  if ((m_modifiers & pressedModifiers) != m_modifiers)
    return false;

  // Check if this shortcut is only
  if (m_scancode == kKeyNil && m_unicodeChar == 0)
    return true;

  // Compare with all pressed scancodes
  for (int s = int(kKeyNil); s < int(kKeyFirstModifierScancode); ++s) {
    if (sys->isKeyPressed(KeyScancode(s)) && isPressed(m_modifiers, // Use same modifiers (we've
                                                                    // already compared the
                                                                    // modifiers)
                                                       KeyScancode(s),
                                                       sys->getUnicodeFromScancode(KeyScancode(s))))
      return true;
  }

  return false;
}

} // namespace ui
