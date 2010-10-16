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
 *      BeOS keyboard driver.
 *
 *      By Jason Wilkins, rewritten by Angelo Mottola to use the unified
 *      pckeys API.
 *
 *      See readme.txt for copyright information.
 */

#include <stdlib.h>

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif                

#define KEY_ID_PC101 0 // the docs say it should be 0x83ab, but they lie

#define KEY_SEMAPHORE_NAME "keyboard driver waiting..."

#define KEY_THREAD_PERIOD   33333             // microseconds, 1/30th of a second
#define KEY_THREAD_NAME     "keyboard driver"
#define KEY_THREAD_PRIORITY 60               // above average

#define BE_KEY_PAUSE        16
#define BE_KEY_NUMLOCK      34
#define BE_KEY_CAPSLOCK     59
#define BE_KEY_SCROLOCK     15


#define PREFIX_I                "al-bkey INFO: "
#define PREFIX_W                "al-bkey WARNING: "
#define PREFIX_E                "al-bkey ERROR: "

static uint16 keyboard_id = (uint16)(-1);

static volatile int keyboard_thread_running = FALSE;
static thread_id    keyboard_thread_id      = -1;

static sem_id waiting_for_input = -1;

static const int be_to_pc[128] = {
// Scancode		Be	Key
   -1,			//	0	(not used)
   0x1,			//	1	ESC
   0x3b,		//	2	F1
   0x3c,		//	3	F2
   0x3d,		//	4	F3
   0x3e,		//	5	F4
   0x3f,		//	6	F5
   0x40,		//	7	F6
   0x41, 		//	8	F7
   0x42,		//	9	F8
   0x43,		//	10	F9
   0x44,		//	11	F10
   0x57,		//	12	F11
   0x58,		// 	13	F12
   0x54,		//	14	Print Screen / SysRq
   0x46,		//	15	Scroll Lock
   0,			//	16	Pause / Break
   0x29,		//	17	Tilde
   0x2,			//	18	'1'
   0x3,			//	19	'2'
   0x4,			//	20	'3'
   0x5,			//	21	'4'
   0x6,			//	22	'5'
   0x7,			//	23	'6'
   0x8,			//	24	'7'
   0x9,			//	25	'8'
   0xa,			//	26	'9'
   0xb,			//	27	'0'
   0xc,			//	28	'-'
   0xd,			//	29	'='
   0xe,			//	30	Backspace
   0x5200,		//	31	Insert
   0x4700,		//	32	Home
   0x4900,		//	33	Page Up
   0x45,		//	34	Num Lock
   0x3500,		//	35	Pad '/'
   0x37,		//	36	Pad '*'
   0x4a,		//	37	Pad '-'
   0xf,			//	38	Tab
   0x10,		//	39	'q'
   0x11,		//	40	'w'
   0x12,		//	41	'e'
   0x13,		//	42	'r'
   0x14,		//	43	't'
   0x15,		//	44	'y'
   0x16,		//	45	'u'
   0x17,		//	46	'i'
   0x18,		//	47	'o'
   0x19,		//	48	'p'
   0x1a,		//	49	'['
   0x1b,		//	50	']'
   0x35,		//	51	'\'
   0x5300,		//	52	Delete
   0x4f00,		//	53	End
   0x5100,		//	54	Page Down
   0x47,		//	55	Pad '7'
   0x48,		//	56	Pad '8'
   0x49,		// 	57	Pad '9'
   0x4e,		//	58	Pad '+'
   0x3a,		//	59	Caps Lock
   0x1e,		//	60	'a'
   0x1f,		//	61	's'
   0x20,		//	62	'd'
   0x21,		//	63	'f'
   0x22,		//	64	'g'
   0x23,		//	65	'h'
   0x24,		//	66	'j'
   0x25,		//	67	'k'
   0x26,		//	68	'l'
   0x27,		//	69	';'
   0x28,		//	70	'''
   0x1c,		//	71	Enter
   0x4b,		//	72	Pad '4'
   0x4c,		//	73	Pad '5'
   0x4d,		//	74	Pad '6'
   0x2a,		//	75	Left Shift
   0x2c,		//	76	'z'
   0x2d,		//	77	'x'
   0x2e,		//	78	'c'
   0x2f,		//	79	'v'
   0x30,		//	80	'b'
   0x31,		//	81	'n'
   0x32,		//	82	'm'
   0x33,		//	83	','
   0x34,		//	84	'.'
   0x35,		//	85	'/'
   0x36,		//	86	Right Shift
   0x4800,		//	87	Up
   0x4f,		//	88	Pad '1'
   0x50,		//	89	Pad '2'
   0x51,		//	90	Pad '3'
   0x1c00,		//	91	Pad Enter
   0x1d,		//	92	Left Control
   0x38,		//	93	Left Alt
   0x39,		//	94	Space
   0x3800,		//	95	Right Alt
   0x1d00,		//	96	Right Control
   0x4b00,		//	97	Left
   0x5000,		//	98	Down
   0x4d00,		//	99	Right
   0x52,		//	100	Pad '0'
   0x53,		//	101	Pad Del
   0x5b,		//	102	Left Windows Key
   0x5c,		//	103	Right Windows Key
   0x5d,		//	104	Menu
   -1,			//	105	(not used)
   -1,			//	106	(not used)
   -1,			//	107	(not used)
   -1,			//	108	(not used)
   -1,			//	109	(not used)
   -1,			//	110	(not used)
   -1,			//	111	(not used)
   -1,			//	112	(not used)
   -1,			//	113	(not used)
   -1,			//	114	(not used)
   -1,			//	115	(not used)
   -1,			//	116	(not used)
   -1,			//	117	(not used)
   -1,			//	118	(not used)
   -1,			//	119	(not used)
   -1,			//	120	(not used)
   -1,			//	121	(not used)
   -1,			//	122	(not used)
   -1,			//	123	(not used)
   -1,			//	124	(not used)
   -1,			//	125	(not used)
   -1,			//	126	(not used)
   -1			//	127	(not used)
};



