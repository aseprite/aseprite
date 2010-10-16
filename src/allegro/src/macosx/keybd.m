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
 *      MacOS X keyboard module.
 *
 *      By Angelo Mottola.
 *
 *      Based on Unix/X11 version by Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error Something is wrong with the makefile
#endif


static int osx_keyboard_init(void);
static void osx_keyboard_exit(void);


/* Mac keycode to Allegro scancode conversion table */
static const int mac_to_scancode[128] = {
/* 0x00 */ KEY_A,          KEY_S,          KEY_D,          KEY_F,
/* 0x04 */ KEY_H,          KEY_G,          KEY_Z,          KEY_X,
/* 0x08 */ KEY_C,          KEY_V,          0,              KEY_B,
/* 0x0c */ KEY_Q,          KEY_W,          KEY_E,          KEY_R,
/* 0x10 */ KEY_Y,          KEY_T,          KEY_1,          KEY_2,
/* 0x14 */ KEY_3,          KEY_4,          KEY_6,          KEY_5,
/* 0x18 */ KEY_EQUALS,     KEY_9,          KEY_7,          KEY_MINUS,
/* 0x1c */ KEY_8,          KEY_0,          KEY_CLOSEBRACE, KEY_O,
/* 0x20 */ KEY_U,          KEY_OPENBRACE,  KEY_I,          KEY_P,
/* 0x24 */ KEY_ENTER,      KEY_L,          KEY_J,          KEY_QUOTE,
/* 0x28 */ KEY_K,          KEY_SEMICOLON,  KEY_BACKSLASH,  KEY_COMMA,
/* 0x2c */ KEY_SLASH,      KEY_N,          KEY_M,          KEY_STOP,
/* 0x30 */ KEY_TAB,        KEY_SPACE,      KEY_BACKQUOTE,  KEY_BACKSPACE,
/* 0x34 */ KEY_ENTER,      KEY_ESC,        0,              KEY_COMMAND,
/* 0x38 */ KEY_LSHIFT,     KEY_CAPSLOCK,   KEY_ALT,        KEY_LEFT,
/* 0x3c */ KEY_RIGHT,      KEY_DOWN,       KEY_UP,         0,
/* 0x40 */ 0,              KEY_STOP,       0,              KEY_ASTERISK,
/* 0x44 */ 0,              KEY_PLUS_PAD,   0,              KEY_NUMLOCK,
/* 0x48 */ 0,              0,              0,              KEY_SLASH_PAD,
/* 0x4c */ KEY_ENTER_PAD,  0,              KEY_MINUS_PAD,   0,
/* 0x50 */ 0,              KEY_EQUALS_PAD, KEY_0_PAD,      KEY_1_PAD,
/* 0x54 */ KEY_2_PAD,      KEY_3_PAD,      KEY_4_PAD,      KEY_5_PAD,
/* 0x58 */ KEY_6_PAD,      KEY_7_PAD,      0,              KEY_8_PAD,
/* 0x5c */ KEY_9_PAD,      0,              0,              0,
/* 0x60 */ KEY_F5,         KEY_F6,         KEY_F7,         KEY_F3,
/* 0x64 */ KEY_F8,         KEY_F9,         0,              KEY_F11,
/* 0x68 */ 0,              KEY_PRTSCR,     0,              KEY_SCRLOCK,
/* 0x6c */ 0,              KEY_F10,        0,              KEY_F12,
/* 0x70 */ 0,              KEY_PAUSE,      KEY_INSERT,     KEY_HOME,
/* 0x74 */ KEY_PGUP,       KEY_DEL,        KEY_F4,         KEY_END,
/* 0x78 */ KEY_F2,         KEY_PGDN,       KEY_F1,         KEY_LEFT,
/* 0x7c */ KEY_RIGHT,      KEY_DOWN,       KEY_UP,         0
};


static unsigned int old_mods = 0;


