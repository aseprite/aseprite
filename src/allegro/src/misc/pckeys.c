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
 *      Shared helper routines for converting PC keyboard scancodes
 *      into a format that the main keyboard.c can understand, using
 *      the mapping tables from keyboard.dat.
 *
 *      By Shawn Hargreaves.
 *
 *      Salvador Eduardo Tropea added support for extended scancodes,
 *      keyboard LED's, capslock and numlock, and alt+numpad input.
 *
 *      Fabian Nunez added support for the special Microsoft keys.
 *
 *      Mathieu Lafon added support for the Pause and PrtScr keys and
 *      changed the key[] table to a normal/extended bitfield.
 *
 *      Dynamic keyboard switching by Peter Cech.
 *
 *      Peter Pavlovic improved the handling of accented keymaps.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



#define KB_MODIFIERS     (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG | KB_LWIN_FLAG | KB_RWIN_FLAG | KB_MENU_FLAG)
#define KB_LED_FLAGS     (KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG | KB_CAPSLOCK_FLAG)
#define KB_ACCENTS       (KB_ACCENT1_FLAG | KB_ACCENT2_FLAG | KB_ACCENT3_FLAG | KB_ACCENT4_FLAG)
#define KB_SH_CTRL_ALT   (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG)
#define KB_CTRL_ALT      (KB_CTRL_FLAG | KB_ALT_FLAG)


static int key_extended = FALSE;
static int key_altgr = FALSE;
static int key_pad_seq = 0;
static int key_pause_loop = 0;
static int key_paused = FALSE;

int _key_accent1 = 0;
int _key_accent2 = 0;
int _key_accent3 = 0;
int _key_accent4 = 0;

int _key_accent1_flag = 0;
int _key_accent2_flag = 0;
int _key_accent3_flag = 0;
int _key_accent4_flag = 0;

int _key_standard_kb = TRUE;

char *_keyboard_layout = NULL;

/* lookup table for converting hardware scancodes into Allegro format */
static unsigned char hw_to_mycode[128] =
{
   /* 0x00 */  0,              KEY_ESC,        KEY_1,          KEY_2, 
   /* 0x04 */  KEY_3,          KEY_4,          KEY_5,          KEY_6,
   /* 0x08 */  KEY_7,          KEY_8,          KEY_9,          KEY_0, 
   /* 0x0C */  KEY_MINUS,      KEY_EQUALS,     KEY_BACKSPACE,  KEY_TAB,
   /* 0x10 */  KEY_Q,          KEY_W,          KEY_E,          KEY_R, 
   /* 0x14 */  KEY_T,          KEY_Y,          KEY_U,          KEY_I,
   /* 0x18 */  KEY_O,          KEY_P,          KEY_OPENBRACE,  KEY_CLOSEBRACE, 
   /* 0x1C */  KEY_ENTER,      KEY_LCONTROL,   KEY_A,          KEY_S,
   /* 0x20 */  KEY_D,          KEY_F,          KEY_G,          KEY_H, 
   /* 0x24 */  KEY_J,          KEY_K,          KEY_L,          KEY_COLON,
   /* 0x28 */  KEY_QUOTE,      KEY_TILDE,      KEY_LSHIFT,     KEY_BACKSLASH, 
   /* 0x2C */  KEY_Z,          KEY_X,          KEY_C,          KEY_V,
   /* 0x30 */  KEY_B,          KEY_N,          KEY_M,          KEY_COMMA, 
   /* 0x34 */  KEY_STOP,       KEY_SLASH,      KEY_RSHIFT,     KEY_ASTERISK,
   /* 0x38 */  KEY_ALT,        KEY_SPACE,      KEY_CAPSLOCK,   KEY_F1, 
   /* 0x3C */  KEY_F2,         KEY_F3,         KEY_F4,         KEY_F5,
   /* 0x40 */  KEY_F6,         KEY_F7,         KEY_F8,         KEY_F9, 
   /* 0x44 */  KEY_F10,        KEY_NUMLOCK,    KEY_SCRLOCK,    KEY_7_PAD,
   /* 0x48 */  KEY_8_PAD,      KEY_9_PAD,      KEY_MINUS_PAD,  KEY_4_PAD, 
   /* 0x4C */  KEY_5_PAD,      KEY_6_PAD,      KEY_PLUS_PAD,   KEY_1_PAD,
   /* 0x50 */  KEY_2_PAD,      KEY_3_PAD,      KEY_0_PAD,      KEY_DEL_PAD, 
   /* 0x54 */  KEY_PRTSCR,     0,              KEY_BACKSLASH2, KEY_F11,
   /* 0x58 */  KEY_F12,        0,              0,              KEY_LWIN, 
   /* 0x5C */  KEY_RWIN,       KEY_MENU,       0,              0,
   /* 0x60 */  0,              0,              0,              0, 
   /* 0x64 */  0,              0,              0,              0,
   /* 0x68 */  0,              0,              0,              0, 
   /* 0x6C */  0,              0,              0,              0,
   /* 0x70 */  KEY_KANA,       0,              0,              KEY_ABNT_C1, 
   /* 0x74 */  0,              0,              0,              0,
   /* 0x78 */  0,              KEY_CONVERT,    0,              KEY_NOCONVERT, 
   /* 0x7C */  0,              KEY_YEN,        0,              0
};