/* keyboard_thread:
 */
static int32 keyboard_thread(void *keyboard_started)
{
   key_info  key_info_old;
   key_info  key_info_new;

   get_key_info(&key_info_old);
   if (key_info_old.modifiers & B_CAPS_LOCK)
      _key_shifts |= KB_CAPSLOCK_FLAG;
   if (key_info_old.modifiers & B_SCROLL_LOCK)
      _key_shifts |= KB_SCROLOCK_FLAG;
   if (key_info_old.modifiers & B_NUM_LOCK)
      _key_shifts |= KB_NUMLOCK_FLAG;

   release_sem(*(sem_id *)keyboard_started);

   while(keyboard_thread_running) {
      int i;

      snooze(KEY_THREAD_PERIOD);
      if (!_be_focus_count)
         continue;
      
      get_key_info(&key_info_new);

      if (three_finger_flag &&
          (_key_shifts & KB_CTRL_FLAG) &&
          (_key_shifts & KB_ALT_FLAG)  &&
          (key_info_new.key_states[52 / 8] & (52 % 8)) ) {
         _be_terminate(keyboard_thread_id, true);
      }

      for (i=0; i<128; i++) {
         int new_key_pressed;
         int old_key_pressed;
         int scancode;
         
         new_key_pressed = key_info_new.key_states[i >> 3] &
            (1 << (7 - (i % 8)));
         old_key_pressed = key_info_old.key_states[i >> 3] &
            (1 << (7 - (i % 8)));
         if (new_key_pressed != old_key_pressed) {
            scancode = be_to_pc[i];
            if (scancode != -1) {
               if (i == BE_KEY_PAUSE) {
                  /* pause key special case */
                  _handle_key_press(0, KEY_PAUSE);
                  _handle_key_release(KEY_PAUSE);
                  continue;
               }
               if ((i == BE_KEY_CAPSLOCK) ||
                   (i == BE_KEY_SCROLOCK) ||
                   (i == BE_KEY_NUMLOCK)) {
                  /* caps lock, scroll lock, and num lock special cases */
                  _handle_pckey(scancode);
                  _handle_pckey(scancode | 0x80);
                  continue;
               }
               if (scancode & 0xff00) {
                  /* extended code */
                  _handle_pckey(0xe0);
                  scancode >>= 8;
               }
               /* handle normal key press/release */
               if (!new_key_pressed)
                  scancode |= 0x80;
               _handle_pckey(scancode);
            }
         }
      }

      key_info_old = key_info_new;
   }

   /* XXX commented out due to conflicting TRACE in Haiku
   TRACE(PREFIX_I "keyboard thread exited\n");
   */

   return 0;
}



