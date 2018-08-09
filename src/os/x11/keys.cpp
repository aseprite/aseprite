// LAF OS Library
// Copyright (C) 2016-2018  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "base/ints.h"
#include "os/x11/keys.h"
#include "os/x11/x11.h"

#include <X11/Xlib.h>
#include <X11/keysym.h>

namespace os {

KeyScancode x11_keysym_to_scancode(const KeySym keysym)
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

KeySym x11_keysym_to_scancode(const KeyScancode scancode)
{
  switch (scancode) {
    case kKeyBackspace: return XK_BackSpace;
    case kKeyTab: return XK_Tab;
    case kKeyEnter: return XK_Return;
    case kKeyPause: return XK_Pause;
    case kKeyScrLock: return XK_Scroll_Lock;
    case kKeyEsc: return XK_Escape;
    case kKeyDel: return XK_Delete;
    case kKeyHome: return XK_Home;
    case kKeyLeft: return XK_Left;
    case kKeyUp: return XK_Up;
    case kKeyRight: return XK_Right;
    case kKeyDown: return XK_Down;
    case kKeyPageUp: return XK_Page_Up;
    case kKeyPageDown: return XK_Page_Down;
    case kKeyEnd: return XK_End;
    case kKeyPrtscr: return XK_Print;
    case kKeyInsert: return XK_Insert;
    case kKeyMenu: return XK_Menu;
    case kKeyNumLock: return XK_Num_Lock;
    case kKeyEnterPad: return XK_KP_Enter;
    case kKey0Pad: return XK_KP_0;
    case kKey1Pad: return XK_KP_1;
    case kKey2Pad: return XK_KP_2;
    case kKey3Pad: return XK_KP_3;
    case kKey4Pad: return XK_KP_4;
    case kKey5Pad: return XK_KP_5;
    case kKey6Pad: return XK_KP_6;
    case kKey7Pad: return XK_KP_7;
    case kKey8Pad: return XK_KP_8;
    case kKey9Pad: return XK_KP_9;
    case kKeyDelPad: return XK_KP_Delete;
    case kKeyEqualsPad: return XK_KP_Equal;
    case kKeyAsterisk: return XK_KP_Multiply;
    case kKeyPlusPad: return XK_KP_Add;
    case kKeyMinusPad: return XK_KP_Subtract;
    case kKeySlashPad: return XK_KP_Divide;
    case kKeyF1: return XK_F1;
    case kKeyF2: return XK_F2;
    case kKeyF3: return XK_F3;
    case kKeyF4: return XK_F4;
    case kKeyF5: return XK_F5;
    case kKeyF6: return XK_F6;
    case kKeyF7: return XK_F7;
    case kKeyF8: return XK_F8;
    case kKeyF9: return XK_F9;
    case kKeyF10: return XK_F10;
    case kKeyF11: return XK_F11;
    case kKeyF12: return XK_F12;
    case kKeyLShift: return XK_Shift_L;
    case kKeyRShift: return XK_Shift_R;
    case kKeyLControl: return XK_Control_L;
    case kKeyRControl: return XK_Control_R;
    case kKeyCapsLock: return XK_Caps_Lock;
    case kKeyAlt: return XK_Alt_L;
    case kKeyAltGr: return XK_Alt_R;
    case kKeyLWin: return XK_Super_L;
    case kKeyRWin: return XK_Super_R;
    case kKeySpace: return XK_space;
    case kKeyQuote: return XK_apostrophe;
    case kKeyComma: return XK_comma;
    case kKeyMinus: return XK_minus;
    case kKeyStop: return XK_period;
    case kKeySlash: return XK_slash;
    case kKey0: return XK_0;
    case kKey1: return XK_1;
    case kKey2: return XK_2;
    case kKey3: return XK_3;
    case kKey4: return XK_4;
    case kKey5: return XK_5;
    case kKey6: return XK_6;
    case kKey7: return XK_7;
    case kKey8: return XK_8;
    case kKey9: return XK_9;
    case kKeyColon: return XK_semicolon;
    case kKeyBackslash2: return XK_less;
    case kKeyOpenbrace: return XK_bracketleft;
    case kKeyBackslash: return XK_backslash;
    case kKeyClosebrace: return XK_bracketright;
    case kKeyTilde: return XK_grave;
    case kKeyA: return XK_a;
    case kKeyB: return XK_b;
    case kKeyC: return XK_c;
    case kKeyD: return XK_d;
    case kKeyE: return XK_e;
    case kKeyF: return XK_f;
    case kKeyG: return XK_g;
    case kKeyH: return XK_h;
    case kKeyI: return XK_i;
    case kKeyJ: return XK_j;
    case kKeyK: return XK_k;
    case kKeyL: return XK_l;
    case kKeyM: return XK_m;
    case kKeyN: return XK_n;
    case kKeyO: return XK_o;
    case kKeyP: return XK_p;
    case kKeyQ: return XK_q;
    case kKeyR: return XK_r;
    case kKeyS: return XK_s;
    case kKeyT: return XK_t;
    case kKeyU: return XK_u;
    case kKeyV: return XK_v;
    case kKeyW: return XK_w;
    case kKeyX: return XK_x;
    case kKeyY: return XK_y;
    case kKeyZ: return XK_z;
  }
  return 0;
}

bool x11_is_key_pressed(const KeyScancode scancode)
{
  ::Display* display = X11::instance()->display();
  const KeySym keysym = x11_keysym_to_scancode(scancode);
  const KeyCode keycode = XKeysymToKeycode(display, keysym);
  if (!keycode)
    return false;

  // TODO several platforms have this kind of API to get the whole
  //      keyboard state, it would be a lot better if we expose this
  //      API to the user (so we don't have to call XQueryKeymap for
  //      each key).
  char keys[32];
  XQueryKeymap(display, keys);

  return (keys[keycode/8] & (1 << (keycode%8)) ? true: false);
}

// TODO I guess that this code should be common to all platforms, but
//      osx/win_get_unicode_from_scancode() work in a different way:
//      the Unicode is returned only if the scancode key is pressed
//      (and that's the case anyway in the only part we are using
//      System::getUnicodeFromScancode(), a System::isKeyPressed() is
//      tested before).
int x11_get_unicode_from_scancode(const KeyScancode scancode)
{
  switch (scancode) {
    case kKeyEqualsPad: return '=';
    case kKeyAsterisk: return '*';
    case kKeyPlusPad: return '+';
    case kKeyMinusPad: return '-';
    case kKeySlashPad: return '/';
    case kKeySpace: return ' ';
    case kKeyQuote: return '\'';
    case kKeyComma: return ',';
    case kKeyMinus: return '-';
    case kKeyStop: return '.';
    case kKeySlash: return '/';
    case kKey0: case kKey0Pad: return '0';
    case kKey1: case kKey1Pad: return '1';
    case kKey2: case kKey2Pad: return '2';
    case kKey3: case kKey3Pad: return '3';
    case kKey4: case kKey4Pad: return '4';
    case kKey5: case kKey5Pad: return '5';
    case kKey6: case kKey6Pad: return '6';
    case kKey7: case kKey7Pad: return '7';
    case kKey8: case kKey8Pad: return '8';
    case kKey9: case kKey9Pad: return '9';
    case kKeyColon: return ':';
    case kKeyBackslash: case kKeyBackslash2: return '\\';
    case kKeyOpenbrace: return '[';
    case kKeyClosebrace: return ']';
    case kKeyTilde: return '~';
    case kKeyA: return 'a';
    case kKeyB: return 'b';
    case kKeyC: return 'c';
    case kKeyD: return 'd';
    case kKeyE: return 'e';
    case kKeyF: return 'f';
    case kKeyG: return 'g';
    case kKeyH: return 'h';
    case kKeyI: return 'i';
    case kKeyJ: return 'j';
    case kKeyK: return 'k';
    case kKeyL: return 'l';
    case kKeyM: return 'm';
    case kKeyN: return 'n';
    case kKeyO: return 'o';
    case kKeyP: return 'p';
    case kKeyQ: return 'q';
    case kKeyR: return 'r';
    case kKeyS: return 's';
    case kKeyT: return 't';
    case kKeyU: return 'u';
    case kKeyV: return 'v';
    case kKeyW: return 'w';
    case kKeyX: return 'x';
    case kKeyY: return 'y';
    case kKeyZ: return 'z';
  }
  return 0;
}

} // namespace os
