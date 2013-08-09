// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under MIT license,
// please read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ui/accelerator.h"

#include "base/unique_ptr.h"

#include <allegro/keyboard.h>
#include <allegro/unicode.h>
#include <ctype.h>

// #define REPORT_KEYS
#define PREPROCESS_KEYS

namespace ui {

void Accelerator::addKey(KeyModifiers modifiers, KeyScancode scancode, int ascii)
{
  KeyCombo key;

  key.modifiers = modifiers;
  key.scancode = scancode;
  key.ascii = ascii;

  m_combos.push_back(key);
}

static void process_one_word(Accelerator* accel, char* word)
{
  KeyModifiers modifiers = kKeyNoneModifier;
  KeyScancode scancode = kKeyNil;
  int ascii = 0;
  char* tok;

  // Special case: plus sign
  if (word[0] == '+' && word[1] == 0) {
    accel->addKey(kKeyNoneModifier, kKeyNil, '+');
    return;
  }

  for (tok=ustrtok(word, "+"); tok;
       tok=ustrtok(NULL, "+")) {
    // Modifiers

    if (ustricmp(tok, "Shift") == 0) {
      modifiers = (KeyModifiers)((int)modifiers | (int)kKeyShiftModifier);
    }
    else if (ustricmp (tok, "Alt") == 0) {
      modifiers = (KeyModifiers)((int)modifiers | (int)kKeyAltModifier);
    }
    else if (ustricmp (tok, "Ctrl") == 0) {
      modifiers = (KeyModifiers)((int)modifiers | (int)kKeyCtrlModifier);
    }

    // Scancode

    // word with one character
    else if (tok[1] == 0) {
      if (((*tok >= 'a') && (*tok <= 'z')) ||
          ((*tok >= 'A') && (*tok <= 'Z'))) {
        ascii = tolower(*tok);
      }
      else {
        ascii = *tok;
      }

      if (((*tok >= 'a') && (*tok <= 'z')) ||
          ((*tok >= 'A') && (*tok <= 'Z')))
        scancode = (KeyScancode)((int)kKeyA + tolower(*tok) - 'a');
      else if ((*tok >= '0') && (*tok <= '9'))
        scancode = (KeyScancode)((int)kKey0 + *tok - '0');
      else {
        switch (*tok) {
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
      if ((toupper (*tok) == 'F') && (ustrlen(tok) <= 3)) {
        int num = ustrtol(tok+1, NULL, 10);
        if ((num >= 1) && (num <= 12))
          scancode = (KeyScancode)((int)kKeyF1 + num - 1);
      }
      else if ((ustricmp(tok, "Escape") == 0) ||
               (ustricmp(tok, "Esc") == 0))
        scancode = kKeyEsc;
      else if (ustricmp(tok, "Backspace") == 0)
        scancode = kKeyBackspace;
      else if (ustricmp(tok, "Tab") == 0)
        scancode = kKeyTab;
      else if (ustricmp(tok, "Enter") == 0)
        scancode = kKeyEnter;
      else if (ustricmp(tok, "Space") == 0)
        scancode = kKeySpace;
      else if ((ustricmp(tok, "Insert") == 0) ||
               (ustricmp(tok, "Ins") == 0))
        scancode = kKeyInsert;
      else if ((ustricmp(tok, "Delete") == 0) ||
               (ustricmp(tok, "Del") == 0))
        scancode = kKeyDel;
      else if (ustricmp(tok, "Home") == 0)
        scancode = kKeyHome;
      else if (ustricmp(tok, "End") == 0)
        scancode = kKeyEnd;
      else if ((ustricmp(tok, "Page Up") == 0) ||
               (ustricmp(tok, "PgUp") == 0))
        scancode = kKeyPageUp;
      else if ((ustricmp(tok, "Page Down") == 0) ||
               (ustricmp(tok, "PgDn") == 0))
        scancode = kKeyPageDown;
      else if (ustricmp(tok, "Left") == 0)
        scancode = kKeyLeft;
      else if (ustricmp(tok, "Right") == 0)
        scancode = kKeyRight;
      else if (ustricmp(tok, "Up") == 0)
        scancode = kKeyUp;
      else if (ustricmp(tok, "Down") == 0)
        scancode = kKeyDown;
      else if (ustricmp(tok, "0 Pad") == 0)
        scancode = kKey0Pad;
      else if (ustricmp(tok, "1 Pad") == 0)
        scancode = kKey1Pad;
      else if (ustricmp(tok, "2 Pad") == 0)
        scancode = kKey2Pad;
      else if (ustricmp(tok, "3 Pad") == 0)
        scancode = kKey3Pad;
      else if (ustricmp(tok, "4 Pad") == 0)
        scancode = kKey4Pad;
      else if (ustricmp(tok, "5 Pad") == 0)
        scancode = kKey5Pad;
      else if (ustricmp(tok, "6 Pad") == 0)
        scancode = kKey6Pad;
      else if (ustricmp(tok, "7 Pad") == 0)
        scancode = kKey7Pad;
      else if (ustricmp(tok, "8 Pad") == 0)
        scancode = kKey8Pad;
      else if (ustricmp(tok, "9 Pad") == 0)
        scancode = kKey9Pad;
      else if (ustricmp(tok, "Slash Pad") == 0)
        scancode = kKeySlashPad;
      else if (ustricmp(tok, "Asterisk") == 0)
        scancode = kKeyAsterisk;
      else if (ustricmp(tok, "Minus Pad") == 0)
        scancode = kKeyMinusPad;
      else if (ustricmp(tok, "Plus Pad") == 0)
        scancode = kKeyPlusPad;
      else if (ustricmp(tok, "Del Pad") == 0)
        scancode = kKeyDelPad;
      else if (ustricmp(tok, "Enter Pad") == 0)
        scancode = kKeyEnterPad;
    }
  }

  accel->addKey(modifiers, scancode, ascii);
}

void Accelerator::addKeysFromString(const char* string)
{
  char *s, *begin, buf[256];
  int backup;

  ustrcpy(buf, string);

  for (s=buf; *s; ) {
    if (*s == '<') {
      begin = ++s;

      while ((*s) && (*s != '>')) s++;

      backup = *s;
      *s = 0;

      process_one_word(this, begin);

      *s = backup;
    }
    else {
      s++;
    }
  }
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

  char buf[256];
  ustrcpy(buf, "");

  // Shifts
  if (this->modifiers & kKeyCtrlModifier)
    ustrcat(buf, "Ctrl+");

  if (this->modifiers & kKeyAltModifier)
    ustrcat(buf, "Alt+");

  if (this->modifiers & kKeyShiftModifier)
    ustrcat(buf, "Shift+");

  // Key
  if (this->ascii)
    usprintf(buf+ustrlen(buf), "%c", toupper(this->ascii));
  else if (this->scancode)
    ustrcat(buf, table[this->scancode]);
  else
    ustrcat(buf, "Unknown");

  return buf;
}

std::string Accelerator::toString()
{
  ASSERT(!m_combos.empty());
  return m_combos.front().toString();
}

bool Accelerator::check(KeyModifiers modifiers, KeyScancode scancode, int ascii)
{
#ifdef REPORT_KEYS
  char buf[256];
  std::string buf2;
#endif

  // Preprocess the character to be compared with the accelerator
#ifdef PREPROCESS_KEYS
  // Directly scancode
  if ((scancode >= kKeyF1 && scancode <= KEY_F12) ||
      (scancode == kKeyEsc) ||
      (scancode == kKeyBackspace) ||
      (scancode == kKeyTab) ||
      (scancode == kKeyEnter) ||
      (scancode == kKeyBackslash) ||
      (scancode == kKeyBackslash2) ||
      (scancode >= kKeySpace && scancode <= kKeyDown) ||
      (scancode >= kKeyEnterPad && scancode <= kKeyNoconvert) ||
      (scancode == kKeyKanji)) {
    ascii = 0;
  }
  // For Ctrl+number
  /*           scancode    ascii
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
           (ascii < 32 || ascii == 127)) {
    ascii = '0' + scancode - kKey0;
    scancode = kKeyNil;
  }
  // For Ctrl+letter
  else if (ascii >= 1 && ascii <= 'z'-'a'+1) {
    ascii = 'a'+ascii-1;
    scancode = kKeyNil;
  }
  // For any other legal ASCII code
  else if (ascii >= ' ') {
    ascii = tolower(ascii);

    /* without shift (because characters like '*' can be trigger with
       "Shift+8", so we don't want "Shift+*") */
    if (!(ascii >= 'a' && ascii <= 'z'))
      modifiers = (KeyModifiers)((int)modifiers & ((int)~kKeyShiftModifier));

    scancode = kKeyNil;
  }
#endif

#ifdef REPORT_KEYS
  {
    base::UniquePtr<Accelerator> a2(new Accelerator);
    a2->addKey(modifiers, scancode, ascii);
    buf2 = a2->getString();
  }
#endif

  for (KeyCombos::iterator it = m_combos.begin(), end = m_combos.end();
       it != end; ++it) {
#ifdef REPORT_KEYS
    printf("%3d==%3d %3d==%3d %s==%s ",
           it->scancode, scancode, it->ascii, ascii,
           it->getString().c_str(), buf2.c_str();
#endif

    if ((it->modifiers == modifiers) &&
        ((it->scancode != kKeyNil && it->scancode == scancode) ||
         (it->ascii && it->ascii == ascii))) {

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

  if (key[KEY_LSHIFT]  ) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyShiftModifier);
  if (key[KEY_RSHIFT]  ) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyShiftModifier);
  if (key[KEY_LCONTROL]) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyCtrlModifier);
  if (key[KEY_RCONTROL]) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyCtrlModifier);
  if (key[KEY_ALT]     ) modifiers = (KeyModifiers)((int)modifiers | (int)kKeyAltModifier);

  for (KeyCombos::iterator it = m_combos.begin(), end = m_combos.end();
       it != end; ++it) {
    if ((it->scancode == 0 || key[it->scancode]) &&
        (it->modifiers == modifiers)) {
      return true;
    }
  }
  return false;
}

} // namespace ui
