/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <allegro/keyboard.h>
#include <allegro/unicode.h>
#include <ctype.h>

#include "jinete/jaccel.h"
#include "jinete/jlist.h"

/* #define REPORT_KEYS */
#define PREPROCESS_KEYS



struct jaccel
{
  JList key_list;
};

typedef struct KeyCombo
{
  int shifts;
  int ascii;
  int scancode;
} KeyCombo;

JAccel jaccel_new()
{
  JAccel accel;

  accel = jnew(struct jaccel, 1);
  if (!accel)
    return NULL;

  accel->key_list = jlist_new();

  return accel;
}

JAccel jaccel_new_copy(JAccel accel)
{
  KeyCombo *key;
  JAccel copy;
  JLink link;

  copy = jaccel_new();
  if (!copy)
    return NULL;

  JI_LIST_FOR_EACH(accel->key_list, link) {
    key = (KeyCombo *)link->data;
    jaccel_add_key(copy, key->shifts, key->ascii, key->scancode);
  }

  return copy;
}

void jaccel_free(JAccel accel)
{
  JLink link;
  JI_LIST_FOR_EACH(accel->key_list, link) {
    jfree(link->data);
  }
  jlist_free(accel->key_list);
  jfree(accel);
}

void jaccel_add_key(JAccel accel, int shifts, int ascii, int scancode)
{
  KeyCombo *key;

  key = jnew(KeyCombo, 1);
  if (!key)
    return;

  key->shifts = shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG);
  key->ascii = ascii;
  key->scancode = scancode;

  jlist_append(accel->key_list, key);
}

static void proc_one_word(JAccel accel, char* word)
{
  int shifts = 0;
  int ascii = 0;
  int scancode = 0;
  char* tok;

  // Special case: plus sign
  if (word[0] == '+' && word[1] == 0) {
    jaccel_add_key(accel, 0, '+', 0);
    return;
  }

  for (tok=ustrtok(word, "+"); tok;
       tok=ustrtok(NULL, "+")) {
    // key_shifts

    if (ustricmp (tok, "Shift") == 0) {
      shifts |= KB_SHIFT_FLAG;
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

  if (ascii || scancode)
    jaccel_add_key(accel, shifts, ascii, scancode);
}

/* process strings like "<Ctrl+Q> <ESC>" */
void jaccel_add_keys_from_string(JAccel accel, const char *string)
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

      proc_one_word(accel, begin);

      *s = backup;
    }
    else {
      s++;
    }
  }
}

bool jaccel_is_empty(JAccel accel)
{
  return jlist_empty(accel->key_list);
}

static void keycombo_get_string(KeyCombo *key, char *buf)
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

  ustrcpy(buf, "");

  if (!key)
    return;

  /* shifts */
  if (key->shifts & KB_CTRL_FLAG)
    ustrcat(buf, "Ctrl+");

  if (key->shifts & KB_SHIFT_FLAG)
    ustrcat(buf, "Shift+");

  /* key */
  if (key->ascii)
    usprintf(buf+ustrlen(buf), "%c", toupper(key->ascii));
  else if (key->scancode)
    ustrcat(buf, table[key->scancode]);
  else
    ustrcat(buf, "Unknown");
}

void jaccel_to_string(JAccel accel, char *buf)
{
  keycombo_get_string(reinterpret_cast<KeyCombo*>(jlist_first(accel->key_list)->data), buf);
}

bool jaccel_check(JAccel accel, int shifts, int ascii, int scancode)
{
  KeyCombo *key;
  JLink link;
#ifdef REPORT_KEYS
  char buf[256];
  char buf2[256];
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
    JAccel *a2 = jaccel_new();
    jaccel_add_key(a2, shifts, ascii, scancode);
    jaccel_get_string(a2, buf2);
    jaccel_free(a2);
  }
#endif

  JI_LIST_FOR_EACH(accel->key_list, link) {
    key = (KeyCombo *)link->data;
#ifdef REPORT_KEYS
    keycombo_get_string(key, buf);
    printf("%3d==%3d %3d==%3d %s==%s ",
	   key->scancode, scancode, key->ascii, ascii, buf, buf2);
#endif
    if (((key->scancode && key->scancode == scancode)
	 || (key->ascii && key->ascii == ascii))
	&& (key->shifts == (shifts & (KB_SHIFT_FLAG | KB_CTRL_FLAG)))) {
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