/* lookup table for converting extended hardware codes into Allegro format */
static unsigned char hw_to_mycode_ex[128] =
{
   /* 0x00 */  0,              KEY_ESC,        KEY_1,          KEY_2,
   /* 0x04 */  KEY_3,          KEY_4,          KEY_5,          KEY_6,
   /* 0x08 */  KEY_7,          KEY_8,          KEY_9,          KEY_0,
   /* 0x0C */  KEY_MINUS,      KEY_EQUALS,     KEY_BACKSPACE,  KEY_TAB,
   /* 0x10 */  KEY_CIRCUMFLEX, KEY_AT,         KEY_COLON2,     KEY_R,
   /* 0x14 */  KEY_KANJI,      KEY_Y,          KEY_U,          KEY_I,
   /* 0x18 */  KEY_O,          KEY_P,          KEY_OPENBRACE,  KEY_CLOSEBRACE,
   /* 0x1C */  KEY_ENTER_PAD,  KEY_RCONTROL,   KEY_A,          KEY_S,
   /* 0x20 */  KEY_D,          KEY_F,          KEY_G,          KEY_H,
   /* 0x24 */  KEY_J,          KEY_K,          KEY_L,          KEY_COLON,
   /* 0x28 */  KEY_QUOTE,      KEY_TILDE,      0,              KEY_BACKSLASH,
   /* 0x2C */  KEY_Z,          KEY_X,          KEY_C,          KEY_V,
   /* 0x30 */  KEY_B,          KEY_N,          KEY_M,          KEY_COMMA,
   /* 0x34 */  KEY_STOP,       KEY_SLASH_PAD,  0,              KEY_PRTSCR,
   /* 0x38 */  KEY_ALTGR,      KEY_SPACE,      KEY_CAPSLOCK,   KEY_F1,
   /* 0x3C */  KEY_F2,         KEY_F3,         KEY_F4,         KEY_F5,
   /* 0x40 */  KEY_F6,         KEY_F7,         KEY_F8,         KEY_F9,
   /* 0x44 */  KEY_F10,        KEY_NUMLOCK,    KEY_PAUSE,      KEY_HOME,
   /* 0x48 */  KEY_UP,         KEY_PGUP,       KEY_MINUS_PAD,  KEY_LEFT,
   /* 0x4C */  KEY_5_PAD,      KEY_RIGHT,      KEY_PLUS_PAD,   KEY_END,
   /* 0x50 */  KEY_DOWN,       KEY_PGDN,       KEY_INSERT,     KEY_DEL,
   /* 0x54 */  KEY_PRTSCR,     0,              KEY_BACKSLASH2, KEY_F11,
   /* 0x58 */  KEY_F12,        0,              0,              KEY_LWIN,
   /* 0x5C */  KEY_RWIN,       KEY_MENU,       0,              0,
   /* 0x60 */  0,              0,              0,              0,
   /* 0x64 */  0,              0,              0,              0,
   /* 0x68 */  0,              0,              0,              0,
   /* 0x6C */  0,              0,              0,              0,
   /* 0x70 */  0,              0,              0,              0,
   /* 0x74 */  0,              0,              0,              0,
   /* 0x78 */  0,              0,              0,              0,
   /* 0x7C */  0,              0,              0,              0
};



/* convert Allegro format scancodes into key_shifts flag bits */
static unsigned short modifier_table[KEY_MAX - KEY_MODIFIERS] =
{
   KB_SHIFT_FLAG,    KB_SHIFT_FLAG,    KB_CTRL_FLAG,
   KB_CTRL_FLAG,     KB_ALT_FLAG,      KB_ALT_FLAG,
   KB_LWIN_FLAG,     KB_RWIN_FLAG,     KB_MENU_FLAG,
   KB_SCROLOCK_FLAG, KB_NUMLOCK_FLAG,  KB_CAPSLOCK_FLAG
};



