// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include "ui/accelerator.h"

#include "base/unique_ptr.h"

#include <allegro/keyboard.h>
#include <allegro/unicode.h>
#include <ctype.h>

/* #define REPORT_KEYS */
#define PREPROCESS_KEYS

namespace ui {

void Accelerator::addKey(int shifts, int ascii, int scancode)
{
  KeyCombo key;

  key.shifts = shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG);
  key.ascii = ascii;
  key.scancode = scancode;

  m_combos.push_back(key);
}

static void process_one_word(Accelerator* accel, char* word)
{
  int shifts = 0;
  int ascii = 0;
  int scancode = 0;
  char* tok;

  // Special case: plus sign
  if (word[0] == '+' && word[1] == 0) {
    accel->addKey(0, '+', 0);
    return;
  }

  for (tok=ustrtok(word, "+"); tok;
       tok=ustrtok(NULL, "+")) {
    // modifiers

    if (ustricmp (tok, "Shift") == 0) {
      shifts |= KB_SHIFT_FLAG;
    }
    else if (ustricmp (tok, "Alt") == 0) {
      shifts |= KB_ALT_FLAG;
    }
    else if (ustricmp (tok, "Ctrl") == 0) {
      shifts |= KB_CTRL_FLAG;
    }

    // scancode

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
        scancode = KEY_A + tolower(*tok) - 'a';
      else if ((*tok >= '0') && (*tok <= '9'))
        scancode = KEY_0 + *tok - '0';
      else {
        switch (*tok) {
          case '~': scancode = KEY_TILDE; break;
          case '-': scancode = KEY_MINUS; break;
          case '=': scancode = KEY_EQUALS; break;
          case '[': scancode = KEY_OPENBRACE; break;
          case ']': scancode = KEY_CLOSEBRACE; break;
          case ';': scancode = KEY_COLON; break;
          case '\'': scancode = KEY_QUOTE; break;
          case '\\': scancode = KEY_BACKSLASH; break;
          case ',': scancode = KEY_COMMA; break;
          case '.': scancode = KEY_STOP; break;
          case '/': scancode = KEY_SLASH; break;
          case '*': scancode = KEY_ASTERISK; break;
        }
      }
    }
    /* other ones */
    else {
      /* F1, F2, ..., F11, F12 */
      if ((toupper (*tok) == 'F') && (ustrlen(tok) <= 3)) {
        int num = ustrtol(tok+1, NULL, 10);
        if ((num >= 1) && (num <= 12))
          scancode = KEY_F1 + num - 1;
      }
      else if ((ustricmp(tok, "Escape") == 0) ||
               (ustricmp(tok, "Esc") == 0))
        scancode = KEY_ESC;
      else if (ustricmp(tok, "Backspace") == 0)
        scancode = KEY_BACKSPACE;
      else if (ustricmp(tok, "Tab") == 0)
        scancode = KEY_TAB;
      else if (ustricmp(tok, "Enter") == 0)
        scancode = KEY_ENTER;
      else if (ustricmp(tok, "Space") == 0)
        scancode = KEY_SPACE;
      else if ((ustricmp(tok, "Insert") == 0) ||
               (ustricmp(tok, "Ins") == 0))
        scancode = KEY_INSERT;
      else if ((ustricmp(tok, "Delete") == 0) ||
               (ustricmp(tok, "Del") == 0))
        scancode = KEY_DEL;
      else if (ustricmp(tok, "Home") == 0)
        scancode = KEY_HOME;
      else if (ustricmp(tok, "End") == 0)
        scancode = KEY_END;
      else if ((ustricmp(tok, "Page Up") == 0) ||
               (ustricmp(tok, "PgUp") == 0))
        scancode = KEY_PGUP;
      else if ((ustricmp(tok, "Page Down") == 0) ||
               (ustricmp(tok, "PgDn") == 0))
        scancode = KEY_PGDN;
      else if (ustricmp(tok, "Left") == 0)
        scancode = KEY_LEFT;
      else if (ustricmp(tok, "Right") == 0)
        scancode = KEY_RIGHT;
      else if (ustricmp(tok, "Up") == 0)
        scancode = KEY_UP;
      else if (ustricmp(tok, "Down") == 0)
        scancode = KEY_DOWN;
      else if (ustricmp(tok, "0 Pad") == 0)
        scancode = KEY_0_PAD;
      else if (ustricmp(tok, "1 Pad") == 0)
        scancode = KEY_1_PAD;
      else if (ustricmp(tok, "2 Pad") == 0)
        scancode = KEY_2_PAD;
      else if (ustricmp(tok, "3 Pad") == 0)
        scancode = KEY_3_PAD;
      else if (ustricmp(tok, "4 Pad") == 0)
        scancode = KEY_4_PAD;
      else if (ustricmp(tok, "5 Pad") == 0)
        scancode = KEY_5_PAD;
      else if (ustricmp(tok, "6 Pad") == 0)
        scancode = KEY_6_PAD;
      else if (ustricmp(tok, "7 Pad") == 0)
        scancode = KEY_7_PAD;
      else if (ustricmp(tok, "8 Pad") == 0)
        scancode = KEY_8_PAD;
      else if (ustricmp(tok, "9 Pad") == 0)
        scancode = KEY_9_PAD;
      else if (ustricmp(tok, "Slash Pad") == 0)
        scancode = KEY_SLASH_PAD;
      else if (ustricmp(tok, "Asterisk") == 0)
        scancode = KEY_ASTERISK;
      else if (ustricmp(tok, "Minus Pad") == 0)
        scancode = KEY_MINUS_PAD;
      else if (ustricmp(tok, "Plus Pad") == 0)
        scancode = KEY_PLUS_PAD;
      else if (ustricmp(tok, "Del Pad") == 0)
        scancode = KEY_DEL_PAD;
      else if (ustricmp(tok, "Enter Pad") == 0)
        scancode = KEY_ENTER_PAD;
    }
  }

  accel->addKey(shifts, ascii, scancode);
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
  /* same order that Allegro scancodes */
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
  if (this->shifts & KB_CTRL_FLAG)
    ustrcat(buf, "Ctrl+");

  if (this->shifts & KB_ALT_FLAG)
    ustrcat(buf, "Alt+");

  if (this->shifts & KB_SHIFT_FLAG)
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

