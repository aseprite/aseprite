/*         ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      Keyboard routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_KEYBOARD_H
#define ALLEGRO_KEYBOARD_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

typedef struct KEYBOARD_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   int autorepeat;
   AL_METHOD(int,  init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(void, poll, (void));
   AL_METHOD(void, set_leds, (int leds));
   AL_METHOD(void, set_rate, (int delay, int rate));
   AL_METHOD(void, wait_for_input, (void));
   AL_METHOD(void, stop_waiting_for_input, (void));
   AL_METHOD(int,  scancode_to_ascii, (int scancode));
   AL_METHOD(AL_CONST char *, scancode_to_name, (int scancode));
} KEYBOARD_DRIVER;


AL_VAR(KEYBOARD_DRIVER *, keyboard_driver);
AL_ARRAY(_DRIVER_INFO, _keyboard_driver_list);

AL_FUNC(int, install_keyboard, (void));
AL_FUNC(void, remove_keyboard, (void));

AL_FUNC(int, poll_keyboard, (void));
AL_FUNC(int, keyboard_needs_poll, (void));

AL_FUNCPTR(int, keyboard_callback, (int key));
AL_FUNCPTR(int, keyboard_ucallback, (int key, int *scancode));
AL_FUNCPTR(void, keyboard_lowlevel_callback, (int scancode));

AL_FUNC(void, install_keyboard_hooks, (AL_METHOD(int, keypressed, (void)), AL_METHOD(int, readkey, (void))));

AL_ARRAY(volatile char, key);
AL_VAR(volatile int, key_shifts);

AL_VAR(int, three_finger_flag);
AL_VAR(int, key_led_flag);

AL_FUNC(int, keypressed, (void));
AL_FUNC(int, readkey, (void));
AL_FUNC(int, ureadkey, (int *scancode));
AL_FUNC(void, simulate_keypress, (int keycode));
AL_FUNC(void, simulate_ukeypress, (int keycode, int scancode));
AL_FUNC(void, clear_keybuf, (void));
AL_FUNC(void, set_leds, (int leds));
AL_FUNC(void, set_keyboard_rate, (int delay, int repeat));
AL_FUNC(int, scancode_to_ascii, (int scancode));
AL_FUNC(AL_CONST char *, scancode_to_name, (int scancode));

/* The KEY_ macros are no longer #defined directly to avoid conflicting with
 * linux (which has its own KEY_ constants).  "__allegro_"-prefixed constants
 * are used by the Allegro-Linux keyboard driver, but everyone else should
 * continue to use the KEY_ constants.
 */

enum {
   __allegro_KB_SHIFT_FLAG    = 0x0001,
   __allegro_KB_CTRL_FLAG     = 0x0002,
   __allegro_KB_ALT_FLAG      = 0x0004,
   __allegro_KB_LWIN_FLAG     = 0x0008,
   __allegro_KB_RWIN_FLAG     = 0x0010,
   __allegro_KB_MENU_FLAG     = 0x0020,
   __allegro_KB_COMMAND_FLAG  = 0x0040,
   __allegro_KB_SCROLOCK_FLAG = 0x0100,
   __allegro_KB_NUMLOCK_FLAG  = 0x0200,
   __allegro_KB_CAPSLOCK_FLAG = 0x0400,
   __allegro_KB_INALTSEQ_FLAG = 0x0800,
   __allegro_KB_ACCENT1_FLAG  = 0x1000,
   __allegro_KB_ACCENT2_FLAG  = 0x2000,
   __allegro_KB_ACCENT3_FLAG  = 0x4000,
   __allegro_KB_ACCENT4_FLAG  = 0x8000
};