/* convert numeric pad scancodes into arrow codes */
static unsigned char numlock_table[10] =
{
   KEY_INSERT, KEY_END,    KEY_DOWN,   KEY_PGDN,   KEY_LEFT,
   KEY_5_PAD,  KEY_RIGHT,  KEY_HOME,   KEY_UP,     KEY_PGUP
};



/* default mapping table for the US keyboard layout */
static unsigned short standard_key_ascii_table[KEY_MAX] =
{
   /* start */       0,
   /* alphabet */    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
   /* numbers */     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* numpad */      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* func keys */   0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* misc chars */  27, '`', '-', '=', 8, 9, '[', ']', 13, ';', '\'', '\\', '\\', ',', '.', '/', ' ',
   /* controls */    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* numpad */      '/', '*', '-', '+', '.', 13,
   /* others */      0, 0, 0, 0, 0, 0, 0, 0, 0, '\'', 0, 0, 0, 0, 0,
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/* capslock mapping table for the US keyboard layout */
static unsigned short standard_key_capslock_table[KEY_MAX] =
{
   /* start */       0,
   /* alphabet */    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
   /* numbers */     '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* numpad */      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* func keys */   0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* misc chars */  27, '`', '-', '=', 8, 9, '[', ']', 13, ';', '\'', '\\', '\\', ',', '.', '/', ' ',
   /* controls */    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* numpad */      '/', '*', '-', '+', '.', 13,
   /* others */      0, 0, 0, 0, 0, 0, 0, 0, 0, '\'', 0, 0, 0, 0, 0,
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/* shifted mapping table for the US keyboard layout */
static unsigned short standard_key_shift_table[KEY_MAX] =
{
   /* start */       0,
   /* alphabet */    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
   /* numbers */     ')', '!', '@', '#', '$', '%', '^', '&', '*', '(',
   /* numpad */      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* func keys */   0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* misc chars */  27, '~', '_', '+', 8, 9, '{', '}', 13, ':', '"', '|', '|', '<', '>', '?', ' ',
   /* controls */    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* numpad */      '/', '*', '-', '+', '.', 13,
   /* others */      0, 0, 0, 0, 0, 0, 0, 0, 0, '"', 0, 0, 0, 0, 0,
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/* ctrl+key mapping table for the US keyboard layout */
static unsigned short standard_key_control_table[KEY_MAX] =
{
   /* start */       0,
   /* alphabet */    1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26,
   /* numbers */     2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
   /* numpad */      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
   /* func keys */   0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* misc chars */  27, 2, 2, 2, 127, 127, 2, 2, 10, 2, 2, 2, 2, 2, 2, 2, 2,
   /* controls */    0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF,
   /* numpad */      2, 2, 2, 2, 2, 10,
   /* others */      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



/* empty table for stuff we don't need (accents in US map) */
static unsigned short standard_key_empty_table[KEY_MAX];



/* alternative mappings for custom keyboard layouts */
static unsigned short custom_key_ascii_table[KEY_MAX];
static unsigned short custom_key_capslock_table[KEY_MAX];
static unsigned short custom_key_shift_table[KEY_MAX];
static unsigned short custom_key_control_table[KEY_MAX];
static unsigned short custom_key_altgr_lower_table[KEY_MAX];
static unsigned short custom_key_altgr_upper_table[KEY_MAX];
static unsigned short custom_key_accent1_lower_table[KEY_MAX];
static unsigned short custom_key_accent1_upper_table[KEY_MAX];
static unsigned short custom_key_accent2_lower_table[KEY_MAX];
static unsigned short custom_key_accent2_upper_table[KEY_MAX];
static unsigned short custom_key_accent3_lower_table[KEY_MAX];
static unsigned short custom_key_accent3_upper_table[KEY_MAX];
static unsigned short custom_key_accent4_lower_table[KEY_MAX];
static unsigned short custom_key_accent4_upper_table[KEY_MAX];


/* shortcut pointers to the current layout mappings */
unsigned short *_key_ascii_table         = standard_key_ascii_table;
unsigned short *_key_capslock_table      = standard_key_capslock_table;
unsigned short *_key_shift_table         = standard_key_shift_table;
unsigned short *_key_control_table       = standard_key_control_table;
unsigned short *_key_altgr_lower_table   = standard_key_empty_table;
unsigned short *_key_altgr_upper_table   = standard_key_empty_table;
unsigned short *_key_accent1_lower_table = standard_key_empty_table;
unsigned short *_key_accent1_upper_table = standard_key_empty_table;
unsigned short *_key_accent2_lower_table = standard_key_empty_table;
unsigned short *_key_accent2_upper_table = standard_key_empty_table;
unsigned short *_key_accent3_lower_table = standard_key_empty_table;
unsigned short *_key_accent3_upper_table = standard_key_empty_table;
unsigned short *_key_accent4_lower_table = standard_key_empty_table;
unsigned short *_key_accent4_upper_table = standard_key_empty_table;



/* Table with key descriptions. */
char *_pckeys_names[KEY_MAX];



/* set_standard_keyboard:
 *  Sets up pointers ready to use the standard US keyboard mapping.
 */
static INLINE void set_standard_keyboard(void)
{
   _key_ascii_table         = standard_key_ascii_table;
   _key_capslock_table      = standard_key_capslock_table;
   _key_shift_table         = standard_key_shift_table;
   _key_control_table       = standard_key_control_table;
   _key_altgr_lower_table   = standard_key_empty_table;
   _key_altgr_upper_table   = standard_key_empty_table;
   _key_accent1_lower_table = standard_key_empty_table;
   _key_accent1_upper_table = standard_key_empty_table;
   _key_accent2_lower_table = standard_key_empty_table;
   _key_accent2_upper_table = standard_key_empty_table;
   _key_accent3_lower_table = standard_key_empty_table;
   _key_accent3_upper_table = standard_key_empty_table;
   _key_accent4_lower_table = standard_key_empty_table;
   _key_accent4_upper_table = standard_key_empty_table;

   _key_standard_kb = TRUE;
}



/* set_custom_keyboard:
 *  Sets up pointers ready to use the custom keyboard mapping.
 */
static INLINE void set_custom_keyboard(void)
{
   _key_ascii_table         = custom_key_ascii_table;
   _key_capslock_table      = custom_key_capslock_table;
   _key_shift_table         = custom_key_shift_table;
   _key_control_table       = custom_key_control_table;
   _key_altgr_lower_table   = custom_key_altgr_lower_table;
   _key_altgr_upper_table   = custom_key_altgr_upper_table;
   _key_accent1_lower_table = custom_key_accent1_lower_table;
   _key_accent1_upper_table = custom_key_accent1_upper_table;
   _key_accent2_lower_table = custom_key_accent2_lower_table;
   _key_accent2_upper_table = custom_key_accent2_upper_table;
   _key_accent3_lower_table = custom_key_accent3_lower_table;
   _key_accent3_upper_table = custom_key_accent3_upper_table;
   _key_accent4_lower_table = custom_key_accent4_lower_table;
   _key_accent4_upper_table = custom_key_accent4_upper_table;

   _key_standard_kb = FALSE;
}



/* _pckey_scancode_to_ascii:
 *  Looks up a scancode in the current mapping table.
 */
int _pckey_scancode_to_ascii(int scancode)
{
   int val;

   if ((scancode < 0) || (scancode >= KEY_MAX))
      return 0;

   val = _key_ascii_table[scancode];

   if (val == 0xFFFF)
      val = 0;

   return val;
}



/* _pckey_scancode_to_name:
 *  Like above, but returns a static string and also works for things like
 *  modifier keys.
 */
AL_CONST char *_pckey_scancode_to_name(int scancode)
{
   if ((scancode < 0) || (scancode >= KEY_MAX))
      return NULL;

   return _pckeys_names[scancode];
}



/* _handle_pckey:
 *  Handles PC keyboard input, in the same format it comes from the
 *  keyboard controller hardware (raw scancodes, top bit set means the
 *  key was released, special escapes for extended keys, pause, etc).
 *  This routine translates the data using the current mapping table,
 *  and calls routines from keyboard.c as required.
 */
void _handle_pckey(int code)
{
   int origcode, mycode, flag, numflag, i=0;
   unsigned short *table;

   if (key_pause_loop) { 
      /* skip multiple codes generated by the pause key */
      key_pause_loop--;
      return;
   }

   if (code == 0xE1) { 
      /* the pause key requires special handling */
      if (key_paused)
	 _handle_key_release(KEY_PAUSE);
      else
	 _handle_key_press(0, KEY_PAUSE);

      key_paused = !key_paused;
      key_pause_loop = 5;
      return;
   }

   if (code == 0xE0) {
      /* flag that the next key will be an extended one */
      key_extended = TRUE; 
      return;
   }

   /* convert from hardware to Allegro format */
   if (key_extended) {
      mycode = hw_to_mycode_ex[code & 0x7F];
      key_extended = FALSE;
   }
   else
      mycode = hw_to_mycode[code & 0x7F];

   if (!mycode)
      return;

   origcode = mycode;

   if (mycode >= KEY_MODIFIERS)
      flag = modifier_table[mycode - KEY_MODIFIERS];
   else
      flag = 0;

   numflag = ((_key_shifts & KB_NUMLOCK_FLAG) != 0) == ((_key_shifts & KB_SHIFT_FLAG) != 0);

   /* handle released keys */
   if (code & 0x80) {
      if (flag & KB_ALT_FLAG) {
	 /* end of an alt+numpad numeric entry sequence */
	 if (_key_shifts & KB_INALTSEQ_FLAG) {
	    _key_shifts &= ~KB_INALTSEQ_FLAG;
	    _handle_key_press(key_pad_seq, 0);
	 }
      }

      if (flag & KB_MODIFIERS) {
	 /* turn off the shift state for this key */
	 _key_shifts &= ~flag; 
	 if (mycode == KEY_ALTGR)
	    key_altgr = FALSE;
      }

      /* update the key array for a released key */
      _handle_key_release(mycode);
      return;
   }

   if ((mycode == KEY_F1) && ((_key_shifts & KB_CTRL_ALT) == KB_CTRL_ALT)) {
      /* switch to the standard keyboard layout */
      _handle_key_press(-1, KEY_F1);
      set_standard_keyboard();
      return;
   } 

   if ((mycode == KEY_F2) && ((_key_shifts & KB_CTRL_ALT) == KB_CTRL_ALT)) {
      /* switch to the custom keyboard layout */
      _handle_key_press(-1, KEY_F2);
      set_custom_keyboard();
      return;
   }

   if (flag & KB_MODIFIERS) {
      /* turn on a modifier key */
      _key_shifts |= flag;
      if (mycode == KEY_ALTGR)
	 key_altgr = TRUE;
      _handle_key_press(-1, origcode);
      return;
   }

   if ((flag & KB_LED_FLAGS) && (key_led_flag)) {
      /* toggle caps/num/scroll lock */
      _key_shifts ^= flag;
      _handle_key_press(-1, origcode);
      return;
   }

   /* new ACCENT stuff */
   if (!_key_standard_kb) {
      if ((mycode == _key_accent1) && ((_key_shifts & KB_SH_CTRL_ALT) == _key_accent1_flag)) {
	 _key_shifts |= KB_ACCENT1_FLAG;
	 _handle_key_press(-1, origcode);
	 return;
      }
      else if ((mycode == _key_accent2) && ((_key_shifts & KB_SH_CTRL_ALT) == _key_accent2_flag)) {
	 _key_shifts |= KB_ACCENT2_FLAG;
	 _handle_key_press(-1, origcode);
	 return;
      }
      else if ((mycode == _key_accent3) && ((_key_shifts & KB_SH_CTRL_ALT) == _key_accent3_flag)) {
	 _key_shifts |= KB_ACCENT3_FLAG;
	 _handle_key_press(-1, origcode);
	 return;
      }
      else if ((mycode == _key_accent4) && ((_key_shifts & KB_SH_CTRL_ALT) == _key_accent4_flag)) {
	 _key_shifts |= KB_ACCENT4_FLAG;
	 _handle_key_press(-1, origcode);
	 return;
      }
   }

   if (_key_shifts & KB_ACCENTS) {
      /* accented character input */
      if (((_key_shifts & KB_SHIFT_FLAG) != 0) ^ ((_key_shifts & KB_CAPSLOCK_FLAG) != 0)) {
	 if (_key_shifts & KB_ACCENT1_FLAG)
	    table = _key_accent1_upper_table;
	 else if (_key_shifts & KB_ACCENT2_FLAG)
	    table = _key_accent2_upper_table;
	 else if (_key_shifts & KB_ACCENT3_FLAG)
	    table = _key_accent3_upper_table;
	 else if (_key_shifts & KB_ACCENT4_FLAG)
	    table = _key_accent4_upper_table;
	 else
	    table = NULL;
      }
      else {
	 if (_key_shifts & KB_ACCENT1_FLAG)
	    table = _key_accent1_lower_table;
	 else if (_key_shifts & KB_ACCENT2_FLAG)
	    table = _key_accent2_lower_table;
	 else if (_key_shifts & KB_ACCENT3_FLAG)
	    table = _key_accent3_lower_table;
	 else if (_key_shifts & KB_ACCENT4_FLAG)
	    table = _key_accent4_lower_table;
	 else
	    table = NULL;
      }

      if (table[mycode]) {
	 /* simple accented char */
	 _key_shifts &= ~KB_ACCENTS;
	 _handle_key_press(table[mycode], origcode);
	 return;
      }
      else {
	 /* add the accent as an individual character */
	 if      (_key_shifts & (KB_ACCENT1_FLAG)) i = _key_accent1;
	 else if (_key_shifts & (KB_ACCENT2_FLAG)) i = _key_accent2;
	 else if (_key_shifts & (KB_ACCENT3_FLAG)) i = _key_accent3;
	 else if (_key_shifts & (KB_ACCENT4_FLAG)) i = _key_accent4;

	 _handle_key_press(_key_ascii_table[i], i);
	 _key_shifts &= ~KB_ACCENTS;
      }
   }

   if (_key_shifts & KB_ALT_FLAG) {
      if ((mycode >= KEY_0_PAD) && (mycode <= KEY_9_PAD)) { 
	 /* alt+numpad numeric entry */
	 if (_key_shifts & KB_INALTSEQ_FLAG) {
	    key_pad_seq = key_pad_seq*10 + mycode - KEY_0_PAD;
	 }
	 else {
	    _key_shifts |= KB_INALTSEQ_FLAG;
	    key_pad_seq = mycode - KEY_0_PAD;
	 }
	 _handle_key_press(-1, origcode);
	 return;
      }
      else {
	 /* alt+key */
	 if (_key_ascii_table[mycode] == 0xFFFF)
	    i = 0xFFFF;
         else if (key_altgr) {
            if (((_key_shifts & KB_SHIFT_FLAG) != 0) ^ ((_key_shifts & KB_CAPSLOCK_FLAG) != 0))
               i = _key_altgr_upper_table[mycode];
            else
               i = _key_altgr_lower_table[mycode];
         }
	 else
	    i = 0;
      }
   }
   else if ((mycode >= KEY_0_PAD) && (mycode <= KEY_9_PAD)) {
      /* handle numlock number->arrow conversions */
      i = mycode - KEY_0_PAD;
      if ((_key_shifts & KB_CTRL_FLAG) || (numflag)) {
	 mycode = numlock_table[i];
	 i = 0xFFFF;
      }
      else
	 i = _key_ascii_table[mycode];
   }
   else if (mycode == KEY_DEL_PAD) {
      /* handle numlock logic for the del key */
      if (numflag) {
	 mycode = KEY_DEL;
	 i = 0xFFFF;
      }
      else
	 i = _key_ascii_table[KEY_DEL_PAD];
   }
   else if (_key_shifts & KB_CTRL_FLAG) {
      /* ctrl+key */
      i = _key_control_table[mycode];
   }
   else if (_key_shifts & KB_SHIFT_FLAG) {
      /* shift+key */
      if (_key_shifts & KB_CAPSLOCK_FLAG) {
	 if (_key_ascii_table[mycode] == _key_capslock_table[mycode])
	    i = _key_shift_table[mycode];
	 else
	    i = _key_ascii_table[mycode];
      }
      else
	 i = _key_shift_table[mycode];
   }
   else if (_key_shifts & KB_CAPSLOCK_FLAG) {
      /* capslock+key */
      i = _key_capslock_table[mycode];
   }
   else {
      /* normal key */
      i = _key_ascii_table[mycode];
   }

   /* use the current modifier state in place of the key code? */
   if (i == 0xFFFF)
      i = _key_shifts & KB_MODIFIERS;

   _key_shifts &= ~KB_INALTSEQ_FLAG;

   /* phew! */
   _handle_key_press(i, origcode);
}

END_OF_FUNCTION(_handle_pckey);



/* read_key_table:
 *  Reads a specific keymapping table from the config file.
 */
static void read_key_table(unsigned short *out, unsigned short *in, char *section)
{
   char tmp1[64], tmp2[128], name[128];
   char *fmt = uconvert_ascii("key%d", tmp1);
   char *sec = uconvert_ascii(section, tmp2);
   int i;

   for (i=0; i<KEY_MAX; i++) {
      uszprintf(name, sizeof(name), fmt, i);
      out[i] = get_config_int(sec, name, in[i]);
   }
}



/* Updates the key descriptions from the ASCII values. */
static void update_key_descriptions(void)
{
   char str[64];
   int i;
   for (i = 0; i < KEY_MAX; i++)
   {
      int ascii = scancode_to_ascii (i);
      if (_pckeys_names[i])
            _AL_FREE(_pckeys_names[i]);
      if (ascii > ' ') {
         uszprintf(str, sizeof str, "%c", ascii);
         _pckeys_names[i] = strdup (str);
      }
      else {
         _pckeys_names[i] = strdup(_keyboard_common_names[i]);
      }
   }
}



/* read_keyboard_config:
 *  Reads in the keyboard config tables.
 */
static void read_keyboard_config(void)
{
   char filename[1024], tmp1[128], tmp2[128], *ext, *datafile;
   AL_CONST char* name;

   name = get_config_string(uconvert_ascii("system", tmp1), uconvert_ascii("keyboard", tmp2), _keyboard_layout);

   if ((!name) || (!ugetc(name)))
      return;

   ext = uconvert_ascii(".cfg", tmp1);
   datafile = uconvert_ascii("keyboard.dat", tmp2);

   if (find_allegro_resource(filename, name, ext, datafile, NULL, NULL, NULL, sizeof(filename)) != 0)
      return;

   push_config_state();
   set_config_file(filename);

   read_key_table(custom_key_ascii_table,          standard_key_ascii_table,     "key_ascii");
   read_key_table(custom_key_capslock_table,       standard_key_capslock_table,  "key_capslock");
   read_key_table(custom_key_shift_table,          standard_key_shift_table,     "key_shift");
   read_key_table(custom_key_control_table,        standard_key_control_table,   "key_control");

   /* preserve backward compatibility with former unique key_altgr table */
   read_key_table(custom_key_altgr_lower_table,    standard_key_empty_table,     "key_altgr");
   read_key_table(custom_key_altgr_upper_table,    standard_key_empty_table,     "key_altgr");

   read_key_table(custom_key_altgr_lower_table,    custom_key_altgr_lower_table, "key_altgr_lower");
   read_key_table(custom_key_altgr_upper_table,    custom_key_altgr_upper_table, "key_altgr_upper");

   read_key_table(custom_key_accent1_lower_table,  standard_key_empty_table,     "key_accent1_lower");
   read_key_table(custom_key_accent1_upper_table,  standard_key_empty_table,     "key_accent1_upper");
   read_key_table(custom_key_accent2_lower_table,  standard_key_empty_table,     "key_accent2_lower");
   read_key_table(custom_key_accent2_upper_table,  standard_key_empty_table,     "key_accent2_upper");
   read_key_table(custom_key_accent3_lower_table,  standard_key_empty_table,     "key_accent3_lower");
   read_key_table(custom_key_accent3_upper_table,  standard_key_empty_table,     "key_accent3_upper");
   read_key_table(custom_key_accent4_lower_table,  standard_key_empty_table,     "key_accent4_lower");
   read_key_table(custom_key_accent4_upper_table,  standard_key_empty_table,     "key_accent4_upper");

   _key_accent1 = get_config_int(uconvert_ascii("key_escape", tmp1), uconvert_ascii("accent1", tmp2), 0);
   _key_accent2 = get_config_int(uconvert_ascii("key_escape", tmp1), uconvert_ascii("accent2", tmp2), 0);
   _key_accent3 = get_config_int(uconvert_ascii("key_escape", tmp1), uconvert_ascii("accent3", tmp2), 0);
   _key_accent4 = get_config_int(uconvert_ascii("key_escape", tmp1), uconvert_ascii("accent4", tmp2), 0);

   _key_accent1_flag = get_config_int(uconvert_ascii("key_escape", tmp1), uconvert_ascii("accent1_flag", tmp2), 0);
   _key_accent2_flag = get_config_int(uconvert_ascii("key_escape", tmp1), uconvert_ascii("accent2_flag", tmp2), 0);
   _key_accent3_flag = get_config_int(uconvert_ascii("key_escape", tmp1), uconvert_ascii("accent3_flag", tmp2), 0);
   _key_accent4_flag = get_config_int(uconvert_ascii("key_escape", tmp1), uconvert_ascii("accent4_flag", tmp2), 0);

   pop_config_state();

   set_custom_keyboard();

   update_key_descriptions();
}



/* _pckeys_init:
 *  Initialises the scancode translation routines, loading mapping tables
 *  from keyboard.dat.
 */
void _pckeys_init(void)
{
   int i;

   _key_accent1 = 0;
   _key_accent2 = 0;
   _key_accent3 = 0;
   _key_accent4 = 0;
   _key_accent1_flag = 0;
   _key_accent2_flag = 0;
   _key_accent3_flag = 0;
   _key_accent4_flag = 0;

   key_extended = FALSE;
   key_altgr = FALSE;
   key_pad_seq = 0;
   key_pause_loop = 0;
   key_paused = FALSE;

   for (i=0; i<KEY_MAX; i++)
      standard_key_empty_table[i] = 0;

   memcpy(custom_key_ascii_table,            standard_key_ascii_table,     sizeof(custom_key_ascii_table));
   memcpy(custom_key_capslock_table,         standard_key_capslock_table,  sizeof(custom_key_capslock_table));
   memcpy(custom_key_shift_table,            standard_key_shift_table,     sizeof(custom_key_shift_table));
   memcpy(custom_key_control_table,          standard_key_control_table,   sizeof(custom_key_control_table));
   memcpy(custom_key_altgr_lower_table,      standard_key_empty_table,     sizeof(custom_key_altgr_lower_table));
   memcpy(custom_key_altgr_upper_table,      standard_key_empty_table,     sizeof(custom_key_altgr_upper_table));
   memcpy(custom_key_accent1_lower_table,    standard_key_empty_table,     sizeof(custom_key_accent1_lower_table));
   memcpy(custom_key_accent1_upper_table,    standard_key_empty_table,     sizeof(custom_key_accent1_upper_table));
   memcpy(custom_key_accent2_lower_table,    standard_key_empty_table,     sizeof(custom_key_accent2_lower_table));
   memcpy(custom_key_accent2_upper_table,    standard_key_empty_table,     sizeof(custom_key_accent2_upper_table));
   memcpy(custom_key_accent3_lower_table,    standard_key_empty_table,     sizeof(custom_key_accent1_lower_table));
   memcpy(custom_key_accent3_upper_table,    standard_key_empty_table,     sizeof(custom_key_accent1_upper_table));
   memcpy(custom_key_accent4_lower_table,    standard_key_empty_table,     sizeof(custom_key_accent2_lower_table));
   memcpy(custom_key_accent4_upper_table,    standard_key_empty_table,     sizeof(custom_key_accent2_upper_table));

   update_key_descriptions();

   LOCK_VARIABLE(hw_to_mycode);
   LOCK_VARIABLE(hw_to_mycode_ex);
   LOCK_VARIABLE(modifier_table);
   LOCK_VARIABLE(numlock_table);
   LOCK_VARIABLE(standard_key_ascii_table);
   LOCK_VARIABLE(standard_key_capslock_table);
   LOCK_VARIABLE(standard_key_shift_table);
   LOCK_VARIABLE(standard_key_control_table);
   LOCK_VARIABLE(standard_key_empty_table);
   LOCK_VARIABLE(custom_key_ascii_table);
   LOCK_VARIABLE(custom_key_capslock_table);
   LOCK_VARIABLE(custom_key_shift_table);
   LOCK_VARIABLE(custom_key_control_table);
   LOCK_VARIABLE(custom_key_altgr_lower_table);
   LOCK_VARIABLE(custom_key_altgr_upper_table);
   LOCK_VARIABLE(custom_key_accent1_lower_table);
   LOCK_VARIABLE(custom_key_accent1_upper_table);
   LOCK_VARIABLE(custom_key_accent2_lower_table);
   LOCK_VARIABLE(custom_key_accent2_upper_table);
   LOCK_VARIABLE(custom_key_accent3_lower_table);
   LOCK_VARIABLE(custom_key_accent3_upper_table);
   LOCK_VARIABLE(custom_key_accent4_lower_table);
   LOCK_VARIABLE(custom_key_accent4_upper_table);
   LOCK_VARIABLE(_key_ascii_table);
   LOCK_VARIABLE(_key_capslock_table);
   LOCK_VARIABLE(_key_shift_table);
   LOCK_VARIABLE(_key_control_table);
   LOCK_VARIABLE(_key_altgr_lower_table);
   LOCK_VARIABLE(_key_altgr_upper_table);
   LOCK_VARIABLE(_key_accent1_lower_table);
   LOCK_VARIABLE(_key_accent1_upper_table);
   LOCK_VARIABLE(_key_accent2_lower_table);
   LOCK_VARIABLE(_key_accent2_upper_table);
   LOCK_VARIABLE(_key_accent3_lower_table);
   LOCK_VARIABLE(_key_accent3_upper_table);
   LOCK_VARIABLE(_key_accent4_lower_table);
   LOCK_VARIABLE(_key_accent4_upper_table);
   LOCK_VARIABLE(_key_standard_kb);
   LOCK_VARIABLE(key_extended);
   LOCK_VARIABLE(key_altgr);
   LOCK_VARIABLE(key_pad_seq);
   LOCK_VARIABLE(key_pause_loop);
   LOCK_VARIABLE(key_paused);
   LOCK_VARIABLE(_key_accent1);
   LOCK_VARIABLE(_key_accent2);
   LOCK_VARIABLE(_key_accent3);
   LOCK_VARIABLE(_key_accent4);
   LOCK_VARIABLE(_key_accent1_flag);
   LOCK_VARIABLE(_key_accent2_flag);
   LOCK_VARIABLE(_key_accent3_flag);
   LOCK_VARIABLE(_key_accent4_flag);
   LOCK_FUNCTION(_handle_pckey);

   _key_standard_kb = TRUE;

   read_keyboard_config();
}

