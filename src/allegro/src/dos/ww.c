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
 *      Joystick driver for the Wingman Warrior.
 *
 *      By Kester Maddock
 *
 *      TODO: Support all SWIFT devices.  x and y values are correct,
 *      my Wingman always has these set to 0, joystick values in
 *      pitch and roll.  Joystick values range from -8192 to 8192, I
 *      >> by 6 to get -128 to 128 Allegro range.  Spinner's range is
 *      +/- 18?  I would recommend setting x/y values to hat switch's
 *      axis, and reading the hat switch as buttons instead of an axis.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* driver functions */
static int ww_init(void);
static void ww_exit(void);
static int ww_poll(void);



typedef union SWIFT_T
{
   struct {
      unsigned char type;
   } swift;
   struct {
      short x;                /* unused */
      short y;                /* unused */
      short z;                /* throttle */
      short pitch;            /* stick y axis */
      short roll;             /* stick x axis */
      short yaw;              /* spinner */
      short buttons;
   } data;
} SWIFT_T;


static SWIFT_T *wingman = NULL;



JOYSTICK_DRIVER joystick_ww =
{
   JOY_TYPE_WINGWARRIOR,
   empty_string,
   empty_string,
   "Wingman Warrior",
   ww_init,
   ww_exit,
   ww_poll,
   NULL, NULL,
   NULL, NULL
};



/* ww_init:
 *  Initialises the driver.
 */
static int ww_init(void)
{
   __dpmi_regs r;

   wingman = _AL_MALLOC(sizeof(SWIFT_T));
   memset(wingman, 0, sizeof(SWIFT_T));

   memset(&r, 0, sizeof(r));
   r.d.eax = 0x53C1;
   r.x.es = __tb >> 4;
   r.d.edx = 0;
   __dpmi_int(0x33, &r);

   dosmemget(__tb, sizeof(SWIFT_T), wingman);

   if (r.x.ax != 1) {
      /* SWIFT functions not present */
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("SWIFT Device not detected"));
      _AL_FREE(wingman);
      wingman = NULL;
      return -1;
   }

   if (wingman->swift.type != 1) {
      /* no SWIFT device, or not Wingman */
      if (wingman->swift.type == 0)
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Wingman Warrior not connected"));
      else
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Device connected is not a Wingman Warrior"));

      _AL_FREE(wingman);
      wingman = NULL;
      return -1;
   }

   num_joysticks = 1;
   joy[0].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
   joy[0].num_sticks = 4;
   joy[0].num_buttons = 4;

   joy[0].stick[0].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
   joy[0].stick[0].num_axis = 2;
   joy[0].stick[0].name = get_config_text("Stick");
   joy[0].stick[0].axis[0].name = get_config_text("X");
   joy[0].stick[0].axis[1].name = get_config_text("Y");

   joy[0].stick[1].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
   joy[0].stick[1].num_axis = 1;
   joy[0].stick[1].name = joy[0].stick[1].axis[0].name = get_config_text("Spinner");

   joy[0].stick[2].flags = JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
   joy[0].stick[2].num_axis = 1;
   joy[0].stick[2].name = joy[0].stick[2].axis[0].name = get_config_text("Throttle");

   joy[0].stick[3].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
   joy[0].stick[3].num_axis = 2;
   joy[0].stick[3].name = get_config_text("Hat");
   joy[0].stick[3].axis[0].name = get_config_text("X");
   joy[0].stick[3].axis[1].name = get_config_text("Y");

   joy[0].button[0].name = get_config_text("B1");
   joy[0].button[1].name = get_config_text("B2");
   joy[0].button[2].name = get_config_text("B3");
   joy[0].button[3].name = get_config_text("B4");

   return 0;
}



/* ww_poll:
 *  Polls the joystick state.
 */
static int ww_poll(void)
{
   __dpmi_regs r;

   /* no wingman for you */
   if (!wingman)
      return -1;

   /* read from the mouse driver */
   memset(&r, 0, sizeof(r));
   r.d.eax = 0x5301;
   r.x.es = __tb >> 4;
   r.d.edx = 0;
   __dpmi_int(0x33, &r);
   dosmemget(__tb, sizeof(SWIFT_T), wingman);

   /* main X/Y axis */
   joy[0].stick[0].axis[0].pos = -wingman->data.roll >> 6;
   joy[0].stick[0].axis[1].pos = wingman->data.pitch >> 6;

   /* spin control */
   joy[0].stick[1].axis[0].pos = -wingman->data.yaw * 7;

   /* throttle */
   joy[0].stick[2].axis[0].pos = (wingman->data.z >> 6) + 128;

   /* setup digital controls */
   joy[0].stick[0].axis[0].d1 = joy[0].stick[0].axis[0].pos < -64;
   joy[0].stick[0].axis[0].d2 = joy[0].stick[0].axis[0].pos > 64;
   joy[0].stick[0].axis[1].d1 = joy[0].stick[0].axis[1].pos < -64;
   joy[0].stick[0].axis[1].d2 = joy[0].stick[0].axis[1].pos > 64;

   joy[0].stick[1].axis[0].d1 = joy[0].stick[1].axis[0].pos < -64;
   joy[0].stick[1].axis[0].d2 = joy[0].stick[1].axis[0].pos > 64;

   joy[0].stick[2].axis[0].d2 = joy[0].stick[2].axis[0].pos > 255*2/3;

   /* setup buttons */
   joy[0].button[0].b = wingman->data.buttons & 0x4;
   joy[0].button[1].b = wingman->data.buttons & 0x1;
   joy[0].button[2].b = wingman->data.buttons & 0x2;
   joy[0].button[3].b = wingman->data.buttons & 0x10;

   /* hat switch */
   joy[0].stick[0].axis[0].d1 = wingman->data.buttons & 0x020;
   joy[0].stick[0].axis[0].d2 = wingman->data.buttons & 0x040;
   joy[0].stick[0].axis[1].d1 = wingman->data.buttons & 0x080;
   joy[0].stick[0].axis[1].d2 = wingman->data.buttons & 0x100;

   joy[0].stick[0].axis[0].pos =  128*joy[0].stick[0].axis[0].d2 - 
				  128*joy[0].stick[0].axis[0].d1;

   joy[0].stick[0].axis[1].pos =  128*joy[0].stick[0].axis[1].d2 -
				  128*joy[0].stick[0].axis[1].d1;

   return 0;
}



/* ww_exit:
 *  Shuts down the driver.
 */
static void ww_exit(void)
{
   if (wingman) {
      _AL_FREE(wingman);
      wingman = NULL;
   }
}



