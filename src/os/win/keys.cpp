// LAF OS Library
// Copyright (C) 2012-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/win/keys.h"

namespace os {

KeyScancode win32vk_to_scancode(int vk)
{
  static KeyScancode keymap[256] = {
    // 0x00
    kKeyNil, // 0x00
    kKeyNil, // 0x01 - VK_LBUTTON
    kKeyNil, // 0x02 - VK_RBUTTON
    kKeyNil, // 0x03 - VK_CANCEL
    kKeyNil, // 0x04 - VK_MBUTTON
    kKeyNil, // 0x05 - VK_XBUTTON1
    kKeyNil, // 0x06 - VK_XBUTTON2
    kKeyNil, // 0x07 - Unassigned
    kKeyBackspace, // 0x08 - VK_BACK
    kKeyTab, // 0x09 - VK_TAB
    kKeyNil, // 0x0A - Reserved
    kKeyNil, // 0x0B - Reserved
    kKeyNil, // 0x0C - VK_CLEAR
    kKeyEnter, // 0x0D - VK_RETURN
    kKeyNil, // 0x0E - Undefined
    kKeyNil, // 0x0F - Undefined
    // 0x10
    kKeyLShift, // 0x10 - VK_SHIFT
    kKeyLControl, // 0x11 - VK_CONTROL
    kKeyAlt, // 0x12 - VK_MENU
    kKeyPause, // 0x13 - VK_PAUSE
    kKeyCapsLock, // 0x14 - VK_CAPITAL
    kKeyKana, // 0x15 - VK_KANA
    kKeyNil, // 0x16 - Undefined
    kKeyNil, // 0x17 - VK_JUNJA
    kKeyNil, // 0x18 - VK_FINAL
    kKeyKanji, // 0x19 - VK_KANJI
    kKeyNil, // 0x1A - Unknown
    kKeyEsc, // 0x1B - VK_ESCAPE
    kKeyConvert, // 0x1C - VK_CONVERT
    kKeyNoconvert, // 0x1D - VK_NONCONVERT
    kKeyNil, // 0x1E - VK_ACCEPT
    kKeyNil, // 0x1F - VK_MODECHANGE
    // 0x20
    kKeySpace, // 0x20 - VK_SPACE
    kKeyPageUp, // 0x21 - VK_PRIOR
    kKeyPageDown, // 0x22 - VK_NEXT
    kKeyEnd, // 0x23 - VK_END
    kKeyHome, // 0x24 - VK_HOME
    kKeyLeft, // 0x25 - VK_LEFT
    kKeyUp, // 0x26 - VK_UP
    kKeyRight, // 0x27 - VK_RIGHT
    kKeyDown, // 0x28 - VK_DOWN
    kKeyNil, // 0x29 - VK_SELECT
    kKeyNil, // 0x2A - VK_PRINT
    kKeyNil, // 0x2B - VK_EXECUTE
    kKeyPrtscr, // 0x2C - VK_SNAPSHOT
    kKeyInsert, // 0x2D - VK_INSERT
    kKeyDel, // 0x2E - VK_DELETE
    kKeyNil, // 0x2F - VK_HELP
    // 0x30
    kKey0, // 0x30 - VK_0
    kKey1, // 0x31 - VK_1
    kKey2, // 0x32 - VK_2
    kKey3, // 0x33 - VK_3
    kKey4, // 0x34 - VK_4
    kKey5, // 0x35 - VK_5
    kKey6, // 0x36 - VK_6
    kKey7, // 0x37 - VK_7
    kKey8, // 0x38 - VK_8
    kKey9, // 0x39 - VK_9
    kKeyNil, // 0x3A - Unassigned
    kKeyNil, // 0x3B - Unassigned
    kKeyNil, // 0x3C - Unassigned
    kKeyNil, // 0x3D - Unassigned
    kKeyNil, // 0x3E - Unassigned
    kKeyNil, // 0x3F - Unassigned
    // 0x40
    kKeyNil, // 0x40 - Unassigned
    kKeyA, // 0x41 - VK_A
    kKeyB, // 0x42 - VK_B
    kKeyC, // 0x43 - VK_C
    kKeyD, // 0x44 - VK_D
    kKeyE, // 0x45 - VK_E
    kKeyF, // 0x46 - VK_F
    kKeyG, // 0x47 - VK_G
    kKeyH, // 0x48 - VK_H
    kKeyI, // 0x49 - VK_I
    kKeyJ, // 0x4A - VK_J
    kKeyK, // 0x4B - VK_K
    kKeyL, // 0x4C - VK_L
    kKeyM, // 0x4D - VK_M
    kKeyN, // 0x4E - VK_N
    kKeyO, // 0x4F - VK_O
    // 0x50
    kKeyP, // 0x50 - VK_P
    kKeyQ, // 0x51 - VK_Q
    kKeyR, // 0x52 - VK_R
    kKeyS, // 0x53 - VK_S
    kKeyT, // 0x54 - VK_T
    kKeyU, // 0x55 - VK_U
    kKeyV, // 0x56 - VK_V
    kKeyW, // 0x57 - VK_W
    kKeyX, // 0x58 - VK_X
    kKeyY, // 0x59 - VK_Y
    kKeyZ, // 0x5A - VK_Z
    kKeyLWin, // 0x5B - VK_LWIN
    kKeyRWin, // 0x5C - VK_RWIN
    kKeyMenu, // 0x5D - VK_APPS
    kKeyNil, // 0x5E - Reserved
    kKeyNil, // 0x5F - VK_SLEEP
    // 0x60
    kKey0Pad, // 0x60 - VK_NUMPAD0
    kKey1Pad, // 0x61 - VK_NUMPAD1
    kKey2Pad, // 0x62 - VK_NUMPAD2
    kKey3Pad, // 0x63 - VK_NUMPAD3
    kKey4Pad, // 0x64 - VK_NUMPAD4
    kKey5Pad, // 0x65 - VK_NUMPAD5
    kKey6Pad, // 0x66 - VK_NUMPAD6
    kKey7Pad, // 0x67 - VK_NUMPAD7
    kKey8Pad, // 0x68 - VK_NUMPAD8
    kKey9Pad, // 0x69 - VK_NUMPAD9
    kKeyAsterisk, // 0x6A - VK_MULTIPLY
    kKeyPlusPad, // 0x6B - VK_ADD
    kKeyNil, // 0x6C - VK_SEPARATOR
    kKeyMinusPad, // 0x6D - VK_SUBTRACT
    kKeyNil, // 0x6E - VK_DECIMAL
    kKeySlashPad, // 0x6F - VK_DIVIDE
    // 0x70
    kKeyF1, // 0x70 - VK_F1
    kKeyF2, // 0x71 - VK_F2
    kKeyF3, // 0x72 - VK_F3
    kKeyF4, // 0x73 - VK_F4
    kKeyF5, // 0x74 - VK_F5
    kKeyF6, // 0x75 - VK_F6
    kKeyF7, // 0x76 - VK_F7
    kKeyF8, // 0x77 - VK_F8
    kKeyF9, // 0x78 - VK_F9
    kKeyF10, // 0x79 - VK_F10
    kKeyF11, // 0x7A - VK_F11
    kKeyF12, // 0x7B - VK_F12
    kKeyNil, // 0x7C - VK_F13
    kKeyNil, // 0x7D - VK_F14
    kKeyNil, // 0x7E - VK_F15
    kKeyNil, // 0x7F - VK_F16
    // 0x80
    kKeyNil, // 0x80 - VK_F17
    kKeyNil, // 0x81 - VK_F18
    kKeyNil, // 0x82 - VK_F19
    kKeyNil, // 0x83 - VK_F20
    kKeyNil, // 0x84 - VK_F21
    kKeyNil, // 0x85 - VK_F22
    kKeyNil, // 0x86 - VK_F23
    kKeyNil, // 0x87 - VK_F24
    kKeyNil, // 0x88 - Unassigned
    kKeyNil, // 0x89 - Unassigned
    kKeyNil, // 0x8A - Unassigned
    kKeyNil, // 0x8B - Unassigned
    kKeyNil, // 0x8C - Unassigned
    kKeyNil, // 0x8D - Unassigned
    kKeyNil, // 0x8E - Unassigned
    kKeyNil, // 0x8F - Unassigned
    // 0x90
    kKeyNumLock, // 0x90 - VK_NUMLOCK
    kKeyScrLock, // 0x91 - VK_SCROLL
    kKeyNil, // 0x92 - VK_OEM_NEC_EQUAL / VK_OEM_FJ_JISHO
    kKeyNil, // 0x93 - VK_OEM_FJ_MASSHOU
    kKeyNil, // 0x94 - VK_OEM_FJ_TOUROKU
    kKeyNil, // 0x95 - VK_OEM_FJ_LOYA
    kKeyNil, // 0x96 - VK_OEM_FJ_ROYA
    kKeyNil, // 0x97 - Unassigned
    kKeyNil, // 0x98 - Unassigned
    kKeyNil, // 0x99 - Unassigned
    kKeyNil, // 0x9A - Unassigned
    kKeyNil, // 0x9B - Unassigned
    kKeyNil, // 0x9C - Unassigned
    kKeyNil, // 0x9D - Unassigned
    kKeyNil, // 0x9E - Unassigned
    kKeyNil, // 0x9F - Unassigned
    // 0xA0
    kKeyLShift, // 0xA0 - VK_LSHIFT
    kKeyRShift, // 0xA1 - VK_RSHIFT
    kKeyLControl, // 0xA2 - VK_LCONTROL
    kKeyRControl, // 0xA3 - VK_RCONTROL
    kKeyAlt, // 0xA4 - VK_LMENU
    kKeyAltGr, // 0xA5 - VK_RMENU
    kKeyNil, // 0xA6 - VK_BROWSER_BACK
    kKeyNil, // 0xA7 - VK_BROWSER_FORWARD
    kKeyNil, // 0xA8 - VK_BROWSER_REFRESH
    kKeyNil, // 0xA9 - VK_BROWSER_STOP
    kKeyNil, // 0xAA - VK_BROWSER_SEARCH
    kKeyNil, // 0xAB - VK_BROWSER_FAVORITES
    kKeyNil, // 0xAC - VK_BROWSER_HOME
    kKeyNil, // 0xAD - VK_VOLUME_MUTE
    kKeyNil, // 0xAE - VK_VOLUME_DOWN
    kKeyNil, // 0xAF - VK_VOLUME_UP
    // 0xB0
    kKeyNil, // 0xB0 - VK_MEDIA_NEXT_TRACK
    kKeyNil, // 0xB1 - VK_MEDIA_PREV_TRACK
    kKeyNil, // 0xB2 - VK_MEDIA_STOP
    kKeyNil, // 0xB3 - VK_MEDIA_PLAY_PAUSE
    kKeyNil, // 0xB4 - VK_LAUNCH_MAIL
    kKeyNil, // 0xB5 - VK_LAUNCH_MEDIA_SELECT
    kKeyNil, // 0xB6 - VK_LAUNCH_APP1
    kKeyNil, // 0xB7 - VK_LAUNCH_APP2
    kKeyNil, // 0xB8 - Reserved
    kKeyNil, // 0xB9 - Reserved
    kKeySemicolon, // 0xBA - VK_OEM_1
    kKeyEquals, // 0xBB - VK_OEM_PLUS
    kKeyComma, // 0xBC - VK_OEM_COMMA
    kKeyMinus, // 0xBD - VK_OEM_MINUS
    kKeyStop, // 0xBE - VK_OEM_PERIOD
    kKeySlash, // 0xBF - VK_OEM_2
    // 0xC0
    kKeyTilde, // 0xC0 - VK_OEM_3
    kKeyNil, // 0xC1 - Reserved
    kKeyNil, // 0xC2 - Reserved
    kKeyNil, // 0xC3 - Reserved
    kKeyNil, // 0xC4 - Reserved
    kKeyNil, // 0xC5 - Reserved
    kKeyNil, // 0xC6 - Reserved
    kKeyNil, // 0xC7 - Reserved
    kKeyNil, // 0xC8 - Reserved
    kKeyNil, // 0xC9 - Reserved
    kKeyNil, // 0xCA - Reserved
    kKeyNil, // 0xCB - Reserved
    kKeyNil, // 0xCC - Reserved
    kKeyNil, // 0xCD - Reserved
    kKeyNil, // 0xCE - Reserved
    kKeyNil, // 0xCF - Reserved
    // 0xD0
    kKeyNil, // 0xD0 - Reserved
    kKeyNil, // 0xD1 - Reserved
    kKeyNil, // 0xD2 - Reserved
    kKeyNil, // 0xD3 - Reserved
    kKeyNil, // 0xD4 - Reserved
    kKeyNil, // 0xD5 - Reserved
    kKeyNil, // 0xD6 - Reserved
    kKeyNil, // 0xD7 - Reserved
    kKeyNil, // 0xD8 - Unassigned
    kKeyNil, // 0xD9 - Unassigned
    kKeyNil, // 0xDA - Unassigned
    kKeyOpenbrace, // 0xDB - VK_OEM_4
    kKeyBackslash, // 0xDC - VK_OEM_5
    kKeyClosebrace, // 0xDD - VK_OEM_6
    kKeyQuote, // 0xDE - VK_OEM_7
    kKeyNil, // 0xDF - VK_OEM_8
    // 0xE0
    kKeyNil, // 0xE0 - Reserved
    kKeyNil, // 0xE1 - VK_OEM_AX
    kKeyBackslash2, // 0xE2 - VK_OEM_102
    kKeyNil, // 0xE3 - VK_ICO_HELP
    kKeyNil, // 0xE4 - VK_ICO_00
    kKeyNil, // 0xE5 - VK_PROCESSKEY
    kKeyNil, // 0xE6 - VK_ICO_CLEAR
    kKeyNil, // 0xE7 - VK_PACKET
    kKeyNil, // 0xE8 - Unassigned
    kKeyNil, // 0xE9 - VK_OEM_RESET
    kKeyNil, // 0xEA - VK_OEM_JUMP
    kKeyNil, // 0xEB - VK_OEM_PA1
    kKeyNil, // 0xEC - VK_OEM_PA2
    kKeyNil, // 0xED - VK_OEM_PA3
    kKeyNil, // 0xEE - VK_OEM_WSCTRL
    kKeyNil, // 0xEF - VK_OEM_CUSEL
    // 0xF0
    kKeyNil, // 0xF0 - VK_OEM_ATTN
    kKeyNil, // 0xF1 - VK_OEM_FINISH
    kKeyNil, // 0xF2 - VK_OEM_COPY
    kKeyNil, // 0xF3 - VK_OEM_AUTO
    kKeyNil, // 0xF4 - VK_OEM_ENLW
    kKeyNil, // 0xF5 - VK_OEM_BACKTAB
    kKeyNil, // 0xF6 - VK_ATTN
    kKeyNil, // 0xF7 - VK_CRSEL
    kKeyNil, // 0xF8 - VK_EXSEL
    kKeyNil, // 0xF9 - VK_EREOF
    kKeyNil, // 0xFA - VK_PLAY
    kKeyNil, // 0xFB - VK_ZOOM
    kKeyNil, // 0xFC - VK_NONAME
    kKeyNil, // 0xFD - VK_PA1
    kKeyNil, // 0xFE - VK_OEM_CLEAR
    kKeyNil, // 0xFF - Reserved
  };
  if (vk < 0 || vk > 255)
    vk = 0;
  return keymap[vk];
}

KeyModifiers get_modifiers_from_last_win32_message()
{
  int modifiers = kKeyNoneModifier;
  if ((GetKeyState(VK_LSHIFT) & 0x8000) ||
      (GetKeyState(VK_RSHIFT) & 0x8000))
    modifiers |= kKeyShiftModifier;
  if ((GetKeyState(VK_LCONTROL) & 0x8000) ||
      (GetKeyState(VK_RCONTROL) & 0x8000))
    modifiers |= kKeyCtrlModifier;
  if ((GetKeyState(VK_LMENU) & 0x8000) ||
      (GetKeyState(VK_RMENU) & 0x8000))
    modifiers |= kKeyAltModifier;
  if (GetKeyState(VK_SPACE) & 0x8000)
    modifiers |= kKeySpaceModifier;
  if ((GetKeyState(VK_LWIN) & 0x8000) ||
      (GetKeyState(VK_RWIN) & 0x8000))
    modifiers |= kKeyWinModifier;
  return (KeyModifiers)modifiers;
}

static int scancode_to_win32vk(KeyScancode scancode)
{
  static int initialized = false;
  static int keymap[kKeyScancodes];

  if (!initialized) {
    initialized = true;
    for (int i=0; i<kKeyScancodes; ++i)
      keymap[i] = 0;
    for (int vk=0; vk<256; ++vk) {
      KeyScancode sc = win32vk_to_scancode(vk);
      if (sc != kKeyNil)
        keymap[sc] = vk;
    }
  }

  if (scancode < kKeyNil || scancode > kKeyScancodes)
    scancode = kKeyNil;

  return keymap[scancode];
}

bool win_is_key_pressed(KeyScancode scancode)
{
  int vk = scancode_to_win32vk(scancode);
  if (vk)
    return (GetAsyncKeyState(vk) & 0x8000 ? true: false);
  else
    return false;
}

int win_get_unicode_from_scancode(KeyScancode scancode)
{
  int vk = scancode_to_win32vk(scancode);
  if (vk && (GetAsyncKeyState(vk) & 0x8000 ? true: false)) {
    VkToUnicode tu;
    if (tu) {
      tu.toUnicode(vk, 0);
      if (tu.size() > 0)
        return tu[0];
    }
  }
  return 0;
}

} // namespace os
