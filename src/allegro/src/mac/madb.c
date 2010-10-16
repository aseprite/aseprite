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
 *      Mac ADB drivers.
 *
 *      By Ronaldo H Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "macalleg.h"
#include "allegro/platform/aintmac.h"
#include <DeskBus.h>

extern volatile KeyMap KeyNow;
extern volatile KeyMap KeyOld;

#define ADB_LISTENMASK     8
#define ADB_TALKMASK       12
#define ADB_COMMANDMASK    12
#define ADB_TYPEREGISTER   3
#define ADB_KEYLEDREGISTER 2
#define ADB_KEYLEDMASK     7
#define ADB_MOUSE          3
#define ADB_KEYBOARD       2
#define ADB_ALLEGROTYPE    3

volatile int adb_command_done = 1;
extern volatile short _interrupt_time;
extern volatile short _mouse2nd;


int adb_keyboard_id=0;
ADBDataBlock adb_key_save;
ADBServiceRoutineUPP adb_key_upp;
ADBCompletionUPP adb_key_cpt_upp;
static int key_adb_init(void);
static void key_adb_exit(void);


int adb_mouse_id=0;
ADBDataBlock adb_mouse_save;
ADBServiceRoutineUPP adb_mouse_upp;
ADBCompletionUPP adb_mouse_cpt_upp;
static int mouse_adb_init(void);
static void mouse_adb_exit(void);
static void mouse_adb_position(int x, int y);
static void mouse_adb_set_range(int x1, int y1, int x2, int y2);
static void mouse_adb_get_mickeys(int *mickeyx, int *mickeyy);
static short mouse_x_min;
static short mouse_y_min;
static short mouse_x_max;
static short mouse_y_max;
static short mickey_x;
static short mickey_y;

KEYBOARD_DRIVER keyboard_adb ={
   KEYBOARD_ADB, 
   empty_string, 
   empty_string, 
   "Apple Desktop Bus keyboard", 
   TRUE, 
   key_adb_init, 
   key_adb_exit, 
   NULL, 
   NULL, NULL, 
   NULL, 
   NULL, 
   NULL
};

MOUSE_DRIVER mouse_adb ={
      MOUSE_ADB, 
      empty_string, 
      empty_string, 
      "Apple Desktop Bus Mouse", 
      mouse_adb_init, 
      mouse_adb_exit, 
      NULL,
      NULL,
      mouse_adb_position,
      mouse_adb_set_range,
      NULL,
      mouse_adb_get_mickeys,
      NULL,
      NULL
};


#define k_apple_caps     0x3E
#define k_apple_shift    0x3F
#define k_apple_rshift   0x38
#define k_apple_alt      0x3D
#define k_apple_altgr    0x3A
#define k_apple_control  0x3C
#define k_apple_rcontrol 0x39
#define k_apple_command  0x30
#define k_2nd k_apple_command
#define kk_2nd           0x37

