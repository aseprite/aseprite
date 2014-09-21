// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/accelerator.h"

#include "base/unique_ptr.h"
#include "base/split_string.h"
#include "base/string.h"

#include <cctype>
#include <string>
#include <vector>

// #define REPORT_KEYS
#define PREPROCESS_KEYS

namespace ui {

void Accelerator::addKey(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar)
{
  KeyCombo key;

  key.modifiers = modifiers;
  key.scancode = scancode;
  key.unicodeChar = unicodeChar;

  m_combos.push_back(key);
}

void Accelerator::addKeysFromString(const std::string& str)
{
  KeyModifiers modifiers = kKeyNoneModifier;
  KeyScancode scancode = kKeyNil;
  int unicodeChar = 0;

  // Special case: plus sign
  if (str == "+") {
    addKey(kKeyNoneModifier, kKeyNil, '+');
    return;
  }

  std::vector<std::string> tokens;
  base::split_string(str, tokens, "+");
  for (std::string tok : tokens) {
    tok = base::string_to_lower(tok);

    if (scancode == kKeySpace) {
      modifiers = (KeyModifiers)((int)modifiers | (int)kKeySpaceModifier);
      scancode = kKeyNil;
    }

    // Modifiers
    if (tok == "shift") {
      modifiers = (KeyModifiers)((int)modifiers | (int)kKeyShiftModifier);
    }
    else if (tok == "alt") {
      modifiers = (KeyModifiers)((int)modifiers | (int)kKeyAltModifier);
    }
    else if (tok == "ctrl") {
      modifiers = (KeyModifiers)((int)modifiers | (int)kKeyCtrlModifier);
    }
    else if (tok == "cmd") {
      modifiers = (KeyModifiers)((int)modifiers | (int)kKeyCmdModifier);
    }

    // Scancode

    // word with one character
    else if (tok.size() == 1) {
      if ((tok[0] >= 'a') && (tok[0] <= 'z')) {
        unicodeChar = tok[0];
      }
      else {
        unicodeChar = tok[0];
      }

      if ((tok[0] >= 'a') && (tok[0] <= 'z'))
        scancode = (KeyScancode)((int)kKeyA + tolower(tok[0]) - 'a');
      else if ((tok[0] >= '0') && (tok[0] <= '9'))
        scancode = (KeyScancode)((int)kKey0 + tok[0] - '0');
      else {
        switch (tok[0]) {
          case '~': scancode = kKeyTilde; break;
          case '-': scancode = kKeyMinus; break;
          case '=': scancode = kKeyEquals; break;
          case '[': scancode = kKeyOpenbrace; break;
          case ']': scancode = kKeyClosebrace; break;
          case ';': scancode = kKeyColon; break;
          case '\'': scancode = kKeyQuote; break;
          case '\\': scancode = kKeyBackslash; break;
          case ',': scancode = kKeyComma; break;
          case '.': scancode = kKeyStop; break;
          case '/': scancode = kKeySlash; break;
          case '*': scancode = kKeyAsterisk; break;
        }
      }
    }
    /* other ones */
    else {
      /* F1, F2, ..., F11, F12 */
      if (tok[0] == 'f' && (tok.size() <= 3)) {
        int num = strtol(tok.c_str()+1, NULL, 10);
        if ((num >= 1) && (num <= 12))
          scancode = (KeyScancode)((int)kKeyF1 + num - 1);
      }
      else if ((tok == "escape") || (tok == "esc"))
        scancode = kKeyEsc;
      else if (tok == "backspace")
        scancode = kKeyBackspace;
      else if (tok == "tab")
        scancode = kKeyTab;
      else if (tok == "enter")
        scancode = kKeyEnter;
      else if (tok == "space")
        scancode = kKeySpace;
      else if ((tok == "insert") || (tok == "ins"))
        scancode = kKeyInsert;
      else if ((tok == "delete") || (tok == "del"))
        scancode = kKeyDel;
      else if (tok == "home")
        scancode = kKeyHome;
      else if (tok == "end")
        scancode = kKeyEnd;
      else if ((tok == "page up") || (tok == "pgup"))
        scancode = kKeyPageUp;
      else if ((tok == "page down") || (tok == "pgdn"))
        scancode = kKeyPageDown;
      else if (tok == "left")
        scancode = kKeyLeft;
      else if (tok == "right")
        scancode = kKeyRight;
      else if (tok == "up")
        scancode = kKeyUp;
      else if (tok == "down")
        scancode = kKeyDown;
      else if (tok == "0 pad")
        scancode = kKey0Pad;
      else if (tok == "1 pad")
        scancode = kKey1Pad;
      else if (tok == "2 pad")
        scancode = kKey2Pad;
      else if (tok == "3 pad")
        scancode = kKey3Pad;
      else if (tok == "4 pad")
        scancode = kKey4Pad;
      else if (tok == "5 pad")
        scancode = kKey5Pad;
      else if (tok == "6 pad")
        scancode = kKey6Pad;
      else if (tok == "7 pad")
        scancode = kKey7Pad;
      else if (tok == "8 pad")
        scancode = kKey8Pad;
      else if (tok == "9 pad")
        scancode = kKey9Pad;
      else if (tok == "slash pad")
        scancode = kKeySlashPad;
      else if (tok == "asterisk")
        scancode = kKeyAsterisk;
      else if (tok == "minus pad")
        scancode = kKeyMinusPad;
      else if (tok == "plus pad")
        scancode = kKeyPlusPad;
      else if (tok == "del pad")
        scancode = kKeyDelPad;
      else if (tok == "enter pad")
        scancode = kKeyEnterPad;
    }
  }

  addKey(modifiers, scancode, unicodeChar);
}

std::string Accelerator::KeyCombo::toString()
{
  // Same order that Allegro scancodes
  static const char *table[] = {
    NULL,
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
    "Delete Pad",
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

  std::string buf;

  // Shifts
  if (this->modifiers & kKeyCtrlModifier) buf += "Ctrl+";
  if (this->modifiers & kKeyCmdModifier) buf += "Cmd+";
  if (this->modifiers & kKeyAltModifier) buf += "Alt+";
  if (this->modifiers & kKeyShiftModifier) buf += "Shift+";
  if (this->modifiers & kKeySpaceModifier) buf += "Space+";

  // Key
  if (this->unicodeChar)
    buf += (wchar_t)toupper(this->unicodeChar);
  else if (this->scancode)
    buf += table[this->scancode];
  else if (!buf.empty() && buf[buf.size()-1] == '+')
    buf.erase(buf.size()-1);

  return buf;
}

std::string Accelerator::toString()
{
  ASSERT(!m_combos.empty());
  return m_combos.front().toString();
}

bool Accelerator::check(KeyModifiers modifiers, KeyScancode scancode, int unicodeChar)
{
#ifdef REPORT_KEYS
  char buf[256];
  std::string buf2;
#endif

  // Preprocess the character to be compared with the accelerator
#ifdef PREPROCESS_KEYS
  // Directly scancode
  if ((scancode >= kKeyF1 && scancode <= kKeyF12) ||
      (scancode == kKeyEsc) ||
      (scancode == kKeyBackspace) ||
      (scancode == kKeyTab) ||
      (scancode == kKeyEnter) ||
      (scancode == kKeyBackslash) ||
      (scancode == kKeyBackslash2) ||
      (scancode >= kKeySpace && scancode <= kKeyDown) ||
      (scancode >= kKeyEnterPad && scancode <= kKeyNoconvert) ||
      (scancode == kKeyKanji)) {
    unicodeChar = 0;
  }
  // For Ctrl+number
  /*           scancode    unicodeChar
     Ctrl+0    27          0
     Ctrl+1    28          2
     Ctrl+2    29          0
     Ctrl+3    30          27
     Ctrl+4    31          28
     Ctrl+5    32          29
     Ctrl+6    33          30
     Ctrl+7    34          31
     Ctrl+8    35          127
     Ctrl+9    36          2
   */
  else if ((scancode >= kKey0 && scancode <= kKey9) &&
           (unicodeChar < 32 || unicodeChar == 127)) {
    unicodeChar = '0' + scancode - kKey0;
    scancode = kKeyNil;
  }
  // For Ctrl+letter
  else if (unicodeChar >= 1 && unicodeChar <= 'z'-'a'+1) {
    unicodeChar = 'a'+unicodeChar-1;
    scancode = kKeyNil;
  }
  // For any other legal Unicode code
  else if (unicodeChar >= ' ') {
    unicodeChar = tolower(unicodeChar);

    /* without shift (because characters like '*' can be trigger with
       "Shift+8", so we don't want "Shift+*") */
    if (!(unicodeChar >= 'a' && unicodeChar <= 'z'))
      modifiers = (KeyModifiers)((int)modifiers & ((int)~kKeyShiftModifier));

    scancode = kKeyNil;
  }
#endif

#ifdef REPORT_KEYS
  {
    base::UniquePtr<Accelerator> a2(new Accelerator);
    a2->addKey(modifiers, scancode, unicodeChar);
    buf2 = a2->getString();
  }
#endif

  for (KeyCombos::iterator it = m_combos.begin(), end = m_combos.end();
       it != end; ++it) {
#ifdef REPORT_KEYS
    printf("%3d==%3d %3d==%3d %s==%s ",
           it->scancode, scancode, it->unicodeChar, unicodeChar,
           it->getString().c_str(), buf2.c_str();
#endif

    if ((it->modifiers == modifiers) &&
        ((it->scancode != kKeyNil && it->scancode == scancode) ||
         (it->unicodeChar && it->unicodeChar == unicodeChar))) {

#ifdef REPORT_KEYS
      printf("true\n");
#endif

      return true;
    }
#ifdef REPORT_KEYS
    printf("false\n");
#endif
  }

  return false;
}

bool Accelerator::checkFromAllegroKeyArray()
{
  KeyModifiers modifiers = kKeyNoneModifier;

  if (she::is_key_pressed(kKeyLShift)  ) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyShiftModifier);
  if (she::is_key_pressed(kKeyRShift)  ) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyShiftModifier);
  if (she::is_key_pressed(kKeyLControl)) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyCtrlModifier);
  if (she::is_key_pressed(kKeyRControl)) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyCtrlModifier);
  if (she::is_key_pressed(kKeyAlt)     ) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyAltModifier);
  if (she::is_key_pressed(kKeyCommand) ) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyCmdModifier);

  for (KeyCombos::iterator it = m_combos.begin(), end = m_combos.end();
       it != end; ++it) {
    if ((it->scancode == 0 || she::is_key_pressed(it->scancode)) &&
        (it->modifiers == modifiers)) {
      return true;
    }
  }
  return false;
}

} // namespace ui
