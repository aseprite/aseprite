/*
  Mouse driver for ASE
  by David Capello

  Based on code of Stefan Schimanski, Peter Wang, Elias Pschernig,
  and Milan Mimica.
*/

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wkey INFO: "
#define PREFIX_W                "al-wkey WARNING: "
#define PREFIX_E                "al-wkey ERROR: "



#define KEY_UNKNOWN KEY_MAX


/* lookup table for converting virtualkey VK_* codes into Allegro KEY_* codes */
/* Last unknown key sequence: 39*/
static const unsigned char hw_to_mycode[256] =
{
   /* 0x00 */    0,                       KEY_UNKNOWN+0,             KEY_UNKNOWN+1,              KEY_UNKNOWN+2,
   /* 0x04 */    KEY_UNKNOWN+3,           KEY_UNKNOWN+4,             KEY_UNKNOWN+5,              0,
   /* 0x08 */    KEY_BACKSPACE,           KEY_TAB,                   0,                          0,
   /* 0x0C */    KEY_UNKNOWN+39,          KEY_ENTER,                 0,                          0,
   /* 0x10 */    0/*L or R shift*/,       0/*L or R ctrl*/,          0/*L or R alt*/,            KEY_PAUSE,
   /* 0x14 */    KEY_CAPSLOCK,            KEY_KANA,                  0,                          KEY_UNKNOWN+6,
   /* 0x18 */    KEY_UNKNOWN+7,           KEY_KANJI,                 0,                          KEY_ESC,
   /* 0x1C */    KEY_CONVERT,             KEY_NOCONVERT,             KEY_UNKNOWN+8,              KEY_UNKNOWN+9,
   /* 0x20 */    KEY_SPACE,               KEY_PGUP,                  KEY_PGDN,                   KEY_END,
   /* 0x24 */    KEY_HOME,                KEY_LEFT,                  KEY_UP,                     KEY_RIGHT,
   /* 0x28 */    KEY_DOWN,                KEY_UNKNOWN+10,            KEY_UNKNOWN+11,             KEY_UNKNOWN+12,
   /* 0x2C */    KEY_PRTSCR,              KEY_INSERT,                KEY_DEL,                    KEY_UNKNOWN+13,
   /* 0x30 */    KEY_0,                   KEY_1,                     KEY_2,                      KEY_3,
   /* 0x34 */    KEY_4,                   KEY_5,                     KEY_6,                      KEY_7,
   /* 0x38 */    KEY_8,                   KEY_9,                     0,                          0,
   /* 0x3C */    0,                       0,                         0,                          0,
   /* 0x40 */    0,                       KEY_A,                     KEY_B,                      KEY_C,
   /* 0x44 */    KEY_D,                   KEY_E,                     KEY_F,                      KEY_G,    
   /* 0x48 */    KEY_H,                   KEY_I,                     KEY_J,                      KEY_K,
   /* 0x4C */    KEY_L,                   KEY_M,                     KEY_N,                      KEY_O,
   /* 0x50 */    KEY_P,                   KEY_Q,                     KEY_R,                      KEY_S,
   /* 0x54 */    KEY_T,                   KEY_U,                     KEY_V,                      KEY_W,
   /* 0x58 */    KEY_X,                   KEY_Y,                     KEY_Z,                      KEY_LWIN,
   /* 0x5C */    KEY_RWIN,                KEY_UNKNOWN+14,            0,                          0,
   /* 0x60 */    KEY_0_PAD,               KEY_1_PAD,                 KEY_2_PAD,                  KEY_3_PAD,
   /* 0x64 */    KEY_4_PAD,               KEY_5_PAD,                 KEY_6_PAD,                  KEY_7_PAD,
   /* 0x68 */    KEY_8_PAD,               KEY_9_PAD,                 KEY_ASTERISK,               KEY_PLUS_PAD,
   /* 0x6C */    KEY_UNKNOWN+15,          KEY_MINUS_PAD,             KEY_UNKNOWN+16,             KEY_SLASH_PAD,
   /* 0x70 */    KEY_F1,                  KEY_F2,                    KEY_F3,                     KEY_F4,
   /* 0x74 */    KEY_F5,                  KEY_F6,                    KEY_F7,                     KEY_F8,
   /* 0x78 */    KEY_F9,                  KEY_F10,                   KEY_F11,                    KEY_F12,
   /* 0x7C */    KEY_UNKNOWN+17,          KEY_UNKNOWN+18,            KEY_UNKNOWN+19,             KEY_UNKNOWN+20,
   /* 0x80 */    KEY_UNKNOWN+21,          KEY_UNKNOWN+22,            KEY_UNKNOWN+23,             KEY_UNKNOWN+24,
   /* 0x84 */    KEY_UNKNOWN+25,          KEY_UNKNOWN+26,            KEY_UNKNOWN+27,             KEY_UNKNOWN+28,
   /* 0x88 */    0,                       0,                         0,                          0,
   /* 0x8C */    0,                       0,                         0,                          0,
   /* 0x90 */    KEY_NUMLOCK,             KEY_SCRLOCK,               0,                          0,
   /* 0x94 */    0,                       0,                         0,                          0,
   /* 0x98 */    0,                       0,                         0,                          0,
   /* 0x9C */    0,                       0,                         0,                          0,
   /* 0xA0 */    KEY_LSHIFT,              KEY_RSHIFT,                KEY_LCONTROL,               KEY_RCONTROL,
   /* 0xA4 */    KEY_ALT,                 KEY_ALTGR,                 0,                          0,
   /* 0xA8 */    0,                       0,                         0,                          0,
   /* 0xAC */    0,                       0,                         0,                          0,
   /* 0xB0 */    0,                       0,                         0,                          0,
   /* 0xB4 */    0,                       0,                         0,                          0,
   /* 0xB8 */    0,                       0,                         KEY_SEMICOLON,              KEY_EQUALS,
   /* 0xBC */    KEY_COMMA,               KEY_MINUS,                 KEY_STOP,                   KEY_SLASH,
   /* 0xC0 */    KEY_TILDE,               0,                         0,                          0,
   /* 0xC4 */    0,                       0,                         0,                          0,
   /* 0xC8 */    0,                       0,                         0,                          0,
   /* 0xCC */    0,                       0,                         0,                          0,
   /* 0xD0 */    0,                       0,                         0,                          0,
   /* 0xD4 */    0,                       0,                         0,                          0,
   /* 0xD8 */    0,                       0,                         0,                          KEY_OPENBRACE,
   /* 0xDC */    KEY_BACKSLASH,           KEY_CLOSEBRACE,            KEY_QUOTE,                  0,
   /* 0xE0 */    0,                       0,                         KEY_BACKSLASH2,             0,
   /* 0xE4 */    0,                       KEY_UNKNOWN+29,            0,                          0,
   /* 0xE8 */    0,                       0,                         0,                          0,
   /* 0xEC */    0,                       0,                         0,                          0,
   /* 0xF0 */    0,                       0,                         0,                          0,
   /* 0xF4 */    0,                       0,                         KEY_UNKNOWN+30,             KEY_UNKNOWN+31,
   /* 0xF8 */    KEY_UNKNOWN+32,          KEY_UNKNOWN+33,            KEY_UNKNOWN+34,             KEY_UNKNOWN+35,
   /* 0xFC */    KEY_UNKNOWN+36,          KEY_UNKNOWN+37,            KEY_UNKNOWN+38,             0
   };