static const unsigned char adb_to_allegro[]={
/*00*/          KEY_A,          KEY_S,          KEY_D,          KEY_F,
                KEY_H,          KEY_G,          KEY_Z,          KEY_X,
                KEY_C,          KEY_V,          KEY_TILDE,      KEY_B,
                KEY_Q,          KEY_W,          KEY_E,          KEY_R,
/*10*/          KEY_Y,          KEY_T,          KEY_1,          KEY_2,
                KEY_3,          KEY_4,          KEY_6,          KEY_5,
                KEY_EQUALS,     KEY_9,          KEY_7,          KEY_MINUS,
                KEY_8,          KEY_0,          KEY_CLOSEBRACE, KEY_O,
/*20*/          KEY_U,          KEY_OPENBRACE,  KEY_I,          KEY_P,
                KEY_ENTER,      KEY_L,          KEY_J,          KEY_QUOTE,
                KEY_K,          KEY_COLON,      KEY_BACKSLASH,  KEY_COMMA,
                KEY_BACKSLASH2, KEY_N,          KEY_M,          KEY_STOP,
/*30*/          KEY_TAB,        KEY_SPACE,      KEY_SLASH,      KEY_BACKSPACE,
                0,              KEY_ESC,        KEY_LCONTROL,   0/*KEY_MENU*/,
                KEY_LSHIFT,     KEY_CAPSLOCK,   KEY_ALT,        KEY_LEFT,
                KEY_RIGHT,      KEY_DOWN,       KEY_UP,         0,
/*10*/          0,              0,              0,              KEY_ASTERISK,
                0,              KEY_PLUS_PAD,   0,              KEY_NUMLOCK,
                0,              0,              0,              KEY_SLASH_PAD,
                KEY_ENTER_PAD,  0,              KEY_MINUS_PAD,  0,
                0,              KEY_DEL_PAD,    KEY_0_PAD,      KEY_1_PAD,
                KEY_2_PAD,      KEY_3_PAD,      KEY_4_PAD,      KEY_5_PAD,
                KEY_6_PAD,      KEY_7_PAD,      0,              KEY_8_PAD,
                KEY_9_PAD,      0,              0,              0,
                KEY_F5,         KEY_F6,         KEY_F7,         KEY_F3,
                KEY_F8,         KEY_F9,         0,              KEY_F11,
                0,              KEY_PRTSCR,     0,              KEY_SCRLOCK,
                0,              KEY_F10,        0,              KEY_F12,
                0,              KEY_PAUSE,      KEY_INSERT,     KEY_HOME,
                KEY_PGUP,       KEY_DEL,        KEY_F4,         KEY_END,
                KEY_F2,         KEY_PGDN,       KEY_F1,         KEY_RSHIFT,
                KEY_ALTGR,      KEY_RCONTROL,   0,              0,
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



/*
 *
 */
static pascal void adb_command_callback (Ptr buffer, Ptr refCon, long command)
{
#pragma unused buffer, command
   *refCon=1;
}

END_OF_STATIC_FUNCTION(adb_command_callback);



/*
 *   send a command to an ADB device 
 */
static OSErr send_adb_command_sync (Ptr myBufferPtr,short adb_command)
{
   OSErr e;
   adb_command_done = 0;
   e = ADBOp((Ptr)&adb_command_done, adb_key_cpt_upp, myBufferPtr, adb_command);
   if (e == noErr && !_interrupt_time)
   {
     while(!adb_command_done){};
   }
   return e;
};



/*
 *
 */
static pascal void key_adb_interrupt(Ptr buffer, TempADBServiceRoutineUPP completionProc, Ptr refCon, long command)
{
   extern short _mac_keyboard_installed;
   unsigned char origcode;
   unsigned char keycode;
   int a;
   if((command & 0xF) == 0xC){
      for(a = 1; a < 3 ; a++ ){
         origcode=buffer[a];
	 keycode=adb_to_allegro[origcode&0x7f];
         if(keycode!=0){
            if(origcode&0x80){
               if(keycode >= KEY_MODIFIERS && keycode<KEY_MAX )
                  if(keycode != KEY_SCRLOCK && keycode != KEY_NUMLOCK)
                     _key_shifts &= ~modifier_table[keycode - KEY_MODIFIERS];
               _handle_key_release(keycode);
            }
            else{
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
               _handle_key_press(ascii,keycode);
            }
	 }
	 else if(origcode == kk_2nd){
	    _mouse2nd = TRUE;
	 }
	 else if(origcode == kk_2nd+0x80){
	    _mouse2nd = FALSE;
	 }
	 // Debug Hack <CTRL-SHIFT-ON> restore system driver
	 else if(origcode == 0x7F && _key_shifts & KB_SHIFT_FLAG && _key_shifts & KB_CTRL_FLAG){
            ADBAddress adb_address; 
            ADBSetInfoBlock mySet;
            ADBDataBlock now_data;
            adb_address = GetIndADB(&now_data, adb_keyboard_id);
            mySet.siService = adb_key_save.dbServiceRtPtr;
            mySet.siDataAreaAddr = adb_key_save.dbDataAreaAddr;
            SetADBInfo (&mySet,adb_address);
	    _mac_keyboard_installed=TRUE;
	    keyboard_driver = &keyboard_macos;
	 }
      }
   }
}
END_OF_STATIC_FUNCTION(key_adb_interrupt);



/*
 *
 */
static void key_adb_set_leds(int leds)
{
/*   short adb_keyboard_id;*/
/*   ADBDataBlock adb_key_save;*/
/*   ADBAddress adb_address;*/
/*   */
/*   adb_leds = (leds & KB_NUMLOCK_FLAG)?0:1;*/
/*   adb_leds += (leds & KB_CAPSLOCK_FLAG)?0:2;*/
/*   adb_leds += (leds & KB_SCROLOCK_FLAG)?0:4;*/
/**/
/*   for(adb_keyboard_id = 1; adb_keyboard_id <= adb_adb_keyboard_ids; adb_keyboard_id++){*/
/*      adb_address = GetIndADB(&adb_key_save, adb_keyboard_id);*/
/*      if(GetADBInfo(&adb_key_save, adb_address) == noErr){*/
/*         if(adb_key_save.origADBAddr == 2 && adb_controls[adb_keyboard_id].type==EXTKEYBOARD){*/
/*            adb_controls[adb_keyboard_id].adb_key_registers[0] = 2;*/
/*            ADBOp(NULL, led_upp, adb_controls[adb_keyboard_id].adb_key_registers,*/
/*	          (adb_address << 4) + ADB_TALKMASK + ADB_KEYLEDREGISTER);*/
/*         }*/
/*      }*/
/*   }*/
}



/*
 *
 */
static int key_adb_init(void)
{
   OSErr e;
   short adb_command;
   short adb_devices;
   char adb_key_registers[8];
   ADBAddress adb_address;
   ADBSetInfoBlock mySet;

   adb_key_cpt_upp = NewADBCompletionUPP(adb_command_callback);
   adb_key_upp = NewADBServiceRoutineUPP (key_adb_interrupt);

   adb_devices = CountADBs();
   for(adb_keyboard_id = 1 ; adb_keyboard_id <= adb_devices ; adb_keyboard_id++){
      adb_address = GetIndADB(&adb_key_save, adb_keyboard_id);
      e = GetADBInfo(&adb_key_save, adb_address);
      if(adb_key_save.origADBAddr == ADB_KEYBOARD){
         switch (adb_key_save.devType){
            case 2:case 3:case 5:
	       LOCK_VARIABLE(KeyNow);
 	       LOCK_VARIABLE(KeyOld);
 	       LOCK_VARIABLE(_mouse2nd);
 	       LOCK_VARIABLE(adb_to_allegro);
 	       LOCK_VARIABLE(modifier_table);
	       LOCK_VARIABLE(standard_key_ascii_table);
	       LOCK_VARIABLE(standard_key_capslock_table);
	       LOCK_VARIABLE(standard_key_shift_table);
 	       LOCK_VARIABLE(standard_key_control_table);
/*	       LOCK_FUNCTION(key_adb_interrupt);*/
/*	       LOCK_FUNCTION(adb_command_callback);*/

               mySet.siService = adb_key_upp;
	       mySet.siDataAreaAddr = NULL;
	       SetADBInfo (&mySet,adb_address);
               adb_key_registers[0] = 2;
               adb_command = (adb_address * 16) + ADB_TALKMASK + ADB_TYPEREGISTER;
	       if (send_adb_command_sync(adb_key_registers, adb_command) == noErr){
		  adb_key_registers[2] = 3;
                  adb_command = (adb_address * 16) + ADB_LISTENMASK + ADB_TYPEREGISTER;
                  send_adb_command_sync(adb_key_registers, adb_command);
               }
               GetKeys(KeyNow);
               _key_shifts=0;

               if(BitTst(KeyNow,k_apple_shift) || BitTst(KeyNow,k_apple_rshift))_key_shifts |= KB_SHIFT_FLAG;
               if(BitTst(KeyNow,k_apple_alt) || BitTst(KeyNow,k_apple_altgr))_key_shifts |= KB_ALT_FLAG;
               if(BitTst(KeyNow,k_apple_control) || BitTst(KeyNow,k_apple_rcontrol))_key_shifts |= KB_CTRL_FLAG;
               _mouse2nd = BitTst(KeyNow,k_2nd);
               return 0;
            default:
               break;
      	 }
      }
   }
   adb_keyboard_id=0;
   return -1;
}



/*
 *
 */
static void key_adb_exit(void)
{
   OSErr e;
   short adb_command;
   short tmp;
   char adb_key_registers[8];
   ADBAddress adb_address;
   ADBDataBlock now_data;
   ADBSetInfoBlock mySet;
   if(adb_keyboard_id!=0){
      adb_address = GetIndADB(&now_data, adb_keyboard_id);
      e = GetADBInfo(&now_data, adb_address);
      if(now_data.origADBAddr == 2){
         adb_key_registers[0] = 2;
         adb_command = (adb_address * 16) + ADB_TALKMASK + ADB_TYPEREGISTER;
         if (send_adb_command_sync(adb_key_registers, adb_command) == noErr){
            tmp=adb_key_save.devType;
            if(tmp == 3 || tmp ==5)
               adb_key_registers[2] = tmp;
            else
               adb_key_registers[2] = 2;
	    adb_command = (adb_address * 16) + ADB_LISTENMASK + ADB_TYPEREGISTER;
            send_adb_command_sync(adb_key_registers, adb_command);
         }
      }
      mySet.siService = adb_key_save.dbServiceRtPtr;
      mySet.siDataAreaAddr = adb_key_save.dbDataAreaAddr;
      SetADBInfo (&mySet,adb_address);
      adb_keyboard_id=0;
   }
   DisposeADBServiceRoutineUPP (adb_key_upp);
   DisposeADBCompletionUPP(adb_key_cpt_upp);
}



/*
 *
 */
static pascal void mouse_adb_interrupt(Ptr buffer, TempADBServiceRoutineUPP completionProc, Ptr refCon, long command)
{
   signed char tmp;
   tmp=buffer[1];
   _mouse_b = tmp&0x80?0:(_mouse2nd?2:1);
   if(tmp & 0x40)
      tmp |= 0xC0;
   else
      tmp &= 0x3F;
   _mouse_y += tmp;
   mickey_y += tmp;
   _mouse_y = CLAMP(mouse_y_min,mouse_y_max,_mouse_y);

   tmp=buffer[2];
   _mouse_b |= tmp&0x80?0:2;
   if(tmp & 0x40)
      tmp |= 0xC0;
   else
      tmp &= 0x3F;
   _mouse_x += tmp;
   mickey_x += tmp;
   _mouse_x = CLAMP(mouse_x_min,mouse_x_max,_mouse_x);
   
   _handle_mouse_input();
}

END_OF_STATIC_FUNCTION(mouse_adb_interrupt);



/* mick_position:
 *  Sets the position of the mickey-mode mouse.
 */
static void mouse_adb_position(int x, int y)
{
   _mouse_x = x;
   _mouse_y = y;
}



/*
 *
 */
static void mouse_adb_set_range(int x1, int y1, int x2, int y2)
{
   mouse_x_min = x1;
   mouse_y_min = y1;
   mouse_x_max = x2;
   mouse_y_max = y2;

   _mouse_x = CLAMP(mouse_x_min, _mouse_x, mouse_x_max);
   _mouse_y = CLAMP(mouse_x_min, _mouse_y, mouse_y_max);
}



/* 
 *
 */
static void mouse_adb_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mickey_x;
   int temp_y = mickey_y;

   mickey_x -= temp_x;
   mickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}



