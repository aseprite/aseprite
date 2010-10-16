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
 *      Joystick driver routines for PSP.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintpsp.h"
#include <pspctrl.h>

#ifndef ALLEGRO_PSP
#error something is wrong with the makefile
#endif

#define PREFIX_I                "al-pjoy INFO: "
#define PREFIX_W                "al-pjoy WARNING: "
#define PREFIX_E                "al-pjoy ERROR: "


/* The PSP controller no directional buttons. */
struct _PSP_BUTTON
{
   char *name;
   enum PspCtrlButtons code;
} psp_controller_buttons[] = {
   {"TRIANGLE", PSP_CTRL_TRIANGLE},
   {"CIRCLE",   PSP_CTRL_CIRCLE},
   {"CROSS",    PSP_CTRL_CROSS},
   {"SQUARE",   PSP_CTRL_SQUARE},
   {"LTRIGGER", PSP_CTRL_LTRIGGER},
   {"RTRIGGER", PSP_CTRL_RTRIGGER},
   {"SELECT",   PSP_CTRL_SELECT},
   {"START",    PSP_CTRL_START}
};

#define NBUTTONS (sizeof psp_controller_buttons / sizeof psp_controller_buttons[0])


static int psp_joy_init(void);
static void psp_joy_exit(void);
static int psp_joy_poll(void);


JOYSTICK_DRIVER joystick_psp = {
   JOYSTICK_PSP,         // int  id;
   empty_string,         // AL_CONST char *name;
   empty_string,         // AL_CONST char *desc;
   "PSP Digital Pad",    // AL_CONST char *ascii_name;
   psp_joy_init,         // AL_METHOD(int, init, (void));
   psp_joy_exit,         // AL_METHOD(void, exit, (void));
   psp_joy_poll,         // AL_METHOD(int, poll, (void));
   NULL,                 // AL_METHOD(int, save_data, (void));
   NULL,                 // AL_METHOD(int, load_data, (void));
   NULL,                 // AL_METHOD(AL_CONST char *, calibrate_name, (int n));
   NULL                  // AL_METHOD(int, calibrate, (int n));
};



/* psp_joy_init:
 *  Initializes the PSP joystick driver.
 */
static int psp_joy_init(void)
{
   uint8_t b;

   _psp_init_controller(SAMPLING_CYCLE, SAMPLING_MODE);

   num_joysticks = 1;

   for (b=0; b<NBUTTONS; b++) {
      joy[0].button[b].name = psp_controller_buttons[b].name;
      joy[0].num_buttons++;
   }

   joy[0].num_sticks = 1;
   joy[0].stick[0].flags = JOYFLAG_DIGITAL;
   joy[0].stick[0].num_axis = 2;
   joy[0].stick[0].axis[0].name = "X axis";
   joy[0].stick[0].axis[1].name = "Y axis";
   joy[0].stick[0].name = "PSP digital pad";

   TRACE(PREFIX_I "PSP digital pad installed\n");

   return 0;
}



/* psp_joy_exit:
 *  Shuts down the PSP joystick driver.
 */
static void psp_joy_exit(void)
{
}



/* psp_joy_poll:
 *  Polls the active joystick devices and updates internal states.
 */
static int psp_joy_poll(void)
{
   SceCtrlData pad;
   int buffers_to_read = 1;
   uint8_t b;

   sceCtrlPeekBufferPositive(&pad, buffers_to_read);

   /* Report the status of the no directional buttons. */
   for (b=0; b<NBUTTONS; b++)
      joy[0].button[b].b = pad.Buttons & psp_controller_buttons[b].code;

   /* Report the status of the directional buttons. */
   joy[0].stick[0].axis[0].d1 = pad.Buttons & PSP_CTRL_LEFT;
   joy[0].stick[0].axis[0].d2 = pad.Buttons & PSP_CTRL_RIGHT;
   joy[0].stick[0].axis[1].d1 = pad.Buttons & PSP_CTRL_UP;
   joy[0].stick[0].axis[1].d2 = pad.Buttons & PSP_CTRL_DOWN;

   return 0;
}



/* _psp_init_controller:
 *  Internal routine to initialize the PSP controller.
 */
void _psp_init_controller(int cycle, int mode)
{
   sceCtrlSetSamplingCycle(cycle);
   sceCtrlSetSamplingMode(mode);
}