enum {
   __allegro_KEY_A            = 1,
   __allegro_KEY_B            = 2,
   __allegro_KEY_C            = 3,
   __allegro_KEY_D            = 4,
   __allegro_KEY_E            = 5,
   __allegro_KEY_F            = 6,
   __allegro_KEY_G            = 7,
   __allegro_KEY_H            = 8,
   __allegro_KEY_I            = 9,
   __allegro_KEY_J            = 10,
   __allegro_KEY_K            = 11,
   __allegro_KEY_L            = 12,
   __allegro_KEY_M            = 13,
   __allegro_KEY_N            = 14,
   __allegro_KEY_O            = 15,
   __allegro_KEY_P            = 16,
   __allegro_KEY_Q            = 17,
   __allegro_KEY_R            = 18,
   __allegro_KEY_S            = 19,
   __allegro_KEY_T            = 20,
   __allegro_KEY_U            = 21,
   __allegro_KEY_V            = 22,
   __allegro_KEY_W            = 23,
   __allegro_KEY_X            = 24,
   __allegro_KEY_Y            = 25,
   __allegro_KEY_Z            = 26,
   __allegro_KEY_0            = 27,
   __allegro_KEY_1            = 28,
   __allegro_KEY_2            = 29,
   __allegro_KEY_3            = 30,
   __allegro_KEY_4            = 31,
   __allegro_KEY_5            = 32,
   __allegro_KEY_6            = 33,
   __allegro_KEY_7            = 34,
   __allegro_KEY_8            = 35,
   __allegro_KEY_9            = 36,
   __allegro_KEY_0_PAD        = 37,
   __allegro_KEY_1_PAD        = 38,
   __allegro_KEY_2_PAD        = 39,
   __allegro_KEY_3_PAD        = 40,
   __allegro_KEY_4_PAD        = 41,
   __allegro_KEY_5_PAD        = 42,
   __allegro_KEY_6_PAD        = 43,
   __allegro_KEY_7_PAD        = 44,
   __allegro_KEY_8_PAD        = 45,
   __allegro_KEY_9_PAD        = 46,
   __allegro_KEY_F1           = 47,
   __allegro_KEY_F2           = 48,
   __allegro_KEY_F3           = 49,
   __allegro_KEY_F4           = 50,
   __allegro_KEY_F5           = 51,
   __allegro_KEY_F6           = 52,
   __allegro_KEY_F7           = 53,
   __allegro_KEY_F8           = 54,
   __allegro_KEY_F9           = 55,
   __allegro_KEY_F10          = 56,
   __allegro_KEY_F11          = 57,
   __allegro_KEY_F12          = 58,
   __allegro_KEY_ESC          = 59,
   __allegro_KEY_TILDE        = 60,
   __allegro_KEY_MINUS        = 61,
   __allegro_KEY_EQUALS       = 62,
   __allegro_KEY_BACKSPACE    = 63,
   __allegro_KEY_TAB          = 64,
   __allegro_KEY_OPENBRACE    = 65,
   __allegro_KEY_CLOSEBRACE   = 66,
   __allegro_KEY_ENTER        = 67,
   __allegro_KEY_COLON        = 68,
   __allegro_KEY_QUOTE        = 69,
   __allegro_KEY_BACKSLASH    = 70,
   __allegro_KEY_BACKSLASH2   = 71,
   __allegro_KEY_COMMA        = 72,
   __allegro_KEY_STOP         = 73,
   __allegro_KEY_SLASH        = 74,
   __allegro_KEY_SPACE        = 75,
   __allegro_KEY_INSERT       = 76,
   __allegro_KEY_DEL          = 77,
   __allegro_KEY_HOME         = 78,
   __allegro_KEY_END          = 79,
   __allegro_KEY_PGUP         = 80,
   __allegro_KEY_PGDN         = 81,
   __allegro_KEY_LEFT         = 82,
   __allegro_KEY_RIGHT        = 83,
   __allegro_KEY_UP           = 84,
   __allegro_KEY_DOWN         = 85,
   __allegro_KEY_SLASH_PAD    = 86,
   __allegro_KEY_ASTERISK     = 87,
   __allegro_KEY_MINUS_PAD    = 88,
   __allegro_KEY_PLUS_PAD     = 89,
   __allegro_KEY_DEL_PAD      = 90,
   __allegro_KEY_ENTER_PAD    = 91,
   __allegro_KEY_PRTSCR       = 92,
   __allegro_KEY_PAUSE        = 93,
   __allegro_KEY_ABNT_C1      = 94,
   __allegro_KEY_YEN          = 95,
   __allegro_KEY_KANA         = 96,
   __allegro_KEY_CONVERT      = 97,
   __allegro_KEY_NOCONVERT    = 98,
   __allegro_KEY_AT           = 99,
   __allegro_KEY_CIRCUMFLEX   = 100,
   __allegro_KEY_COLON2       = 101,
   __allegro_KEY_KANJI        = 102,
   __allegro_KEY_EQUALS_PAD   = 103,  /* MacOS X */
   __allegro_KEY_BACKQUOTE    = 104,  /* MacOS X */
   __allegro_KEY_SEMICOLON    = 105,  /* MacOS X */
   __allegro_KEY_COMMAND      = 106,  /* MacOS X */
   __allegro_KEY_UNKNOWN1     = 107,
   __allegro_KEY_UNKNOWN2     = 108,
   __allegro_KEY_UNKNOWN3     = 109,
   __allegro_KEY_UNKNOWN4     = 110,
   __allegro_KEY_UNKNOWN5     = 111,
   __allegro_KEY_UNKNOWN6     = 112,
   __allegro_KEY_UNKNOWN7     = 113,
   __allegro_KEY_UNKNOWN8     = 114,