/*
 *
 */
static int mouse_adb_init(void)
{
   OSErr e;
   ADBAddress adb_address;
   ADBSetInfoBlock mySet;
   short adb_devices;

   adb_mouse_cpt_upp = NewADBCompletionUPP(adb_command_callback);
   adb_mouse_upp = NewADBServiceRoutineUPP (mouse_adb_interrupt);

   adb_devices = CountADBs();
   for(adb_mouse_id = 1 ; adb_mouse_id <= adb_devices ; adb_mouse_id++){
      adb_address = GetIndADB(&adb_mouse_save, adb_mouse_id);
      e = GetADBInfo(&adb_mouse_save, adb_address);
      if(adb_mouse_save.origADBAddr == ADB_MOUSE){
         switch (adb_mouse_save.devType){
            case 1:
	       LOCK_VARIABLE(mouse_x_min);
	       LOCK_VARIABLE(mouse_y_min);
	       LOCK_VARIABLE(mouse_x_max);
	       LOCK_VARIABLE(mouse_y_max);
	       LOCK_VARIABLE(mickey_x);
	       LOCK_VARIABLE(mickey_y);
	       LOCK_VARIABLE(_mouse2nd);
	       mickey_x=0;
	       mickey_y=0;
               mySet.siService = adb_mouse_upp;
	       mySet.siDataAreaAddr = NULL;
	       SetADBInfo (&mySet,adb_address);
	       _mouse_x = 0;
	       _mouse_y = 0;
	       _mouse_b = 0;
               return 2;
            default:
               break;
      	 }
      }
   }
   adb_mouse_id=0;
   return -1;
}



/*
 *
 */
static void mouse_adb_exit(void)
{
   ADBAddress adb_address;
   ADBDataBlock now_data;
   ADBSetInfoBlock mySet;
   if(adb_mouse_id != 0){
      adb_address = GetIndADB(&now_data, adb_mouse_id);
      mySet.siService = adb_mouse_save.dbServiceRtPtr;
      mySet.siDataAreaAddr = adb_mouse_save.dbDataAreaAddr;
      SetADBInfo (&mySet,adb_address);
      adb_mouse_id = 0;
   }
   DisposeADBServiceRoutineUPP (adb_mouse_upp);
   DisposeADBCompletionUPP(adb_mouse_cpt_upp);
}

