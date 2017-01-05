// SHE library
// Copyright (C) 2015-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "she/osx/vk.h"

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h> // TIS functions

namespace she {

KeyScancode cocoavk_to_scancode(UInt16 keyCode)
{
  static KeyScancode keymap[256] = {
    // 0x00
    kKeyA, // 0x00 - kVK_ANSI_A
    kKeyS, // 0x01 - kVK_ANSI_S
    kKeyD, // 0x02 - kVK_ANSI_D
    kKeyF, // 0x03 - kVK_ANSI_F
    kKeyH, // 0x04 - kVK_ANSI_H
    kKeyG, // 0x05 - kVK_ANSI_G
    kKeyZ, // 0x06 - kVK_ANSI_Z
    kKeyX, // 0x07 - kVK_ANSI_X
    kKeyC, // 0x08 - kVK_ANSI_C
    kKeyV, // 0x09 - kVK_ANSI_V
    kKeyNil, // 0x0A - kVK_ISO_Section
    kKeyB, // 0x0B - kVK_ANSI_B
    kKeyQ, // 0x0C - kVK_ANSI_Q
    kKeyW, // 0x0D - kVK_ANSI_W
    kKeyE, // 0x0E - kVK_ANSI_E
    kKeyR, // 0x0F - kVK_ANSI_R
    // 0x10
    kKeyY, // 0x10 - kVK_ANSI_Y
    kKeyT, // 0x11 - kVK_ANSI_T
    kKey1, // 0x12 - kVK_ANSI_1
    kKey2, // 0x13 - kVK_ANSI_2
    kKey3, // 0x14 - kVK_ANSI_3
    kKey4, // 0x15 - kVK_ANSI_4
    kKey6, // 0x16 - kVK_ANSI_6
    kKey5, // 0x17 - kVK_ANSI_5
    kKeyEquals, // 0x18 - kVK_ANSI_Equal
    kKey9, // 0x19 - kVK_ANSI_9
    kKey7, // 0x1A - kVK_ANSI_7
    kKeyMinus, // 0x1B - kVK_ANSI_Minus
    kKey8, // 0x1C - kVK_ANSI_8
    kKey0, // 0x1D - kVK_ANSI_0
    kKeyClosebrace, // 0x1E - kVK_ANSI_RightBracket
    kKeyO, // 0x1F - kVK_ANSI_O
    // 0x20
    kKeyU, // 0x20 - kVK_ANSI_U
    kKeyOpenbrace, // 0x21 - kVK_ANSI_LeftBracket
    kKeyI, // 0x22 - kVK_ANSI_I
    kKeyP, // 0x23 - kVK_ANSI_P
    kKeyEnter, // 0x24 - kVK_Return
    kKeyL, // 0x25 - kVK_ANSI_L
    kKeyJ, // 0x26 - kVK_ANSI_J
    kKeyQuote, // 0x27 - kVK_ANSI_Quote
    kKeyK, // 0x28 - kVK_ANSI_K
    kKeySemicolon, // 0x29 - kVK_ANSI_Semicolon
    kKeyBackslash, // 0x2A - kVK_ANSI_Backslash
    kKeyComma, // 0x2B - kVK_ANSI_Comma
    kKeySlash, // 0x2C - kVK_ANSI_Slash
    kKeyN, // 0x2D - kVK_ANSI_N
    kKeyM, // 0x2E - kVK_ANSI_M
    kKeyStop, // 0x2F - kVK_ANSI_Period
    // 0x30
    kKeyTab, // 0x30 - kVK_Tab
    kKeySpace, // 0x31 - kVK_Space
    kKeyNil, // 0x32 - kVK_ANSI_Grave
    kKeyBackspace, // 0x33 - kVK_Delete
    kKeyNil, // 0x34 - ?
    kKeyEsc, // 0x35 - kVK_Escape
    kKeyNil, // 0x36 - ?
    kKeyCommand, // 0x37 - kVK_Command
    kKeyLShift, // 0x38 - kVK_Shift
    kKeyCapsLock, // 0x39 - kVK_CapsLock
    kKeyAlt, // 0x3A - kVK_Option
    kKeyLControl, // 0x3B - kVK_Control
    kKeyRShift, // 0x3C - kVK_RightShift
    kKeyAltGr, // 0x3D - kVK_RightOption
    kKeyRControl, // 0x3E - kVK_RightControl
    kKeyNil, // 0x3F - kVK_Function
    // 0x40
    kKeyNil, // 0x40 - kVK_F17
    kKeyNil, // 0x41 - kVK_ANSI_KeypadDecimal
    kKeyNil, // 0x42 - ?
    kKeyAsterisk, // 0x43 - kVK_ANSI_KeypadMultiply
    kKeyNil, // 0x44 - ?
    kKeyPlusPad, // 0x45 - kVK_ANSI_KeypadPlus
    kKeyNil, // 0x46 - ?
    kKeyDelPad, // 0x47 - kVK_ANSI_KeypadClear
    kKeyNil, // 0x48 - kVK_VolumeUp
    kKeyNil, // 0x49 - kVK_VolumeDown
    kKeyNil, // 0x4A - kVK_Mute
    kKeySlashPad, // 0x4B - kVK_ANSI_KeypadDivide
    kKeyEnterPad, // 0x4C - kVK_ANSI_KeypadEnter
    kKeyNil, // 0x4D - ?
    kKeyMinusPad, // 0x4E - kVK_ANSI_KeypadMinus
    kKeyNil, // 0x4F - kVK_F18
    // 0x50
    kKeyNil, // 0x50 - kVK_F19
    kKeyEqualsPad, // 0x51 - kVK_ANSI_KeypadEquals
    kKey0Pad, // 0x52 - kVK_ANSI_Keypad0
    kKey1Pad, // 0x53 - kVK_ANSI_Keypad1
    kKey2Pad, // 0x54 - kVK_ANSI_Keypad2
    kKey3Pad, // 0x55 - kVK_ANSI_Keypad3
    kKey4Pad, // 0x56 - kVK_ANSI_Keypad4
    kKey5Pad, // 0x57 - kVK_ANSI_Keypad5
    kKey6Pad, // 0x58 - kVK_ANSI_Keypad6
    kKey7Pad, // 0x59 - kVK_ANSI_Keypad7
    kKeyNil, // 0x5A - kVK_F20
    kKey8Pad, // 0x5B - kVK_ANSI_Keypad8
    kKey9Pad, // 0x5C - kVK_ANSI_Keypad9
    kKeyYen, // 0x5D - kVK_JIS_Yen
    kKeyNil, // 0x5E - kVK_JIS_Underscore
    kKeyNil, // 0x5F - kVK_JIS_KeypadComma
    // 0x60
    kKeyF5, // 0x60 - kVK_F5
    kKeyF6, // 0x61 - kVK_F6
    kKeyF7, // 0x62 - kVK_F7
    kKeyF3, // 0x63 - kVK_F3
    kKeyF8, // 0x64 - kVK_F8
    kKeyF9, // 0x65 - kVK_F9
    kKeyNil, // 0x66 - kVK_JIS_Eisu
    kKeyF11, // 0x67 - kVK_F11
    kKeyKana, // 0x68 - kVK_JIS_Kana
    kKeyNil, // 0x69 - kVK_F13
    kKeyNil, // 0x6A - kVK_F16
    kKeyNil, // 0x6B - kVK_F14
    kKeyNil, // 0x6C - ?
    kKeyF10, // 0x6D - kVK_F10
    kKeyNil, // 0x6E - ?
    kKeyF12, // 0x6F - kVK_F12
    // 0x70
    kKeyNil, // 0x70 - ?
    kKeyNil, // 0x71 - kVK_F15
    kKeyNil, // 0x72 - kVK_Help
    kKeyHome, // 0x73 - kVK_Home
    kKeyPageUp, // 0x74 - kVK_PageUp
    kKeyDel, // 0x75 - kVK_ForwardDelete
    kKeyF4, // 0x76 - kVK_F4
    kKeyEnd, // 0x77 - kVK_End
    kKeyF2, // 0x78 - kVK_F2
    kKeyPageDown, // 0x79 - kVK_PageDown
    kKeyF1, // 0x7A - kVK_F1
    kKeyLeft, // 0x7B - kVK_LeftArrow
    kKeyRight, // 0x7C - kVK_RightArrow
    kKeyDown, // 0x7D - kVK_DownArrow
    kKeyUp, // 0x7E - kVK_UpArrow
    kKeyNil // 0x7F - ?
  };
  if (keyCode < 0 || keyCode > 127)
    keyCode = 0;
  return keymap[keyCode];
}

// Based on code from:
// http://stackoverflow.com/questions/22566665/how-to-capture-unicode-from-key-events-without-an-nstextview
// http://stackoverflow.com/questions/12547007/convert-key-code-into-key-equivalent-string
// http://stackoverflow.com/questions/8263618/convert-virtual-key-code-to-unicode-string
//
// If "deadKeyState" is = nullptr, it doesn't process dead keys.
CFStringRef get_unicode_from_key_code(const UInt16 keyCode,
                                      const NSEventModifierFlags modifierFlags,
                                      UInt32* deadKeyState)
{
  // The "TISCopyCurrentKeyboardInputSource()" doesn't contain
  // kTISPropertyUnicodeKeyLayoutData (returns nullptr) when the input
  // source is Japanese (Romaji/Hiragana/Katakana).

  //TISInputSourceRef inputSource = TISCopyCurrentKeyboardInputSource();
  TISInputSourceRef inputSource = TISCopyCurrentKeyboardLayoutInputSource();
  CFDataRef keyLayoutData = (CFDataRef)TISGetInputSourceProperty(inputSource, kTISPropertyUnicodeKeyLayoutData);
  const UCKeyboardLayout* keyLayout =
    (keyLayoutData ? (const UCKeyboardLayout*)CFDataGetBytePtr(keyLayoutData): nullptr);

  UInt32 deadKeyStateWrap = (deadKeyState ? *deadKeyState: 0);
  UniChar output[4];
  UniCharCount length;

  // Reference here:
  // https://developer.apple.com/reference/coreservices/1390584-uckeytranslate?language=objc
  UCKeyTranslate(
    keyLayout,
    keyCode,
    kUCKeyActionDown,
    ((modifierFlags >> 16) & 0xFF),
    LMGetKbdType(),
    (deadKeyState ? 0: kUCKeyTranslateNoDeadKeysMask),
    &deadKeyStateWrap,
    sizeof(output) / sizeof(output[0]),
    &length,
    output);

  if (deadKeyState)
    *deadKeyState = deadKeyStateWrap;

  CFRelease(inputSource);
  return CFStringCreateWithCharacters(kCFAllocatorDefault, output, length);
}

} // namespace she