   __allegro_KEY_MODIFIERS    = 115,

   __allegro_KEY_LSHIFT       = 115,
   __allegro_KEY_RSHIFT       = 116,
   __allegro_KEY_LCONTROL     = 117,
   __allegro_KEY_RCONTROL     = 118,
   __allegro_KEY_ALT          = 119,
   __allegro_KEY_ALTGR        = 120,
   __allegro_KEY_LWIN         = 121,
   __allegro_KEY_RWIN         = 122,
   __allegro_KEY_MENU         = 123,
   __allegro_KEY_SCRLOCK      = 124,
   __allegro_KEY_NUMLOCK      = 125,
   __allegro_KEY_CAPSLOCK     = 126,

   __allegro_KEY_MAX          = 127
};

#ifndef ALLEGRO_NO_KEY_DEFINES

#define KB_SHIFT_FLAG         __allegro_KB_SHIFT_FLAG
#define KB_CTRL_FLAG          __allegro_KB_CTRL_FLAG
#define KB_ALT_FLAG           __allegro_KB_ALT_FLAG
#define KB_LWIN_FLAG          __allegro_KB_LWIN_FLAG
#define KB_RWIN_FLAG          __allegro_KB_RWIN_FLAG
#define KB_MENU_FLAG          __allegro_KB_MENU_FLAG
#define KB_COMMAND_FLAG       __allegro_KB_COMMAND_FLAG
#define KB_SCROLOCK_FLAG      __allegro_KB_SCROLOCK_FLAG
#define KB_NUMLOCK_FLAG       __allegro_KB_NUMLOCK_FLAG
#define KB_CAPSLOCK_FLAG      __allegro_KB_CAPSLOCK_FLAG
#define KB_INALTSEQ_FLAG      __allegro_KB_INALTSEQ_FLAG
#define KB_ACCENT1_FLAG       __allegro_KB_ACCENT1_FLAG
#define KB_ACCENT2_FLAG       __allegro_KB_ACCENT2_FLAG
#define KB_ACCENT3_FLAG       __allegro_KB_ACCENT3_FLAG
#define KB_ACCENT4_FLAG       __allegro_KB_ACCENT4_FLAG

