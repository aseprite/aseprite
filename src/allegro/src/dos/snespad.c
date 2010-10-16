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
 *      Joystick driver for the SNES controller.
 *
 *      By Kerry High, based on sample code by Earle F. Philhower, III.
 *
 *      Mucked about with until it works better by Paul Hampson.
 *
 *      Lower CPU usage by Paul Hampson, based on the Linux implementation
 *      by Vojtech Pavlik.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



#define LPT1_BASE 0x378
#define LPT2_BASE 0x278
#define LPT3_BASE 0x3bc
#define SNES_POWER  248
#define SNES_CLOCK    1
#define SNES_LATCH    2
#define SNES_BUTTONS 12  /* actually 16, but the last 4 aren't
                          * connected in a real SNESpad
                          */

/* driver functions */
static int sp_init(void);
static void sp_exit(void);
static int sp_poll(int);
static int sp1_poll(void);
static int sp2_poll(void);
static int sp3_poll(void);


JOYSTICK_DRIVER joystick_sp1 =
{
   JOY_TYPE_SNESPAD_LPT1,
   empty_string,
   empty_string,
   "SNESpad-LPT1",
   sp_init,
   sp_exit,
   sp1_poll,
   NULL, NULL,
   NULL, NULL
};


JOYSTICK_DRIVER joystick_sp2 =
{
   JOY_TYPE_SNESPAD_LPT2,
   empty_string,
   empty_string,
   "SNESpad-LPT2",
   sp_init,
   sp_exit,
   sp2_poll,
   NULL, NULL,
   NULL, NULL
};


JOYSTICK_DRIVER joystick_sp3 =
{
   JOY_TYPE_SNESPAD_LPT3,
   empty_string,
   empty_string,
   "SNESpad-LPT3",
   sp_init,
   sp_exit,
   sp3_poll,
   NULL, NULL,
   NULL, NULL
};



/* sp_init:
 *  Initialises the driver.
 */
static int sp_init(void)
{
   int i;

   /* can't autodetect this... */
   num_joysticks = 5;

   for (i=0; i<num_joysticks; i++) {
      joy[i].flags = JOYFLAG_DIGITAL;

      joy[i].num_sticks = 1;
      joy[i].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      joy[i].stick[0].num_axis = 2;
      joy[i].stick[0].axis[0].name = get_config_text("X");
      joy[i].stick[0].axis[1].name = get_config_text("Y");
      joy[i].stick[0].name = get_config_text("Pad");

      joy[i].num_buttons = 8;

      joy[i].button[0].name = get_config_text("B");
      joy[i].button[1].name = get_config_text("Y");
      joy[i].button[2].name = get_config_text("A");
      joy[i].button[3].name = get_config_text("X");
      joy[i].button[4].name = get_config_text("Select");
      joy[i].button[5].name = get_config_text("Start");
      joy[i].button[6].name = get_config_text("L");
      joy[i].button[7].name = get_config_text("R");
   }

   return 0;
}



/* sp_exit:
 *  Shuts down the driver.
 */
static void sp_exit(void)
{
}



/* sp_poll:
 *  Common - Updates the joystick status variables.
 */
static int sp_poll(int base)
{
   static const int snes_in[] = {0x40, 0x20, 0x10, 0x08, 0x80};
   int buttondata[SNES_BUTTONS];
   int i, b;

   /* read bits from all the SNESpads in parallel */
   for (i = 0; i < SNES_BUTTONS; i++) {
      outportb(base, SNES_POWER);

      if (i == 0)
         outportb(base, SNES_POWER + SNES_LATCH + SNES_CLOCK);
      else
         outportb(base, SNES_POWER + SNES_CLOCK);

      outportb(base, SNES_POWER);

      /* last LPT line (0x80) is inverted compared to the others */
      buttondata[i] = (inportb(base + 1) ^ 0x80);
   }

   outportb(base, 0);

   for (i=0; i<num_joysticks; i++) {

      /* get b button */
      joy[i].button[0].b = ((buttondata[0] & snes_in[i]) == 0);
      /* get y button */
      joy[i].button[1].b = ((buttondata[1] & snes_in[i]) == 0);
      /* get select button */
      joy[i].button[4].b = ((buttondata[2] & snes_in[i]) == 0);
      /* get start button */
      joy[i].button[5].b = ((buttondata[3] & snes_in[i]) == 0);

      /* now, do the direction */
      joy[i].stick[0].axis[1].d1 = ((buttondata[4] & snes_in[i]) == 0);
      joy[i].stick[0].axis[1].d2 = ((buttondata[5] & snes_in[i]) == 0);
      joy[i].stick[0].axis[0].d1 = ((buttondata[6] & snes_in[i]) == 0);
      joy[i].stick[0].axis[0].d2 = ((buttondata[7] & snes_in[i]) == 0);

      for (b=0; b<2; b++) {
	 if (joy[i].stick[0].axis[b].d1)
	    joy[i].stick[0].axis[b].pos = -128;
	 else if (joy[i].stick[0].axis[b].d2)
	    joy[i].stick[0].axis[b].pos = 128;
	 else
	    joy[i].stick[0].axis[b].pos = 0;
      }

      /* get a button */
      joy[i].button[2].b = ((buttondata[8] & snes_in[i]) == 0);
      /* get x button */
      joy[i].button[3].b = ((buttondata[9] & snes_in[i]) == 0);
      /* get l button */
      joy[i].button[6].b = ((buttondata[10] & snes_in[i]) == 0);
      /* get r button */
      joy[i].button[7].b = ((buttondata[11] & snes_in[i]) == 0);

   }

   return 0;
}



/* sp1_poll:
 *  LPT1 - Updates the joystick status variables.
 */
static int sp1_poll(void)
{
   return sp_poll(LPT1_BASE);
}



/* sp2_poll:
 *  LPT2 - Updates the joystick status variables.
 */
static int sp2_poll(void)
{
   return sp_poll(LPT2_BASE);
}



/* sp3_poll:
 *  LPT3 - Updates the joystick status variables.
 */
static int sp3_poll(void)
{
   return sp_poll(LPT3_BASE);
}
