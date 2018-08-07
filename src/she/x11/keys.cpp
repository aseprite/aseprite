// SHE library
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/ints.h"
#include "she/x11/keys.h"

#include <X11/keysym.h>

namespace she {

KeyScancode x11_keysym_to_scancode(KeySym keysym)
{
  switch (keysym) {
    case XK_BackSpace: return kKeyBackspace;
    case XK_Tab: return kKeyTab;
    case XK_Linefeed: return kKeyEnter;
    case XK_Clear: return kKeyDel;
    case XK_Return: return kKeyEnter;
    case XK_Pause:
    case XK_Break: return kKeyPause;
    case XK_Scroll_Lock: return kKeyScrLock;
    case XK_Escape: return kKeyEsc;
    case XK_Delete: return kKeyDel;
    case XK_Home: return kKeyHome;
    case XK_Left: return kKeyLeft;
    case XK_Up: return kKeyUp;
    case XK_Right: return kKeyRight;
    case XK_Down: return kKeyDown;
    case XK_Page_Up: return kKeyPageUp;
    case XK_Page_Down: return kKeyPageDown;
    case XK_End: return kKeyEnd;
    case XK_Sys_Req:
    case XK_Print: return kKeyPrtscr;
    case XK_Insert: return kKeyInsert;
    case XK_Menu: return kKeyMenu;
    case XK_Num_Lock: return kKeyNumLock;
    case XK_KP_Space: return kKeySpace;
    case XK_KP_Tab: return kKeyTab;
    case XK_KP_Enter: return kKeyEnterPad;
    case XK_KP_0: case XK_KP_Insert: return kKey0Pad;
    case XK_KP_1: case XK_KP_End: return kKey1Pad;
    case XK_KP_2: case XK_KP_Down: return kKey2Pad;
    case XK_KP_3: case XK_KP_Page_Down: return kKey3Pad;
    case XK_KP_4: case XK_KP_Left: return kKey4Pad;
    case XK_KP_5: case XK_KP_Begin: return kKey5Pad;
    case XK_KP_6: case XK_KP_Right: return kKey6Pad;
    case XK_KP_7: case XK_KP_Home: return kKey7Pad;
    case XK_KP_8: case XK_KP_Up: return kKey8Pad;
    case XK_KP_9: case XK_KP_Page_Up: return kKey9Pad;
    case XK_KP_Decimal:
    case XK_KP_Delete: return kKeyDelPad;
    case XK_KP_Equal: return kKeyEqualsPad;
    case XK_KP_Multiply: return kKeyAsterisk;
    case XK_KP_Add: return kKeyPlusPad;
    case XK_KP_Subtract: return kKeyMinusPad;
    case XK_KP_Divide: return kKeySlashPad;
    case XK_F1: return kKeyF1;
    case XK_F2: return kKeyF2;
    case XK_F3: return kKeyF3;
    case XK_F4: return kKeyF4;
    case XK_F5: return kKeyF5;
    case XK_F6: return kKeyF6;
    case XK_F7: return kKeyF7;
    case XK_F8: return kKeyF8;
    case XK_F9: return kKeyF9;
    case XK_F10: return kKeyF10;
    case XK_F11: return kKeyF11;
    case XK_F12: return kKeyF12;
    case XK_Shift_L: return kKeyLShift;
    case XK_Shift_R: return kKeyRShift;
    case XK_Control_L: return kKeyLControl;
    case XK_Control_R: return kKeyRControl;
    case XK_Caps_Lock: return kKeyCapsLock;
    case XK_Alt_L: return kKeyAlt;
    case XK_Alt_R: return kKeyAltGr;
    case XK_Meta_L:
    case XK_Super_L: return kKeyLWin;
    case XK_Meta_R:
    case XK_Super_R: return kKeyRWin;
    case XK_space: return kKeySpace;
    case XK_apostrophe: return kKeyQuote;
    case XK_comma: return kKeyComma;
    case XK_minus: return kKeyMinus;
    case XK_period: return kKeyStop;
    case XK_slash: return kKeySlash;
    case XK_0: return kKey0;
    case XK_1: return kKey1;
    case XK_2: return kKey2;
    case XK_3: return kKey3;
    case XK_4: return kKey4;
    case XK_5: return kKey5;
    case XK_6: return kKey6;
    case XK_7: return kKey7;
    case XK_8: return kKey8;
    case XK_9: return kKey9;
    case XK_semicolon: return kKeyColon;
    case XK_less: return kKeyBackslash2;
    case XK_A: return kKeyA;
    case XK_B: return kKeyB;
    case XK_C: return kKeyC;
    case XK_D: return kKeyD;
    case XK_E: return kKeyE;
    case XK_F: return kKeyF;
    case XK_G: return kKeyG;
    case XK_H: return kKeyH;
    case XK_I: return kKeyI;
    case XK_J: return kKeyJ;
    case XK_K: return kKeyK;
    case XK_L: return kKeyL;
    case XK_M: return kKeyM;
    case XK_N: return kKeyN;
    case XK_O: return kKeyO;
    case XK_P: return kKeyP;
    case XK_Q: return kKeyQ;
    case XK_R: return kKeyR;
    case XK_S: return kKeyS;
    case XK_T: return kKeyT;
    case XK_U: return kKeyU;
    case XK_V: return kKeyV;
    case XK_W: return kKeyW;
    case XK_X: return kKeyX;
    case XK_Y: return kKeyY;
    case XK_Z: return kKeyZ;
    case XK_bracketleft: return kKeyOpenbrace;
    case XK_backslash: return kKeyBackslash;
    case XK_bracketright: return kKeyClosebrace;
    case XK_grave: return kKeyTilde;
    case XK_a: return kKeyA;
    case XK_b: return kKeyB;
    case XK_c: return kKeyC;
    case XK_d: return kKeyD;
    case XK_e: return kKeyE;
    case XK_f: return kKeyF;
    case XK_g: return kKeyG;
    case XK_h: return kKeyH;
    case XK_i: return kKeyI;
    case XK_j: return kKeyJ;
    case XK_k: return kKeyK;
    case XK_l: return kKeyL;
    case XK_m: return kKeyM;
    case XK_n: return kKeyN;
    case XK_o: return kKeyO;
    case XK_p: return kKeyP;
    case XK_q: return kKeyQ;
    case XK_r: return kKeyR;
    case XK_s: return kKeyS;
    case XK_t: return kKeyT;
    case XK_u: return kKeyU;
    case XK_v: return kKeyV;
    case XK_w: return kKeyW;
    case XK_x: return kKeyX;
    case XK_y: return kKeyY;
    case XK_z: return kKeyZ;
  }
  return kKeyNil;
}

bool x11_is_key_pressed(KeyScancode scancode)
{
  return false;                 // TODO
}

int x11_get_unicode_from_scancode(KeyScancode scancode)
{
  return 0;                     // TODO
}

} // namespace she