bool Accelerator::check(int shifts, int ascii, int scancode)
{
#ifdef REPORT_KEYS
  char buf[256];
  std::string buf2;
#endif

  /* preprocess the character to be compared with the accelerator */
#ifdef PREPROCESS_KEYS
  /* directly scancode */
  if ((scancode >= KEY_F1 && scancode <= KEY_F12) ||
      (scancode == KEY_ESC) ||
      (scancode == KEY_BACKSPACE) ||
      (scancode == KEY_TAB) ||
      (scancode == KEY_ENTER) ||
      (scancode == KEY_BACKSLASH) ||
      (scancode == KEY_BACKSLASH2) ||
      (scancode >= KEY_SPACE && scancode <= KEY_DOWN) ||
      (scancode >= KEY_ENTER_PAD && scancode <= KEY_NOCONVERT) ||
      (scancode == KEY_KANJI)) {
    ascii = 0;
  }
  /* for Ctrl+number */
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
  else if ((scancode >= KEY_0 && scancode <= KEY_9) &&
           (ascii < 32 || ascii == 127)) {
    ascii = '0'+scancode-KEY_0;
    scancode = 0;
  }
  /* for Ctrl+letter */
  else if (ascii >= 1 && ascii <= 'z'-'a'+1) {
    ascii = 'a'+ascii-1;
    scancode = 0;
  }
  /* for any other legal ASCII code */
  else if (ascii >= ' ') {
    ascii = tolower(ascii);

    /* without shift (because characters like '*' can be trigger with
       "Shift+8", so we don't want "Shift+*") */
    if (!(ascii >= 'a' && ascii <= 'z'))
      shifts &= ~KB_SHIFT_FLAG;

    scancode = 0;
  }
#endif

#ifdef REPORT_KEYS
  {
    UniquePtr<Accelerator> a2(new Accelerator);
    a2->addKey(shifts, ascii, scancode);
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

    if (((it->scancode && it->scancode == scancode)
         || (it->ascii && it->ascii == ascii))
        && (it->shifts == (shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG)))) {

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
  int shifts = 0;

  if (key[KEY_LSHIFT]) shifts |= KB_SHIFT_FLAG;
  if (key[KEY_RSHIFT]) shifts |= KB_SHIFT_FLAG;
  if (key[KEY_LCONTROL]) shifts |= KB_CTRL_FLAG;
  if (key[KEY_RCONTROL]) shifts |= KB_CTRL_FLAG;
  if (key[KEY_ALT]) shifts |= KB_ALT_FLAG;

  for (KeyCombos::iterator it = m_combos.begin(), end = m_combos.end();
       it != end; ++it) {
    if ((it->scancode == 0 || key[it->scancode]) &&
        (it->shifts == (shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG)))) {
      return true;
    }
  }
  return false;
}

} // namespace ui
