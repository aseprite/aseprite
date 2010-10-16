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
 *      Joystick driver for the IF-SEGA/ISA (I-O DATA) controller.
 *
 *      By E. Watanabe (99/09/08),
 *      based on sample code by Earle F. Philhower, III / Kerry High (SNES).
 *
 */


#include "allegro.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



#define IFSEGAISA_BASE 0x1001


/* driver functions */
static int sg_init(void);
static void sg_exit(void);
static int sg_poll(int);
static int sg1_poll(void);

static int sg_poll_sub(int num, int base);
static int sg_poll_sub2(int base);
static int sg_poll_sub3(int num, int count, int base);
static int pad_kind(int base);
static void sg_button_init(int num);



JOYSTICK_DRIVER joystick_sg1 =
{
   JOY_TYPE_IFSEGA_ISA,
   NULL,
   NULL,
   "IF-SEGA/ISA",
   sg_init,
   sg_exit,
   sg1_poll,
   NULL, NULL,
   NULL, NULL
};



/* sg_init:
 *  Initialises the driver.
 */
static int sg_init(void)
{
   static char *name_b[] = { "A", "B", "C", "X", "Y", "Z", "L", "R", "Start" };
   int i, j;

   num_joysticks = 4;

   for (i=0; i<num_joysticks; i++) {
      joy[i].flags = JOYFLAG_ANALOGUE;
      joy[i].num_sticks = 2;

      for (j=0; j<2; j++) {
	 joy[i].stick[j].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	 joy[i].stick[j].num_axis = 2;
	 joy[i].stick[j].axis[0].name = get_config_text("X");
	 joy[i].stick[j].axis[1].name = get_config_text("Y");
	 joy[i].stick[j].name = get_config_text("Pad");
      }

      joy[i].num_buttons = 9;

      for (j=0; j<9; j++)
	 joy[i].button[j].name = get_config_text(name_b[j]);
   }

   return 0;
}



/* sg_exit:
 *  Shuts down the driver.
 */
static void sg_exit(void)
{
}



/* sg_poll:
 *  Common - Updates the joystick status variables.
 */
static int sg_poll(int base)
{
   int num = 0;

   num = sg_poll_sub(num, base);             /* IFSEGA 1P */

   if (num < num_joysticks)
      num = sg_poll_sub(num, base + 4);      /* IFSEGA 2P */

   while (num < num_joysticks) {
      sg_button_init(num);
      num++;
   }

   return 0;
}



static int sg_poll_sub(int num, int base)
{
   int i;
   int j;
   int a;
   int b;
   int num1;

   switch (pad_kind(base)) {

      case 0x05:

	 switch (sg_poll_sub2(base)) {

	    case 0x00:          /* Multi Controller , Racing Controller , etc.. */

	    case 0x01:
	       sg_poll_sub3(num, sg_poll_sub2(base) * 2, base);
	       num++;
	       break;

	    case 0x04:          /* Multi-Terminal 6 */
	       sg_poll_sub2(base);
	       sg_poll_sub2(base);
	       sg_poll_sub2(base);
	       num1 = num;
	       i = 6 + 1;
	       while (num < num_joysticks) {
		  sg_button_init(num);
		  if (!(--i))
		     break;

		  a = sg_poll_sub2(base);
		  b = sg_poll_sub2(base) * 2;
		  if (a == 0x0f)
		     continue;  /* no pad */

		  if ((a == 0x0e) && (b == 6)) {
		     sg_poll_sub2(base);        /* Shuttle Mouse */

		     sg_poll_sub2(base);
		     sg_poll_sub2(base);
		     sg_poll_sub2(base);
		     sg_poll_sub2(base);
		     sg_poll_sub2(base);
		     continue;
		  }
		  sg_poll_sub3(num, b, base);
		  num++;
	       }
	       if (num == num1)
		  num++;
	       break;

	    default:
	       sg_button_init(num);
	 }
	 break;

      case 0x0b:                /* Normal Pad , Twin Stick , etc.. */

	 outportb(base, 0x1f);
	 outportb(base + 2, 0xe0);
	 a = inportb(base + 2); /* L 1 0 0 */

	 joy[num].button[6].b = !(a & 0x08);
	 outportb(base + 2, 0x80);
	 a = inportb(base + 2); /* R X Y Z */

	 joy[num].button[7].b = !(a & 0x08);
	 joy[num].button[3].b = !(a & 0x04);
	 joy[num].button[4].b = !(a & 0x02);
	 joy[num].button[5].b = !(a & 0x01);
	 outportb(base + 2, 0xc0);      /* Start A C B */

	 a = inportb(base + 2);
	 joy[num].button[8].b = !(a & 0x08);
	 joy[num].button[0].b = !(a & 0x04);
	 joy[num].button[2].b = !(a & 0x02);
	 joy[num].button[1].b = !(a & 0x01);
	 outportb(base + 2, 0xa0);
	 a = inportb(base + 2); /* axis R L D U */

	 joy[num].stick[0].axis[0].d1 = !(a & 0x04);
	 joy[num].stick[0].axis[0].d2 = !(a & 0x08);
	 joy[num].stick[0].axis[1].d1 = !(a & 0x01);
	 joy[num].stick[0].axis[1].d2 = !(a & 0x02);

	 for (j = 0; j <= 1; j++) {
	    if (joy[num].stick[0].axis[j].d1)
	       joy[num].stick[0].axis[j].pos = -128;
	    else {
	       if (joy[num].stick[0].axis[j].d2)
		  joy[num].stick[0].axis[j].pos = 128;
	       else
		  joy[num].stick[0].axis[j].pos = 0;
	    }
	 }

	 joy[num].stick[1].axis[0].d1 = 0;
	 joy[num].stick[1].axis[0].d2 = 0;
	 joy[num].stick[1].axis[1].d1 = 0;
	 joy[num].stick[1].axis[1].d2 = 0;
	 joy[num].stick[1].axis[0].pos = 0;
	 joy[num].stick[1].axis[1].pos = 0;

	 num++;
	 break;

      case 0x03:                /* Shuttle Mouse */

      default:
	 sg_button_init(num);
	 num++;
   }

   outportb(base + 2, 0x60);
   return num;
}



