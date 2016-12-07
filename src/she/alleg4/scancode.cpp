// SHE library
// Copyright (C) 2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/alleg4/scancode.h"

#include <allegro.h>

namespace she {

KeyScancode alleg_to_she_scancode(int scancode)
{
  static KeyScancode table[] = {
    kKeyNil,
    kKeyA, // KEY_A
    kKeyB, // KEY_B
    kKeyC, // KEY_C
    kKeyD, // KEY_D
    kKeyE, // KEY_E
    kKeyF, // KEY_F
    kKeyG, // KEY_G
    kKeyH, // KEY_H
    kKeyI, // KEY_I
    kKeyJ, // KEY_J
    kKeyK, // KEY_K
    kKeyL, // KEY_L
    kKeyM, // KEY_M
    kKeyN, // KEY_N
    kKeyO, // KEY_O
    kKeyP, // KEY_P
    kKeyQ, // KEY_Q
    kKeyR, // KEY_R
    kKeyS, // KEY_S
    kKeyT, // KEY_T
    kKeyU, // KEY_U
    kKeyV, // KEY_V
    kKeyW, // KEY_W
    kKeyX, // KEY_X
    kKeyY, // KEY_Y
    kKeyZ, // KEY_Z
    kKey0, // KEY_0
    kKey1, // KEY_1
    kKey2, // KEY_2
    kKey3, // KEY_3
    kKey4, // KEY_4
    kKey5, // KEY_5
    kKey6, // KEY_6
    kKey7, // KEY_7
    kKey8, // KEY_8
    kKey9, // KEY_9
    kKey0Pad, // KEY_0_PAD
    kKey1Pad, // KEY_1_PAD
    kKey2Pad, // KEY_2_PAD
    kKey3Pad, // KEY_3_PAD
    kKey4Pad, // KEY_4_PAD
    kKey5Pad, // KEY_5_PAD
    kKey6Pad, // KEY_6_PAD
    kKey7Pad, // KEY_7_PAD
    kKey8Pad, // KEY_8_PAD
    kKey9Pad, // KEY_9_PAD
    kKeyF1, // KEY_F1
    kKeyF2, // KEY_F2
    kKeyF3, // KEY_F3
    kKeyF4, // KEY_F4
    kKeyF5, // KEY_F5
    kKeyF6, // KEY_F6
    kKeyF7, // KEY_F7
    kKeyF8, // KEY_F8
    kKeyF9, // KEY_F9
    kKeyF10, // KEY_F10
    kKeyF11, // KEY_F11
    kKeyF12, // KEY_F12
    kKeyEsc, // KEY_ESC
    kKeyTilde, // KEY_TILDE
    kKeyMinus, // KEY_MINUS
    kKeyEquals, // KEY_EQUALS
    kKeyBackspace, // KEY_BACKSPACE
    kKeyTab, // KEY_TAB
    kKeyOpenbrace, // KEY_OPENBRACE
    kKeyClosebrace, // KEY_CLOSEBRACE
    kKeyEnter, // KEY_ENTER
    kKeyColon, // KEY_COLON
    kKeyQuote, // KEY_QUOTE
    kKeyBackslash, // KEY_BACKSLASH
    kKeyBackslash2, // KEY_BACKSLASH2
    kKeyComma, // KEY_COMMA
    kKeyStop, // KEY_STOP
    kKeySlash, // KEY_SLASH
    kKeySpace, // KEY_SPACE
    kKeyInsert, // KEY_INSERT
    kKeyDel, // KEY_DEL
    kKeyHome, // KEY_HOME
    kKeyEnd, // KEY_END
    kKeyPageUp, // KEY_PGUP
    kKeyPageDown, // KEY_PGDN
    kKeyLeft, // KEY_LEFT
    kKeyRight, // KEY_RIGHT
    kKeyUp, // KEY_UP
    kKeyDown, // KEY_DOWN
    kKeySlashPad, // KEY_SLASH_PAD
    kKeyAsterisk, // KEY_ASTERISK
    kKeyMinusPad, // KEY_MINUS_PAD
    kKeyPlusPad, // KEY_PLUS_PAD
    kKeyDelPad, // KEY_DEL_PAD
    kKeyEnterPad, // KEY_ENTER_PAD
    kKeyPrtscr, // KEY_PRTSCR
    kKeyPause, // KEY_PAUSE
    kKeyAbntC1, // KEY_ABNT_C1
    kKeyYen, // KEY_YEN
    kKeyKana, // KEY_KANA
    kKeyConvert, // KEY_CONVERT
    kKeyNoconvert, // KEY_NOCONVERT
    kKeyAt, // KEY_AT
    kKeyCircumflex, // KEY_CIRCUMFLEX
    kKeyColon2, // KEY_COLON2
    kKeyKanji, // KEY_KANJI
    kKeyEqualsPad, // KEY_EQUALS_PAD
    kKeyBackquote, // KEY_BACKQUOTE
    kKeySemicolon, // KEY_SEMICOLON
    kKeyCommand, // KEY_COMMAND
    kKeyUnknown1, // KEY_UNKNOWN1
    kKeyUnknown2, // KEY_UNKNOWN2
    kKeyUnknown3, // KEY_UNKNOWN3
    kKeyUnknown4, // KEY_UNKNOWN4
    kKeyUnknown5, // KEY_UNKNOWN5
    kKeyUnknown6, // KEY_UNKNOWN6
    kKeyUnknown7, // KEY_UNKNOWN7
    kKeyNil, // KEY_UNKNOWN8
    kKeyLShift, // KEY_LSHIFT
    kKeyRShift, // KEY_RSHIFT
    kKeyLControl, // KEY_LCONTROL
    kKeyRControl, // KEY_RCONTROL
    kKeyAlt, // KEY_ALT
    kKeyAltGr, // KEY_ALTGR
    kKeyLWin, // KEY_LWIN
    kKeyRWin, // KEY_RWIN
    kKeyMenu, // KEY_MENU
    kKeyScrLock, // KEY_SCRLOCK
    kKeyNumLock, // KEY_NUMLOCK
    kKeyCapsLock, // KEY_CAPSLOCK
  };
  if (scancode >= 0 && scancode < int(sizeof(table) / sizeof(table[0])))
    return table[scancode];
  else
    return kKeyNil;
}

int she_to_alleg_scancode(KeyScancode scancode)
{
  static int table[] = {
    0,
    KEY_A, // kKeyA
    KEY_B, // kKeyB
    KEY_C, // kKeyC
    KEY_D, // kKeyD
    KEY_E, // kKeyE
    KEY_F, // kKeyF
    KEY_G, // kKeyG
    KEY_H, // kKeyH
    KEY_I, // kKeyI
    KEY_J, // kKeyJ
    KEY_K, // kKeyK
    KEY_L, // kKeyL
    KEY_M, // kKeyM
    KEY_N, // kKeyN
    KEY_O, // kKeyO
    KEY_P, // kKeyP
    KEY_Q, // kKeyQ
    KEY_R, // kKeyR
    KEY_S, // kKeyS
    KEY_T, // kKeyT
    KEY_U, // kKeyU
    KEY_V, // kKeyV
    KEY_W, // kKeyW
    KEY_X, // kKeyX
    KEY_Y, // kKeyY
    KEY_Z, // kKeyZ
    KEY_0, // kKey0
    KEY_1, // kKey1
    KEY_2, // kKey2
    KEY_3, // kKey3
    KEY_4, // kKey4
    KEY_5, // kKey5
    KEY_6, // kKey6
    KEY_7, // kKey7
    KEY_8, // kKey8
    KEY_9, // kKey9
    KEY_0_PAD, // kKey0Pad
    KEY_1_PAD, // kKey1Pad
    KEY_2_PAD, // kKey2Pad
    KEY_3_PAD, // kKey3Pad
    KEY_4_PAD, // kKey4Pad
    KEY_5_PAD, // kKey5Pad
    KEY_6_PAD, // kKey6Pad
    KEY_7_PAD, // kKey7Pad
    KEY_8_PAD, // kKey8Pad
    KEY_9_PAD, // kKey9Pad
    KEY_F1, // kKeyF1
    KEY_F2, // kKeyF2
    KEY_F3, // kKeyF3
    KEY_F4, // kKeyF4
    KEY_F5, // kKeyF5
    KEY_F6, // kKeyF6
    KEY_F7, // kKeyF7
    KEY_F8, // kKeyF8
    KEY_F9, // kKeyF9
    KEY_F10, // kKeyF10
    KEY_F11, // kKeyF11
    KEY_F12, // kKeyF12
    KEY_ESC, // kKeyEsc
    KEY_TILDE, // kKeyTilde
    KEY_MINUS, // kKeyMinus
    KEY_EQUALS, // kKeyEquals
    KEY_BACKSPACE, // kKeyBackspace
    KEY_TAB, // kKeyTab
    KEY_OPENBRACE, // kKeyOpenbrace
    KEY_CLOSEBRACE, // kKeyClosebrace
    KEY_ENTER, // kKeyEnter
    KEY_COLON, // kKeyColon
    KEY_QUOTE, // kKeyQuote
    KEY_BACKSLASH, // kKeyBackslash
    KEY_BACKSLASH2, // kKeyBackslash2
    KEY_COMMA, // kKeyComma
    KEY_STOP, // kKeyStop
    KEY_SLASH, // kKeySlash
    KEY_SPACE, // kKeySpace
    KEY_INSERT, // kKeyInsert
    KEY_DEL, // kKeyDel
    KEY_HOME, // kKeyHome
    KEY_END, // kKeyEnd
    KEY_PGUP, // kKeyPageUp
    KEY_PGDN, // kKeyPageDown
    KEY_LEFT, // kKeyLeft
    KEY_RIGHT, // kKeyRight
    KEY_UP, // kKeyUp
    KEY_DOWN, // kKeyDown
    KEY_SLASH_PAD, // kKeySlashPad
    KEY_ASTERISK, // kKeyAsterisk
    KEY_MINUS_PAD, // kKeyMinusPad
    KEY_PLUS_PAD, // kKeyPlusPad
    KEY_DEL_PAD, // kKeyDelPad
    KEY_ENTER_PAD, // kKeyEnterPad
    KEY_PRTSCR, // kKeyPrtscr
    KEY_PAUSE, // kKeyPause
    KEY_ABNT_C1, // kKeyAbntC1
    KEY_YEN, // kKeyYen
    KEY_KANA, // kKeyKana
    KEY_CONVERT, // kKeyConvert
    KEY_NOCONVERT, // kKeyNoconvert
    KEY_AT, // kKeyAt
    KEY_CIRCUMFLEX, // kKeyCircumflex
    KEY_COLON2, // kKeyColon2
    KEY_KANJI, // kKeyKanji
    KEY_EQUALS_PAD, // kKeyEqualsPad
    KEY_BACKQUOTE, // kKeyBackquote
    KEY_SEMICOLON, // kKeySemicolon
    KEY_UNKNOWN1, // kKeyUnknown1
    KEY_UNKNOWN2, // kKeyUnknown2
    KEY_UNKNOWN3, // kKeyUnknown3
    KEY_UNKNOWN4, // kKeyUnknown4
    KEY_UNKNOWN5, // kKeyUnknown5
    KEY_UNKNOWN6, // kKeyUnknown6
    KEY_UNKNOWN7, // kKeyUnknown7
    KEY_UNKNOWN8, // kKeyUnknown8
    KEY_LSHIFT, // kKeyLShift
    KEY_RSHIFT, // kKeyRShift
    KEY_LCONTROL, // kKeyLControl
    KEY_RCONTROL, // kKeyRControl
    KEY_ALT, // kKeyAlt
    KEY_ALTGR, // kKeyAltGr
    KEY_LWIN, // kKeyLWin
    KEY_RWIN, // kKeyRWin
    KEY_MENU, // kKeyMenu
    KEY_COMMAND, // kKeyCommand
    KEY_SCRLOCK, // kKeyScrLock
    KEY_NUMLOCK, // kKeyNumLock
    KEY_CAPSLOCK, // kKeyCapsLock
  };
  if (scancode >= 0 && scancode < int(sizeof(table) / sizeof(table[0])))
    return table[scancode];
  else
    return kKeyNil;
}

} // namespace she