/* Update the key_shifts.
 */
static void update_shifts(BYTE* keystate)
{
   /* TODO: There must be a more efficient way to maintain key_modifiers? */
   /* Can't we just deprecate key_shifts, now that pckeys.c is gone? EP */
   unsigned int modifiers = 0;

   if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
       modifiers |= KB_SHIFT_FLAG;
   if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
       modifiers |= KB_CTRL_FLAG;
   if (GetAsyncKeyState(VK_MENU) & 0x8000)
       modifiers |= KB_ALT_FLAG;

   if (keystate[VK_SCROLL] & 1)
      modifiers |= KB_SCROLOCK_FLAG;
   if (keystate[VK_NUMLOCK] & 1)
      modifiers |= KB_NUMLOCK_FLAG;
   if (keystate[VK_CAPITAL] & 1)
      modifiers |= KB_CAPSLOCK_FLAG;

   _key_shifts = modifiers;
}



/* _al_win_kbd_handle_key_press:
 *  Does stuff when a key is pressed.
 */
void _al_win_kbd_handle_key_press(int scode, int vcode, BOOL repeated)
{
   int my_code;
   int ccode;
   BYTE ks[256];
   WCHAR buf[8];

   if (!GetKeyboardState(&ks[0]))
      ccode = 0; /* shound't really happen */
   else if (ToUnicode(vcode, scode, ks, buf, 8, 0) == 1)
      ccode = buf[0];
   else
      ccode = 0;

   /* Windows doesn't differentiate between the left and right versions
      of the modifiers. To compensate we read the current keyboard state
      and use that information to determine which key was actually
      pressed. We check the last known state of the modifier in question
      and if it is not down we know that is the key that was pressed. */
   if (vcode == VK_SHIFT || vcode == VK_CONTROL || vcode == VK_MENU) {
      if ((ks[VK_LCONTROL] & 0x80) && !key[KEY_LCONTROL])
         vcode = VK_LCONTROL;
      else if ((ks[VK_RCONTROL] & 0x80) && !key[KEY_RCONTROL])
         vcode = VK_RCONTROL;
      else if ((ks[VK_LSHIFT] & 0x80) && !key[KEY_LSHIFT])
         vcode = VK_LSHIFT;
      else if ((ks[VK_RSHIFT] & 0x80) && !key[KEY_RSHIFT])
         vcode = VK_RSHIFT;
      else if ((ks[VK_LMENU] & 0x80) && !key[KEY_ALT])
         vcode = VK_LMENU;
      else if ((ks[VK_RMENU] & 0x80) && !key[KEY_ALTGR])
         vcode = VK_RMENU;
      else
         return;
   }

   /* Ignore repeats for Caps Lock */
   if (vcode == VK_CAPITAL && repeated && key[KEY_CAPSLOCK])
      return;

   my_code = hw_to_mycode[vcode];
   update_shifts(ks);

   _handle_key_press(ccode, my_code);
}




