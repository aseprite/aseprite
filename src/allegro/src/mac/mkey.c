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
 *      MacOs keyboard driver.
 *
 *      By Ronaldo H Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "macalleg.h"
#include "allegro/platform/aintmac.h"

#define TRACE_MAC_KBRD 0

volatile KeyMap KeyNow;
volatile KeyMap KeyOld;
short _mac_keyboard_installed=FALSE;
short volatile _mouse2nd = FALSE;

extern short tm_running;
extern volatile short _interrupt_time;

static int key_mac_init(void);
static void key_mac_exit(void);

#define k_apple_caps     0x3E
#define k_apple_shift    0x3F
#define k_apple_rshift   0x38
#define k_apple_alt      0x3D
#define k_apple_altgr    0x3A
#define k_apple_control  0x3C
#define k_apple_rcontrol 0x39
#define k_apple_command  0x30
#define k_2nd            0x30
/* mouse right button emulation */

/*
The LookUp table to translate AppleKeyboard scan codes to Allegro KEY constants
*/
const unsigned char key_apple_to_allegro[128]=
{
/*00*/   KEY_X,      KEY_Z,      KEY_G,      KEY_H, 
/*04*/   KEY_F,      KEY_D,      KEY_S,      KEY_A,   
/*08*/   KEY_R,      KEY_E,      KEY_W,      KEY_Q, 
/*0C*/   KEY_B,      KEY_TILDE,  KEY_V,      KEY_C, 
/*10*/   KEY_5,      KEY_6,      KEY_4,      KEY_3, 
/*14*/   KEY_2,      KEY_1,      KEY_T,      KEY_Y, 
/*18*/   KEY_O,      KEY_CLOSEBRACE, KEY_0,   KEY_8, 
/*1C*/   KEY_MINUS,  KEY_7,      KEY_9,      KEY_EQUALS, 
/*20*/   KEY_QUOTE,  KEY_J,      KEY_L,      KEY_ENTER, 
/*24*/   KEY_P,      KEY_I,      KEY_OPENBRACE, KEY_U, 
/*28*/   KEY_STOP,   KEY_M,      KEY_N,      KEY_SLASH, 
/*2C*/   KEY_COMMA,  KEY_BACKSLASH, KEY_COLON, KEY_K, 
/*30*/   /*KEY_LWIN used for right button of mouse*/ 
         0,          0,          KEY_ESC,    0,   
/*34*/   KEY_BACKSPACE, KEY_BACKSLASH2, KEY_SPACE, KEY_TAB, 
/*38*/   0,          KEY_RCONTROL, KEY_ALTGR,KEY_RSHIFT,   
/*3C*/   KEY_LCONTROL, KEY_ALT,   KEY_CAPSLOCK, KEY_LSHIFT, 
/*40*/   KEY_NUMLOCK,0,          KEY_PLUS_PAD, 0,   
/*44*/   KEY_ASTERISK, 0,        KEY_DEL_PAD, 0,   
/*48*/   0,          KEY_MINUS_PAD, 0,        KEY_ENTER_PAD,   
/*4C*/   KEY_SLASH_PAD, 0,        0,          0,   
/*50*/   KEY_5_PAD,  KEY_4_PAD,  KEY_3_PAD,   KEY_2_PAD, 
/*54*/   KEY_1_PAD,  KEY_0_PAD,  0,          0,   
/*58*/   0,          0,          0,          KEY_9_PAD, 
/*5C*/   KEY_8_PAD,  0,          KEY_7_PAD,  KEY_6_PAD,   
/*60*/   KEY_F11,    0,          KEY_F9,     KEY_F8, 
/*64*/   KEY_F3,     KEY_F7,     KEY_F6,     KEY_F5, 
/*68*/   KEY_F12,    0,          KEY_F10,    0,   
/*6C*/   KEY_SCRLOCK, 0,         KEY_PRTSCR, 0,   
/*70*/   KEY_END,    KEY_F4,     KEY_DEL,    KEY_PGUP, 
/*74*/   KEY_HOME,   KEY_INSERT, KEY_PAUSE,          0,   
/*78*/   0,          KEY_UP,     KEY_DOWN,   KEY_RIGHT, 
/*7C*/   KEY_LEFT,   KEY_F1,     KEY_PGDN,   KEY_F2, 
};

#define KB_MODIFIERS     (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG | KB_LWIN_FLAG | KB_RWIN_FLAG | KB_MENU_FLAG)
#define KB_LED_FLAGS     (KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG | KB_CAPSLOCK_FLAG)
#define KB_ACCENTS       (KB_ACCENT1_FLAG | KB_ACCENT2_FLAG | KB_ACCENT3_FLAG | KB_ACCENT4_FLAG)
#define KB_SH_CTRL_ALT   (KB_SHIFT_FLAG | KB_CTRL_FLAG | KB_ALT_FLAG)
#define KB_CTRL_ALT      (KB_CTRL_FLAG | KB_ALT_FLAG)


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
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
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
   /* modifiers */   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};



