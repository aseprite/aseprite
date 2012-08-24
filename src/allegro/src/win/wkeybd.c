/*
  Keyboard driver for ASE
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
void _al_win_kbd_update_shifts(void)
{
   if (!keyboard_driver)
      return;

#define HANDLE_KEY(mycode, vk)                  \
   if (GetAsyncKeyState(vk) & 0x8000) {         \
      if (!key[mycode])                         \
         _handle_key_press(0, mycode);          \
   }                                            \
   else {                                       \
      if (key[mycode])                          \
         _handle_key_release(mycode);           \
   }

   HANDLE_KEY(KEY_ALT, VK_MENU);
   HANDLE_KEY(KEY_LSHIFT, VK_LSHIFT);
   HANDLE_KEY(KEY_RSHIFT, VK_RSHIFT);
   HANDLE_KEY(KEY_LCONTROL, VK_LCONTROL);
   HANDLE_KEY(KEY_RCONTROL, VK_RCONTROL);
}



/* _al_win_kbd_handle_key_press:
 *  Does stuff when a key is pressed.
 */
void _al_win_kbd_handle_key_press(int scode, int vcode, BOOL repeated)
{
   int mycode;
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
     _al_win_kbd_update_shifts();
     return;
   }

   /* Ignore repeats for Caps Lock */
   if (vcode == VK_CAPITAL && repeated && key[KEY_CAPSLOCK])
      return;

   ASSERT(vcode >= 0 && vcode < 256);

   mycode = hw_to_mycode[vcode];
   _handle_key_press(ccode, mycode);
}




/* _al_win_kbd_handle_key_release:
 *  Does stuff when a key is released.
 */
void _al_win_kbd_handle_key_release(int vcode)
{
   int mycode;

   if (vcode == VK_SHIFT || vcode == VK_CONTROL || vcode == VK_MENU) {
     _al_win_kbd_update_shifts();
     return;
   }

   ASSERT(vcode >= 0 && vcode < 256);

   mycode = hw_to_mycode[vcode];
   _handle_key_release(mycode);
}



static int key_winapi_init(void)
{
   _al_win_kbd_update_shifts();
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