/* set_default_key_repeat:
 */
static inline bool set_default_key_repeat(void)
{
   int32     rate;
   bigtime_t delay;

   if (get_key_repeat_rate(&rate) != B_OK) {
      return false;
   }

   if (get_key_repeat_delay(&delay) != B_OK) {
      return false;
   }

   set_keyboard_rate((delay / 1000), (10000 / rate));

   return true;
}



/* be_key_init:
 */
extern "C" int be_key_init(void)
{
   sem_id keyboard_started;

   _pckeys_init();

   if (get_keyboard_id(&keyboard_id) == B_ERROR) {
      goto cleanup;
   }

   if (keyboard_id != KEY_ID_PC101) {
      goto cleanup;
   }

   waiting_for_input = create_sem(0, "waiting for input...");

   if (waiting_for_input < B_NO_ERROR) {
      goto cleanup;
   }

   keyboard_started = create_sem(0, "starting keyboard thread...");

   if (keyboard_started < B_NO_ERROR) {
      goto cleanup;
   }

   keyboard_thread_id = spawn_thread(keyboard_thread, KEY_THREAD_NAME,
                           KEY_THREAD_PRIORITY, &keyboard_started);

   if (keyboard_thread_id < 0) {
      goto cleanup;
   }

   keyboard_thread_running = TRUE;
   resume_thread(keyboard_thread_id);
   acquire_sem(keyboard_started);
   delete_sem(keyboard_started);

   if(!set_default_key_repeat()) {
     goto cleanup;
   }

   return 0;

   cleanup: {
      if (keyboard_started > 0) {
         delete_sem(keyboard_started);
      }

      be_key_exit();
      return 1;
   }
}



/* be_key_exit:
 */
extern "C" void be_key_exit(void)
{
   if (keyboard_thread_id > 0) {
      keyboard_thread_running = FALSE;
      wait_for_thread(keyboard_thread_id, &ignore_result);
      keyboard_thread_id = -1;
   }

   if (waiting_for_input > 0) {
      delete_sem(waiting_for_input);
      waiting_for_input = -1;
   }

   keyboard_id = (uint16)(-1);
}



/* be_key_set_leds:
 *  Sets keyboard leds.
 */
extern "C" void be_key_set_leds(int leds)
{
   uint32 modifiers;

   modifiers = 0;

   _key_shifts &= ~(KB_CAPSLOCK_FLAG | KB_SCROLOCK_FLAG | KB_NUMLOCK_FLAG);
   
   if (leds & KB_CAPSLOCK_FLAG) {
      modifiers |= B_CAPS_LOCK;
      _key_shifts |= KB_CAPSLOCK_FLAG;
   }

   if (leds & KB_SCROLOCK_FLAG) {
      modifiers |= B_SCROLL_LOCK;
      _key_shifts |= KB_SCROLOCK_FLAG;
   }

   if (leds & KB_NUMLOCK_FLAG) {
      modifiers |= B_NUM_LOCK;
      _key_shifts |= KB_NUMLOCK_FLAG;
   }

   set_keyboard_locks(modifiers);
}



/* be_key_set_rate:
 *  Sets keyboard repeat rate and delay.
 */
extern "C" void be_key_set_rate(int delay, int repeat)
{
   if (delay < 250)
      delay = 250;
   else if (delay < 500)
      delay = 500;
   else if (delay < 750)
      delay = 750;
   else
      delay = 1000;
   set_key_repeat_delay((bigtime_t)(delay * 1000));
   if (repeat > 0)
      set_key_repeat_rate(10000 / repeat);
}



/* be_key_wait_for_input:
 */
extern "C" void be_key_wait_for_input(void)
{
   acquire_sem(waiting_for_input);
}



/* be_key_stop_waiting_for_input:
 */
extern "C" void be_key_stop_waiting_for_input(void)
{
   release_sem(waiting_for_input);
}



extern "C" void be_key_suspend(void)
{
   suspend_thread(keyboard_thread_id);
}


extern "C" void be_key_resume(void)
{
   resume_thread(keyboard_thread_id);
}