#define KEY_A                 __allegro_KEY_A
#define KEY_B                 __allegro_KEY_B
#define KEY_C                 __allegro_KEY_C
#define KEY_D                 __allegro_KEY_D
#define KEY_E                 __allegro_KEY_E
#define KEY_F                 __allegro_KEY_F
#define KEY_G                 __allegro_KEY_G
#define KEY_H                 __allegro_KEY_H
#define KEY_I                 __allegro_KEY_I
#define KEY_J                 __allegro_KEY_J
#define KEY_K                 __allegro_KEY_K
#define KEY_L                 __allegro_KEY_L
#define KEY_M                 __allegro_KEY_M
#define KEY_N                 __allegro_KEY_N
#define KEY_O                 __allegro_KEY_O
#define KEY_P                 __allegro_KEY_P
#define KEY_Q                 __allegro_KEY_Q
#define KEY_R                 __allegro_KEY_R
#define KEY_S                 __allegro_KEY_S
#define KEY_T                 __allegro_KEY_T
#define KEY_U                 __allegro_KEY_U
#define KEY_V                 __allegro_KEY_V
#define KEY_W                 __allegro_KEY_W
#define KEY_X                 __allegro_KEY_X
#define KEY_Y                 __allegro_KEY_Y
#define KEY_Z                 __allegro_KEY_Z
#define KEY_0                 __allegro_KEY_0
#define KEY_1                 __allegro_KEY_1
#define KEY_2                 __allegro_KEY_2
#define KEY_3                 __allegro_KEY_3
#define KEY_4                 __allegro_KEY_4
#define KEY_5                 __allegro_KEY_5
#define KEY_6                 __allegro_KEY_6
#define KEY_7                 __allegro_KEY_7
#define KEY_8                 __allegro_KEY_8
#define KEY_9                 __allegro_KEY_9
#define KEY_0_PAD             __allegro_KEY_0_PAD
#define KEY_1_PAD             __allegro_KEY_1_PAD
#define KEY_2_PAD             __allegro_KEY_2_PAD
#define KEY_3_PAD             __allegro_KEY_3_PAD
#define KEY_4_PAD             __allegro_KEY_4_PAD
#define KEY_5_PAD             __allegro_KEY_5_PAD
#define KEY_6_PAD             __allegro_KEY_6_PAD
#define KEY_7_PAD             __allegro_KEY_7_PAD
#define KEY_8_PAD             __allegro_KEY_8_PAD
#define KEY_9_PAD             __allegro_KEY_9_PAD
#define KEY_F1                __allegro_KEY_F1
#define KEY_F2                __allegro_KEY_F2
#define KEY_F3                __allegro_KEY_F3
#define KEY_F4                __allegro_KEY_F4
#define KEY_F5                __allegro_KEY_F5
#define KEY_F6                __allegro_KEY_F6
#define KEY_F7                __allegro_KEY_F7
#define KEY_F8                __allegro_KEY_F8
#define KEY_F9                __allegro_KEY_F9
#define KEY_F10               __allegro_KEY_F10
#define KEY_F11               __allegro_KEY_F11
#define KEY_F12               __allegro_KEY_F12
#define KEY_ESC               __allegro_KEY_ESC
#define KEY_TILDE             __allegro_KEY_TILDE
#define KEY_MINUS             __allegro_KEY_MINUS
#define KEY_EQUALS            __allegro_KEY_EQUALS
#define KEY_BACKSPACE         __allegro_KEY_BACKSPACE
#define KEY_TAB               __allegro_KEY_TAB
#define KEY_OPENBRACE         __allegro_KEY_OPENBRACE
#define KEY_CLOSEBRACE        __allegro_KEY_CLOSEBRACE
#define KEY_ENTER             __allegro_KEY_ENTER
#define KEY_COLON             __allegro_KEY_COLON
#define KEY_QUOTE             __allegro_KEY_QUOTE
#define KEY_BACKSLASH         __allegro_KEY_BACKSLASH
#define KEY_BACKSLASH2        __allegro_KEY_BACKSLASH2
#define KEY_COMMA             __allegro_KEY_COMMA
#define KEY_STOP              __allegro_KEY_STOP
#define KEY_SLASH             __allegro_KEY_SLASH
#define KEY_SPACE             __allegro_KEY_SPACE
#define KEY_INSERT            __allegro_KEY_INSERT
#define KEY_DEL               __allegro_KEY_DEL
#define KEY_HOME              __allegro_KEY_HOME
#define KEY_END               __allegro_KEY_END
#define KEY_PGUP              __allegro_KEY_PGUP
#define KEY_PGDN              __allegro_KEY_PGDN
#define KEY_LEFT              __allegro_KEY_LEFT
#define KEY_RIGHT             __allegro_KEY_RIGHT
#define KEY_UP                __allegro_KEY_UP
#define KEY_DOWN              __allegro_KEY_DOWN
#define KEY_SLASH_PAD         __allegro_KEY_SLASH_PAD
#define KEY_ASTERISK          __allegro_KEY_ASTERISK
#define KEY_MINUS_PAD         __allegro_KEY_MINUS_PAD
#define KEY_PLUS_PAD          __allegro_KEY_PLUS_PAD
#define KEY_DEL_PAD           __allegro_KEY_DEL_PAD
#define KEY_ENTER_PAD         __allegro_KEY_ENTER_PAD
#define KEY_PRTSCR            __allegro_KEY_PRTSCR
#define KEY_PAUSE             __allegro_KEY_PAUSE
#define KEY_ABNT_C1           __allegro_KEY_ABNT_C1
#define KEY_YEN               __allegro_KEY_YEN
#define KEY_KANA              __allegro_KEY_KANA
#define KEY_CONVERT           __allegro_KEY_CONVERT
#define KEY_NOCONVERT         __allegro_KEY_NOCONVERT
#define KEY_AT                __allegro_KEY_AT
#define KEY_CIRCUMFLEX        __allegro_KEY_CIRCUMFLEX
#define KEY_COLON2            __allegro_KEY_COLON2
#define KEY_KANJI             __allegro_KEY_KANJI
#define KEY_EQUALS_PAD        __allegro_KEY_EQUALS_PAD
#define KEY_BACKQUOTE         __allegro_KEY_BACKQUOTE
#define KEY_SEMICOLON         __allegro_KEY_SEMICOLON
#define KEY_COMMAND           __allegro_KEY_COMMAND
#define KEY_UNKNOWN1          __allegro_KEY_UNKNOWN1
#define KEY_UNKNOWN2          __allegro_KEY_UNKNOWN2
#define KEY_UNKNOWN3          __allegro_KEY_UNKNOWN3
#define KEY_UNKNOWN4          __allegro_KEY_UNKNOWN4
#define KEY_UNKNOWN5          __allegro_KEY_UNKNOWN5
#define KEY_UNKNOWN6          __allegro_KEY_UNKNOWN6
#define KEY_UNKNOWN7          __allegro_KEY_UNKNOWN7
#define KEY_UNKNOWN8          __allegro_KEY_UNKNOWN8

#define KEY_MODIFIERS         __allegro_KEY_MODIFIERS

#define KEY_LSHIFT            __allegro_KEY_LSHIFT
#define KEY_RSHIFT            __allegro_KEY_RSHIFT
#define KEY_LCONTROL          __allegro_KEY_LCONTROL
#define KEY_RCONTROL          __allegro_KEY_RCONTROL
#define KEY_ALT               __allegro_KEY_ALT
#define KEY_ALTGR             __allegro_KEY_ALTGR
#define KEY_LWIN              __allegro_KEY_LWIN
#define KEY_RWIN              __allegro_KEY_RWIN
#define KEY_MENU              __allegro_KEY_MENU
#define KEY_SCRLOCK           __allegro_KEY_SCRLOCK
#define KEY_NUMLOCK           __allegro_KEY_NUMLOCK
#define KEY_CAPSLOCK          __allegro_KEY_CAPSLOCK

#define KEY_MAX               __allegro_KEY_MAX

#endif /* ALLEGRO_NO_KEY_DEFINES */

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_KEYBOARD_H */