static int sg_poll_sub2(int base)
{
   int i = 0x100;
   int a = 0;
   int b = 0;

   a = inportb(base + 2);
   outportb(base + 2, a ^ 0x20);
   while (i--) {
      b = inportb(base + 2);
      if ((a & 0x10) != (b & 0x10))
	 break;
   }

   return (b & 0x0f);
}



static int sg_poll_sub3(int num, int count, int base)
{
   int i;
   int a;

   a = sg_poll_sub2(base);      /* axis R L D U (digital) */

   joy[num].stick[0].axis[0].d1 = !(a & 0x04);
   joy[num].stick[0].axis[0].d2 = !(a & 0x08);
   joy[num].stick[0].axis[1].d1 = !(a & 0x01);
   joy[num].stick[0].axis[1].d2 = !(a & 0x02);
   a = sg_poll_sub2(base);      /* Start A C B */

   joy[num].button[8].b = !(a & 0x08);
   joy[num].button[0].b = !(a & 0x04);
   joy[num].button[2].b = !(a & 0x02);
   joy[num].button[1].b = !(a & 0x01);
   a = sg_poll_sub2(base);      /* R X T Z */

   joy[num].button[7].b = !(a & 0x08);
   joy[num].button[3].b = !(a & 0x04);
   joy[num].button[4].b = !(a & 0x02);
   joy[num].button[5].b = !(a & 0x01);
   a = sg_poll_sub2(base);      /* L */

   joy[num].button[6].b = !(a & 0x08);

   joy[num].stick[1].axis[0].d1 = 0;
   joy[num].stick[1].axis[0].d2 = 0;
   joy[num].stick[1].axis[1].d1 = 0;
   joy[num].stick[1].axis[1].d2 = 0;
   joy[num].stick[1].axis[0].pos = 0;
   joy[num].stick[1].axis[1].pos = 0;

   if (count -= 4)
      joy[num].stick[0].axis[0].pos = ((sg_poll_sub2(base) << 4) | sg_poll_sub2(base)) - 128;
   else {
      for (i = 0; i <= 1; i++) {
	 if (joy[num].stick[0].axis[i].d1)
	    joy[num].stick[0].axis[i].pos = -128;
	 else {
	    if (joy[num].stick[0].axis[i].d2)
	       joy[num].stick[0].axis[i].pos = 128;
	    else
	       joy[num].stick[0].axis[i].pos = 0;
	 }
      }
      return 0;
   }
   if (count -= 2)
      joy[num].stick[0].axis[1].pos = ((sg_poll_sub2(base) << 4) | sg_poll_sub2(base)) - 128;
   else {
      if (joy[num].stick[0].axis[1].d1)
	 joy[num].stick[0].axis[1].pos = -128;
      else {
	 if (joy[num].stick[0].axis[1].d2)
	    joy[num].stick[0].axis[1].pos = 128;
	 else
	    joy[num].stick[0].axis[1].pos = 0;
      }
      return 0;
   }
   if (count -= 2)
      joy[num].stick[1].axis[0].pos = ((sg_poll_sub2(base) << 4) | sg_poll_sub2(base)) - 128;
   else
      return 0;
   if (count -= 2)
      joy[num].stick[1].axis[1].pos = ((sg_poll_sub2(base) << 4) | sg_poll_sub2(base)) - 128;

   return 0;
}



static int pad_kind(int base)
{
   int i = 0x40;
   int a = 0;
   int result = 0;

   outportb(base, 0x1f);

   while (i--) {
      outportb(base+2, 0x60);
      a = inportb(base+2);
      if ((a & 0x07) == 0x01 || (a & 0x07) == 0x04)
	 break;
   }

   if (a & 0x0c)
      result |= 0x08;
   if (a & 0x03)
      result |= 0x04;
   outportb(base + 2, 0x20);
   a = inportb(base + 2) & 0x0f;
   if (a & 0x0c)
      result |= 0x02;
   if (a & 0x03)
      result |= 0x01;

   return result;
}



static void sg_button_init(int num)
{
   int i;

   for (i=0; i<9; i++) {
      joy[num].button[i].b = 0;
   }

   for (i=0; i<=1; i++) {
      joy[num].stick[i].axis[0].d1 = 0;
      joy[num].stick[i].axis[0].d2 = 0;
      joy[num].stick[i].axis[1].d1 = 0;
      joy[num].stick[i].axis[1].d2 = 0;
      joy[num].stick[i].axis[0].pos = 0;
      joy[num].stick[i].axis[1].pos = 0;
   }
}



/* sg1_poll:
 *  1P - Updates the joystick status variables.
 */
static int sg1_poll(void)
{
   return sg_poll(IFSEGAISA_BASE);
}
