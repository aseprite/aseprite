// Aseprite UI Library
// Copyright (C) 2001-2016  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_KEYS_H_INCLUDED
#define UI_KEYS_H_INCLUDED
#pragma once

#include "os/keys.h"

namespace ui {

typedef os::KeyModifiers KeyModifiers;
typedef os::KeyScancode KeyScancode;

using os::kKeyAltModifier;
using os::kKeyCmdModifier;
using os::kKeyCtrlModifier;
using os::kKeyNoneModifier;
using os::kKeyShiftModifier;
using os::kKeySpaceModifier;
using os::kKeyUninitializedModifier;
using os::kKeyWinModifier;

using os::kKey0;
using os::kKey0Pad;
using os::kKey1;
using os::kKey1Pad;
using os::kKey2;
using os::kKey2Pad;
using os::kKey3;
using os::kKey3Pad;
using os::kKey4;
using os::kKey4Pad;
using os::kKey5;
using os::kKey5Pad;
using os::kKey6;
using os::kKey6Pad;
using os::kKey7;
using os::kKey7Pad;
using os::kKey8;
using os::kKey8Pad;
using os::kKey9;
using os::kKey9Pad;
using os::kKeyA;
using os::kKeyAbntC1;
using os::kKeyAsterisk;
using os::kKeyAt;
using os::kKeyB;
using os::kKeyBackquote;
using os::kKeyBackslash;
using os::kKeyBackslash2;
using os::kKeyBackspace;
using os::kKeyC;
using os::kKeyCircumflex;
using os::kKeyClosebrace;
using os::kKeyColon;
using os::kKeyColon2;
using os::kKeyComma;
using os::kKeyConvert;
using os::kKeyD;
using os::kKeyDel;
using os::kKeyDelPad;
using os::kKeyDown;
using os::kKeyE;
using os::kKeyEnd;
using os::kKeyEnter;
using os::kKeyEnterPad;
using os::kKeyEquals;
using os::kKeyEqualsPad;
using os::kKeyEsc;
using os::kKeyF;
using os::kKeyF1;
using os::kKeyF10;
using os::kKeyF11;
using os::kKeyF12;
using os::kKeyF2;
using os::kKeyF3;
using os::kKeyF4;
using os::kKeyF5;
using os::kKeyF6;
using os::kKeyF7;
using os::kKeyF8;
using os::kKeyF9;
using os::kKeyG;
using os::kKeyH;
using os::kKeyHome;
using os::kKeyI;
using os::kKeyInsert;
using os::kKeyJ;
using os::kKeyK;
using os::kKeyKana;
using os::kKeyKanji;
using os::kKeyL;
using os::kKeyLeft;
using os::kKeyM;
using os::kKeyMinus;
using os::kKeyMinusPad;
using os::kKeyN;
using os::kKeyNil;
using os::kKeyNoconvert;
using os::kKeyO;
using os::kKeyOpenbrace;
using os::kKeyP;
using os::kKeyPageDown;
using os::kKeyPageUp;
using os::kKeyPause;
using os::kKeyPlusPad;
using os::kKeyPrtscr;
using os::kKeyQ;
using os::kKeyQuote;
using os::kKeyR;
using os::kKeyRight;
using os::kKeyS;
using os::kKeySemicolon;
using os::kKeySlash;
using os::kKeySlashPad;
using os::kKeySpace;
using os::kKeyStop;
using os::kKeyT;
using os::kKeyTab;
using os::kKeyTilde;
using os::kKeyU;
using os::kKeyUnknown1;
using os::kKeyUnknown2;
using os::kKeyUnknown3;
using os::kKeyUnknown4;
using os::kKeyUnknown5;
using os::kKeyUnknown6;
using os::kKeyUnknown7;
using os::kKeyUnknown8;
using os::kKeyUp;
using os::kKeyV;
using os::kKeyW;
using os::kKeyX;
using os::kKeyY;
using os::kKeyYen;
using os::kKeyZ;

using os::kKeyFirstModifierScancode;

using os::kKeyAlt;
using os::kKeyAltGr;
using os::kKeyCapsLock;
using os::kKeyCommand;
using os::kKeyLControl;
using os::kKeyLShift;
using os::kKeyLWin;
using os::kKeyMenu;
using os::kKeyNumLock;
using os::kKeyRControl;
using os::kKeyRShift;
using os::kKeyRWin;
using os::kKeyScrLock;

using os::kKeyScancodes;

} // namespace ui

#endif
