// SHE library
// Copyright (C) 2012-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef SHE_KEYS_H_INCLUDED
#define SHE_KEYS_H_INCLUDED
#pragma once

namespace she {

  enum KeyModifiers {
    kKeyNoneModifier          = 0,
    kKeyShiftModifier         = 1,
    kKeyCtrlModifier          = 2,
    kKeyAltModifier           = 4,
    kKeyCmdModifier           = 8,
    kKeySpaceModifier         = 16,
    kKeyWinModifier           = 32,
    kKeyUninitializedModifier = 64,
  };

  enum KeyScancode {
    kKeyNil          = 0,
    kKeyA            = 1,
    kKeyB            = 2,
    kKeyC            = 3,
    kKeyD            = 4,
    kKeyE            = 5,
    kKeyF            = 6,
    kKeyG            = 7,
    kKeyH            = 8,
    kKeyI            = 9,
    kKeyJ            = 10,
    kKeyK            = 11,
    kKeyL            = 12,
    kKeyM            = 13,
    kKeyN            = 14,
    kKeyO            = 15,
    kKeyP            = 16,
    kKeyQ            = 17,
    kKeyR            = 18,
    kKeyS            = 19,
    kKeyT            = 20,
    kKeyU            = 21,
    kKeyV            = 22,
    kKeyW            = 23,
    kKeyX            = 24,
    kKeyY            = 25,
    kKeyZ            = 26,
    kKey0            = 27,
    kKey1            = 28,
    kKey2            = 29,
    kKey3            = 30,
    kKey4            = 31,
    kKey5            = 32,
    kKey6            = 33,
    kKey7            = 34,
    kKey8            = 35,
    kKey9            = 36,
    kKey0Pad         = 37,
    kKey1Pad         = 38,
    kKey2Pad         = 39,
    kKey3Pad         = 40,
    kKey4Pad         = 41,
    kKey5Pad         = 42,
    kKey6Pad         = 43,
    kKey7Pad         = 44,
    kKey8Pad         = 45,
    kKey9Pad         = 46,
    kKeyF1           = 47,
    kKeyF2           = 48,
    kKeyF3           = 49,
    kKeyF4           = 50,
    kKeyF5           = 51,
    kKeyF6           = 52,
    kKeyF7           = 53,
    kKeyF8           = 54,
    kKeyF9           = 55,
    kKeyF10          = 56,
    kKeyF11          = 57,
    kKeyF12          = 58,
    kKeyEsc          = 59,
    kKeyTilde        = 60,
    kKeyMinus        = 61,
    kKeyEquals       = 62,
    kKeyBackspace    = 63,
    kKeyTab          = 64,
    kKeyOpenbrace    = 65,
    kKeyClosebrace   = 66,
    kKeyEnter        = 67,
    kKeyColon        = 68,
    kKeyQuote        = 69,
    kKeyBackslash    = 70,
    kKeyBackslash2   = 71,
    kKeyComma        = 72,
    kKeyStop         = 73,
    kKeySlash        = 74,
    kKeySpace        = 75,
    kKeyInsert       = 76,
    kKeyDel          = 77,
    kKeyHome         = 78,
    kKeyEnd          = 79,
    kKeyPageUp       = 80,
    kKeyPageDown     = 81,
    kKeyLeft         = 82,
    kKeyRight        = 83,
    kKeyUp           = 84,
    kKeyDown         = 85,
    kKeySlashPad     = 86,
    kKeyAsterisk     = 87,
    kKeyMinusPad     = 88,
    kKeyPlusPad      = 89,
    kKeyDelPad       = 90,
    kKeyEnterPad     = 91,
    kKeyPrtscr       = 92,
    kKeyPause        = 93,
    kKeyAbntC1       = 94,
    kKeyYen          = 95,
    kKeyKana         = 96,
    kKeyConvert      = 97,
    kKeyNoconvert    = 98,
    kKeyAt           = 99,
    kKeyCircumflex   = 100,
    kKeyColon2       = 101,
    kKeyKanji        = 102,
    kKeyEqualsPad    = 103,  // MacOS X
    kKeyBackquote    = 104,  // MacOS X
    kKeySemicolon    = 105,  // MacOS X
    kKeyCommand      = 106,  // MacOS X
    kKeyUnknown1     = 107,
    kKeyUnknown2     = 108,
    kKeyUnknown3     = 109,
    kKeyUnknown4     = 110,
    kKeyUnknown5     = 111,
    kKeyUnknown6     = 112,
    kKeyUnknown7     = 113,
    kKeyUnknown8     = 114,

    kKeyFirstModifierScancode = 115,

    kKeyLShift       = 115,
    kKeyRShift       = 116,
    kKeyLControl     = 117,
    kKeyRControl     = 118,
    kKeyAlt          = 119,
    kKeyAltGr        = 120,
    kKeyLWin         = 121,
    kKeyRWin         = 122,
    kKeyMenu         = 123,
    kKeyScrLock      = 124,
    kKeyNumLock      = 125,
    kKeyCapsLock     = 126,

    kKeyScancodes    = 127
  };

  // TODO move these functions to she::System

  // Returns true if the the given scancode key is pressed/actived.
  bool is_key_pressed(KeyScancode scancode);

  // Returns the latest unicode character that activated the given
  // scancode.
  int get_unicode_from_scancode(KeyScancode scancode);

  // Clears the keyboard buffer (used only in the Allegro port).
  // TODO (deprecated)
  void clear_keyboard_buffer();

} // namespace she

#endif