/* _al_win_kbd_handle_key_release:
 *  Does stuff when a key is released.
 */
void _al_win_kbd_handle_key_release(int vcode)
{
   int my_code;
   BYTE ks[256];

   /* We need to read the latest key states so we can tell which
      modifier was released. */
   GetKeyboardState(&ks[0]);

   if (vcode == VK_SHIFT && key[KEY_LSHIFT] && !(ks[VK_LSHIFT] & 0x80))
      vcode = VK_LSHIFT;
   else if (vcode == VK_SHIFT && key[KEY_RSHIFT] && !(ks[VK_RSHIFT] & 0x80))
      vcode = VK_RSHIFT;
   else if (vcode == VK_CONTROL && key[KEY_LCONTROL] && !(ks[VK_LCONTROL] & 0x80))
      vcode = VK_LCONTROL;
   else if (vcode == VK_CONTROL && key[KEY_RCONTROL] && !(ks[VK_RCONTROL] & 0x80))
      vcode = VK_RCONTROL;
   else if (vcode == VK_MENU && key[KEY_ALT] && !(ks[VK_LMENU] & 0x80))
      vcode = VK_LMENU;
   else if (vcode == VK_MENU && key[KEY_ALTGR] && !(ks[VK_RMENU] & 0x80))
      vcode = VK_RMENU;
   else if(vcode == VK_MENU)
      return;

   my_code = hw_to_mycode[vcode];
   update_shifts(ks);

   _handle_key_release(my_code);

   /* Windows only sends a WM_KEYUP message for the Shift keys when
      both have been released. If one of the Shift keys is still reported
      as down, we need to release it as well. */
   if (my_code == KEY_LSHIFT && key[KEY_RSHIFT])
      _al_win_kbd_handle_key_release(VK_RSHIFT);
   else if (my_code == KEY_RSHIFT && key[KEY_LSHIFT])
      _al_win_kbd_handle_key_release(VK_LSHIFT);
}



static int key_winapi_init(void)
{
   BYTE ks[256];
   GetKeyboardState(&ks[0]);
   update_shifts(ks);

   return 0;
}



static void key_winapi_exit(void)
{
  /* Do nothing. */
}



#define KEYBOARD_WINAPI AL_ID('W','A','P','I')

static KEYBOARD_DRIVER keyboard_winapi =
{
   KEYBOARD_WINAPI,
   0,
   0,
   "WinAPI keyboard",
   1,
   key_winapi_init,
   key_winapi_exit,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL
};



_DRIVER_INFO _keyboard_driver_list[] =
{
   {KEYBOARD_WINAPI, &keyboard_winapi, TRUE},
   {0, NULL, 0}
};