KEYBOARD_DRIVER keyboard_macosx =
{
   KEYBOARD_MACOSX,
   empty_string,
   empty_string,
   "MacOS X keyboard",
   TRUE,
   osx_keyboard_init,
   osx_keyboard_exit,
   NULL,   // AL_METHOD(void, poll, (void));
   NULL,   // AL_METHOD(void, set_leds, (int leds));
   NULL,   // AL_METHOD(void, set_rate, (int delay, int rate));
   NULL,   // AL_METHOD(void, wait_for_input, (void));
   NULL,   // AL_METHOD(void, stop_waiting_for_input, (void));
   NULL,   // AL_METHOD(int,  scancode_to_ascii, (int scancode));
   NULL    // scancode_to_name
};



/* osx_keyboard_handler:
 *  Keyboard "interrupt" handler.
 */
void osx_keyboard_handler(int pressed, NSEvent *event)
{
   const char character = [[event charactersIgnoringModifiers] lossyCString][0];
   int scancode = mac_to_scancode[[event keyCode]];
   int modifiers = [event modifierFlags];
   
   if (pressed) {
      if (modifiers & NSAlternateKeyMask)
         _handle_key_press(0, scancode);
      else {
         if ((modifiers & NSControlKeyMask) && (isalpha(character)))
            _handle_key_press(tolower(character) - 'a' + 1, scancode);
	 else
            _handle_key_press(character, scancode);
      }
      if ((three_finger_flag) &&
	  (scancode == KEY_END) && (_key_shifts & (KB_CTRL_FLAG | KB_ALT_FLAG))) {
	 raise(SIGTERM);
      }
   }
   else
      _handle_key_release(scancode);
}



/* osx_keyboard_modifier:
 *  Handles keyboard modifiers changes.
 */
void osx_keyboard_modifiers(unsigned int mods)
{
   unsigned const int mod_info[5][3] = { { NSAlphaShiftKeyMask, KB_CAPSLOCK_FLAG, KEY_CAPSLOCK },
                                         { NSShiftKeyMask,      KB_SHIFT_FLAG,    KEY_LSHIFT   },
		       	                 { NSControlKeyMask,    KB_CTRL_FLAG,     KEY_LCONTROL },
			                 { NSAlternateKeyMask,  KB_ALT_FLAG,      KEY_ALT      },
			                 { NSCommandKeyMask,    KB_COMMAND_FLAG,  KEY_COMMAND  } };
   int i, changed;
   
   for (i = 0; i < 5; i++) {
      changed = (mods ^ old_mods) & mod_info[i][0];
      if (changed) {
         if (mods & mod_info[i][0]) {
	    _key_shifts |= mod_info[i][1];
            _handle_key_press(-1, mod_info[i][2]);
	    if (i == 0)
	       /* Caps lock requires special handling */
	       _handle_key_release(mod_info[0][2]);
	 }
	 else {
	    _key_shifts &= ~mod_info[i][1];
	    if (i == 0)
	       _handle_key_press(-1, mod_info[0][2]);
	    _handle_key_release(mod_info[i][2]);
	 }
      }
   }
   old_mods = mods;
}



/* osx_keyboard_focused:
 *  Keyboard focus change handler.
 */
void osx_keyboard_focused(int focused, int state)
{
   int i, mask;

   if (focused) {
      mask = KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG | KB_CAPSLOCK_FLAG;
      _key_shifts = (_key_shifts & ~mask) | (state & mask);
   }
   else {
      for (i=0; i<KEY_MAX; i++) {
	 if (key[i])
	    _handle_key_release(i);
      }
   }
}



/* osx_keyboard_init:
 *  Installs the keyboard handler.
 */
static int osx_keyboard_init(void)
{
   return 0;
}



/* osx_keyboard_exit:
 *  Removes the keyboard handler.
 */
static void osx_keyboard_exit(void)
{
   osx_keyboard_focused(FALSE, 0);
}
