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
 *      Replacement for the Allegro program main() function to execute
 *      the PSP stuff: call the PSP_* macros and create the standard
 *      exit callback.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#include <pspkernel.h>


extern void *_mangled_main_address;



/* Define the module info section */
PSP_MODULE_INFO("Allegro Application", PSP_MODULE_USER, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(270);
PSP_HEAP_SIZE_KB(-256);



/* exit_callback:
 *
 */
static int exit_callback(int arg1, int arg2, void *common)
{
   sceKernelExitGame();
   return 0;
}



/* callback_thread:
 *  Registers the exit callback.
 */
static int callback_thread(SceSize args, void *argp)
{
   int cbid = sceKernelCreateCallback("Exit Callback", (void *) exit_callback, NULL);
   sceKernelRegisterExitCallback(cbid);
   sceKernelSleepThreadCB();
   return 0;
}



/* setup_callback:
 *  Sets up the callback thread and returns its thread id.
 */
static int setup_callback(void)
{
   int thid = 0;
   thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, 0);
   if (thid >= 0) {
      sceKernelStartThread(thid, 0, 0);
   }
   return thid;
}



/* main:
 *  Replacement for main function.
 */
int main(int argc, char *argv[])
{
   int (*real_main) (int, char *[]) = (int (*) (int, char *[])) _mangled_main_address;

   setup_callback();
   return (*real_main)(argc, argv);
}

