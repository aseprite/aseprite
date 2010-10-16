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
 *      QNX keyboard driver.
 *
 *      By Angelo Mottola.
 *
 *      Based on Unix/X11 version by Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintqnx.h"

#ifndef ALLEGRO_QNX
#error Something is wrong with the makefile
#endif


static int qnx_keyboard_init(void);
static void qnx_keyboard_exit(void);


KEYBOARD_DRIVER keyboard_qnx =
{
   KEYBOARD_QNX,
   empty_string,
   empty_string,
   "QNX keyboard",
   TRUE,
   qnx_keyboard_init,
   qnx_keyboard_exit,
   NULL,   // AL_METHOD(void, poll, (void));
   NULL,   // AL_METHOD(void, set_leds, (int leds));
   NULL,   // AL_METHOD(void, set_rate, (int delay, int rate));
   NULL,   // AL_METHOD(void, wait_for_input, (void));
   NULL,   // AL_METHOD(void, stop_waiting_for_input, (void));
   _pckey_scancode_to_ascii,
   _pckey_scancode_to_name
};


static int main_pid;



/* qnx_keyboard_handler:
 *  Keyboard "interrupt" handler.
 */
void qnx_keyboard_handler(int pressed, int code)
{
   int scancode;

   /* Handle scancode.  */
   scancode = (code & 0x7F) | (pressed ? 0x00 : 0x80);
   _handle_pckey(scancode);
   
   /* Exit by Ctrl-Alt-End.  */
   if (((scancode == 0x4F) || (scancode == 0x53)) && three_finger_flag
       && (_key_shifts & KB_CTRL_FLAG) && (_key_shifts & KB_ALT_FLAG))
      kill(main_pid, SIGTERM);
}



/* qnx_keyboard_focused:
 *  Keyboard focus change handler.
 */
void qnx_keyboard_focused(int focused, int state)
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



/* qnx_keyboard_init:
 *  Installs the keyboard handler.
 */
static int qnx_keyboard_init(void)
{
   /* get the pid, which we use for the three finger salute */
   main_pid = getpid();

   _pckeys_init();
   
   return 0;
}



/* qnx_keyboard_exit:
 *  Removes the keyboard handler.
 */
static void qnx_keyboard_exit(void)
{
   qnx_keyboard_focused(FALSE, 0);
}