KEYBOARD_DRIVER keyboard_macos ={
   KEYBOARD_MACOS, 
   empty_string, 
   empty_string, 
   "MacOs Key", 
   TRUE, 
   key_mac_init, 
   key_mac_exit, 
   NULL, 
   NULL, NULL, 
   NULL, 
   NULL, 
   NULL
};



/*
 *
 */
void _handle_mac_key_press(short keycode)
{
   unsigned int ascii;
   if(keycode >= KEY_MODIFIERS && keycode<KEY_MAX ){
      if(keycode == KEY_SCRLOCK || keycode == KEY_NUMLOCK){
	 _key_shifts ^= modifier_table[keycode - KEY_MODIFIERS];
      }
      else
         _key_shifts |= modifier_table[keycode - KEY_MODIFIERS];
         ascii=0;
   }
   else if(_key_shifts & KB_CTRL_FLAG)
      ascii=standard_key_control_table[keycode];
   else if(_key_shifts & KB_CAPSLOCK_FLAG)
      ascii=standard_key_capslock_table[keycode];
   else if(_key_shifts & KB_SHIFT_FLAG)
      ascii=standard_key_shift_table[keycode];
   else
      ascii=standard_key_ascii_table[keycode];
   if(ascii == 0xFFFF)
      ascii = _key_shifts & KB_MODIFIERS;
   _handle_key_press(ascii, keycode);
}

END_OF_FUNCTION(_handle_mac_key_press);



/*
 *
 */
void _handle_mac_key_release(short keycode)
{
   if(keycode>=KEY_MODIFIERS&&keycode<KEY_MAX){
      if(keycode != KEY_SCRLOCK && keycode != KEY_NUMLOCK)
 	 _key_shifts &= ~modifier_table[keycode - KEY_MODIFIERS];
   }
   _handle_key_release(keycode);
}

END_OF_FUNCTION(_handle_mac_key_release);



/*
 *
 */
void _key_mac_interrupt()
{
   UInt32 xKeys;
   int a, row, base, keycode;
   KeyNow[0]=(*(UInt32*)0x174);
   KeyNow[1]=(*(UInt32*)0x178);
   KeyNow[2]=(*(UInt32*)0x17C);
   KeyNow[3]=(*(UInt32*)0x180);
   for(row=0, base=0;row<4;row++, base+=32) {
      xKeys=KeyOld[row]^KeyNow[row];
      if (xKeys) {
         for(a=0;a<32;a++) {
            if (BitTst(&xKeys, a)) {
               keycode=key_apple_to_allegro[a+base];
               if (keycode) {
                  if (BitTst( &KeyNow[row], a)) {
		     _handle_mac_key_press(keycode);
                  }
                  else{
                     _handle_mac_key_release(keycode);
                  }
	       }
	       else{
	          switch(a+base){
                     case k_2nd:
		        _mouse2nd = BitTst( &KeyNow[row], a);
		        break;
		     default:
		        break;
                  }	       
	       }
            }
         }
      }
      KeyOld[row]=KeyNow[row];
   }
}

END_OF_FUNCTION(_key_mac_interrupt);



/*
 *
 */
static int key_mac_init(void)
{

#if (TRACE_MAC_KBRD)
   fprintf(stdout, "key_mac_init()\n");
   fflush(stdout);
#endif
   GetKeys(KeyNow);
   _key_shifts=0;
   if(BitTst(KeyNow,k_apple_shift) || BitTst(KeyNow,k_apple_rshift))_key_shifts |= KB_SHIFT_FLAG;
   if(BitTst(KeyNow,k_apple_alt) || BitTst(KeyNow,k_apple_altgr))_key_shifts |= KB_ALT_FLAG;
   if(BitTst(KeyNow,k_apple_control) || BitTst(KeyNow,k_apple_rcontrol))_key_shifts |= KB_CTRL_FLAG;
   _mouse2nd=BitTst(KeyNow,k_2nd);
   LOCK_VARIABLE(KeyNow);
   LOCK_VARIABLE(KeyOld);
   LOCK_VARIABLE(key_apple_to_allegro);
   LOCK_VARIABLE(modifier_table);
   LOCK_VARIABLE(standard_key_ascii_table);
   LOCK_VARIABLE(standard_key_capslock_table);
   LOCK_VARIABLE(standard_key_shift_table);
   LOCK_VARIABLE(standard_key_control_table);
   LOCK_FUNCTION(_key_mac_interrupt);
   LOCK_FUNCTION(_handle_mac_key_press);
   LOCK_FUNCTION(_handle_mac_key_release);
   
   BlockMove(KeyNow, KeyOld, sizeof(KeyMap));
   if (!tm_running)
      _tm_sys_init();
   _mac_keyboard_installed=1;
   return 0;
}



/*
 *
 */
static void key_mac_exit(void)
{
#if (TRACE_MAC_KBRD)
   fprintf(stdout, "key_mac_exit()\n");
   fflush(stdout);
#endif
   _mac_keyboard_installed=0;
}

