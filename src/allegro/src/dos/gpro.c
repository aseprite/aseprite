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
 *      Joystick driver for the Gravis GamePad Pro.
 *
 *      By Marius Fodor, using information provided by Benji York.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* driver functions */
static int gpro_init(void); 
static void gpro_exit(void); 
static int gpro_poll(void);



JOYSTICK_DRIVER joystick_gpro =
{
   JOY_TYPE_GAMEPAD_PRO,
   empty_string,
   empty_string,
   "GamePad Pro",
   gpro_init,
   gpro_exit,
   gpro_poll,
   NULL, NULL,
   NULL, NULL
};



/* int read_gpp:
 *  Reads the GamePad Pro pointed to by pad_num (0 or 1) and returns its 
 *  status. Returns 1 if the GPP returned corrupt data, or if no GPP was 
 *  detected.
 *
 *  This code was converted from asm into C by Shawn Hargreaves, who didn't
 *  really understand the original, so any errors are very probable :-)
 */
static int read_gpp(int pad_num)
{
   int samples[50];
   int clock_mask, data_mask, data, old_data;
   int num_samples, timeout1, timeout2, sample_pos, c;

   if (pad_num == 0) {
      clock_mask = 0x10;
      data_mask = 0x20;
   }
   else {
      clock_mask = 0x40;
      data_mask = 0x80;
   }

   num_samples = 0;
   timeout1 = 0;

   DISABLE();

   old_data = inportb(0x201);
   data = 0;

   while (num_samples<50) {
      for (timeout2=0; timeout2<255; timeout2++) {
	 data = inportb(0x201);
	 if (data != old_data)
	    break;
      }

      if (timeout2 == 255) {
	 ENABLE();
	 return 1;
      }

      if ((old_data & clock_mask) && (!(data & clock_mask))) {
	 samples[num_samples] = (data & data_mask) ? 1 : 0;
	 num_samples++;
      }

      old_data = data;

      if (timeout1++ == 200)
	 break;
   }

   ENABLE();

   c = 0;

   for (sample_pos=1; sample_pos<num_samples; sample_pos++) {
      if (samples[sample_pos])
	 c++;
      else
	 c = 0;

      if (c == 5)
	 break;
   };

   if (c != 5)
      return 1;

   sample_pos++;
   data = 0;

   for (c=0; c<14; c++) {
      if ((c&3) == 0)
	 sample_pos++;
      data |= samples[sample_pos];
      data <<= 1;
      sample_pos++;
   }

   return data;
}



/* gpro_init:
 *  Initialises the driver.
 */
static int gpro_init(void)
{
   int i;

   if (read_gpp(0) == 1)
      return -1;

   if (read_gpp(1) == 1)
      num_joysticks = 1;
   else
      num_joysticks = 2;

   for (i=0; i<num_joysticks; i++) {
      joy[i].flags = JOYFLAG_DIGITAL;

      joy[i].num_sticks = 1;
      joy[i].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      joy[i].stick[0].num_axis = 2;
      joy[i].stick[0].axis[0].name = get_config_text("X");
      joy[i].stick[0].axis[1].name = get_config_text("Y");
      joy[i].stick[0].name = get_config_text("Pad");

      joy[i].num_buttons = 10;

      joy[i].button[0].name = get_config_text("Red");
      joy[i].button[1].name = get_config_text("Yellow");
      joy[i].button[2].name = get_config_text("Green");
      joy[i].button[3].name = get_config_text("Blue");
      joy[i].button[4].name = get_config_text("L1");
      joy[i].button[5].name = get_config_text("R1");
      joy[i].button[6].name = get_config_text("L2");
      joy[i].button[7].name = get_config_text("R2");
      joy[i].button[8].name = get_config_text("Select");
      joy[i].button[9].name = get_config_text("Start");
   }

   return 0;
}



/* gpro_exit:
 *  Shuts down the driver.
 */
static void gpro_exit(void)
{
}



/* gpro_poll:
 *  Updates the joystick status variables.
 */
static int gpro_poll(void)
{
   int i, b, gpp, mask;

   for (i=0; i<num_joysticks; i++) {
      gpp = read_gpp(i);

      if (gpp == 1)
	 return -1;

      joy[i].stick[0].axis[0].d1 = ((gpp & 0x02) != 0);
      joy[i].stick[0].axis[0].d2 = ((gpp & 0x04) != 0);
      joy[i].stick[0].axis[1].d1 = ((gpp & 0x10) != 0);
      joy[i].stick[0].axis[1].d2 = ((gpp & 0x08) != 0);

      for (b=0; b<2; b++) {
	 if (joy[i].stick[0].axis[b].d1)
	    joy[i].stick[0].axis[b].pos = -128;
	 else if (joy[i].stick[0].axis[b].d2)
	    joy[i].stick[0].axis[b].pos = 128;
	 else
	    joy[i].stick[0].axis[b].pos = 0;
      }

      mask = 0x20;

      for (b=0; b<10; b++) {
	 joy[i].button[b].b = ((gpp & mask) != 0);
	 mask <<= 1;
      }
   }

   return 0;
}


