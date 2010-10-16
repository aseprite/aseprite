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
 *      Joystick driver for N64 controllers.
 *
 *      By Richard Davies, based on sample code by Earle F. Philhower, III.
 *
 *      This driver supports upto four N64 controllers. The analog stick
 *      calibrates itself when the controller is powered up (in hardware).
 *      There is some autodetection code included, but it's unused as it's
 *      unreliable. Care to take a look?
 *
 *      This driver is for the N64 pad -> parallel port interface developed
 *      by Stephan Hans and Simon Nield, supported by DirectPad Pro 4.9.
 *
 *      See http://www.st-hans.de/N64.htm for interface information, and
 *      See http://www.ziplabel.com for DirectPad Pro information.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



#define LPT1_BASE    0x378
#define LPT2_BASE    0x278
#define LPT3_BASE    0x3bC


#define D0           0x01
#define D1           0x02
#define D2           0x04
#define D3           0x08
#define D4           0x10
#define ACK          0x40
#define N64_HI       0xFF



/* driver functions */
static int n64_init(void);
static void n64p1_exit(void); 
static void n64p2_exit(void); 
static void n64p3_exit(void); 
static int n64p1_poll(void);
static int n64p2_poll(void);
static int n64p3_poll(void);
static int n64_detect(int base, int joynum);



JOYSTICK_DRIVER joystick_n641 =
{
   JOY_TYPE_N64PAD_LPT1,
   NULL,
   NULL,
   "N64pad-LPT1",
   n64_init,
   n64p1_exit,
   n64p1_poll,
   NULL, NULL,
   NULL, NULL
};



JOYSTICK_DRIVER joystick_n642 =
{
   JOY_TYPE_N64PAD_LPT2,
   NULL,
   NULL,
   "N64pad-LPT2",
   n64_init,
   n64p2_exit,
   n64p2_poll,
   NULL, NULL,
   NULL, NULL
};



JOYSTICK_DRIVER joystick_n643 =
{
   JOY_TYPE_N64PAD_LPT3,
   NULL,
   NULL,
   "N64pad-LPT3",
   n64_init,
   n64p3_exit,
   n64p3_poll,
   NULL, NULL,
   NULL, NULL
};



/* n64_init:
 *  Initialiases the N64 joypad driver.
 */
static int n64_init(void)
{
   int i;

   num_joysticks = 4;

   for (i=0; i<num_joysticks; i++) {
      joy[i].flags = JOYFLAG_ANALOG | JOYFLAG_SIGNED;

      joy[i].num_sticks = 2;

      joy[i].stick[0].flags = JOYFLAG_ANALOG | JOYFLAG_SIGNED;
      joy[i].stick[0].num_axis = 2;
      joy[i].stick[0].axis[0].name = get_config_text("Stick X");
      joy[i].stick[0].axis[1].name = get_config_text("Stick Y");
      joy[i].stick[0].name = get_config_text("Stick");

      joy[i].stick[1].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      joy[i].stick[1].num_axis = 2;
      joy[i].stick[1].axis[0].name = get_config_text("Direction Pad X");
      joy[i].stick[1].axis[1].name = get_config_text("Direction Pad Y");
      joy[i].stick[1].name = get_config_text("Direction Pad");

      joy[i].num_buttons = 10;

      joy[i].button[0].name = get_config_text("A");
      joy[i].button[1].name = get_config_text("B");
      joy[i].button[2].name = get_config_text("C Down");
      joy[i].button[3].name = get_config_text("C Left");
      joy[i].button[4].name = get_config_text("C Right");
      joy[i].button[5].name = get_config_text("C Up");
      joy[i].button[6].name = get_config_text("L");
      joy[i].button[7].name = get_config_text("R");
      joy[i].button[8].name = get_config_text("Z");
      joy[i].button[9].name = get_config_text("Start");
   }

   return 0;
}



/* n64p1_exit:
 *  Shuts down the driver.
 */
static void n64p1_exit(void)
{
   outportb(LPT1_BASE, 0x00);
}



/* n64p2_exit:
 *  Shuts down the driver.
 */
static void n64p2_exit(void)
{
   outportb(LPT2_BASE, 0x00);
}



/* n64p3_exit:
 *  Shuts down the driver.
 */
static void n64p3_exit(void)
{
   outportb(LPT3_BASE, 0x00);
}



/* n64_poll:
 *  Generic input polling function.
 */
static int n64_poll(int base, int joynum)
{
   static unsigned char joyid[4] = {~D0, ~D2, ~D3, ~D4};
   int i, b;
   int data[4];

   /* disable hardware interrupts, critical timing */
   DISABLE();

   outportb(base, N64_HI);

   /* first seven bits of command 0 */
   for (i=0; i<7; i++) {
      outportb(base, joyid[joynum]);   /* clock low for 3 usec */ 
      outportb(base, joyid[joynum]);
      outportb(base, joyid[joynum]);
      outportb(base, joyid[joynum]);
      outportb(base, joyid[joynum]);
      outportb(base, joyid[joynum]); 
      outportb(base, joyid[joynum]);

      outportb(base, N64_HI);          /* clock high for 1 usec */
      outportb(base, N64_HI);
   }

   /* last two bits of command 1 */
   for (i=7; i<9; i++) {
      outportb(base, joyid[joynum]);   /* clock low for 1 usec */
      outportb(base, joyid[joynum]);

      outportb(base, N64_HI);          /* clock high for 3 usec */
      outportb(base, N64_HI);
      outportb(base, N64_HI);
      outportb(base, N64_HI);
      outportb(base, N64_HI);
      outportb(base, N64_HI);
      outportb(base, N64_HI);
   }

   /* enable hardware interrupts */
   ENABLE();

   /* read shift register into data: */

   for (i=0; i<0x90; i++)
      outportb(base, N64_HI);          /* clock high, delay */

   for (i=0; i<4; i++)
      data[i] = 0;

   for (b=0; b<4; b++) {
      for (i=0; i<8; i++) {
	 data[b] <<= 1;
	 outportb(base, ~D1);          /* clock low */
	 outportb(base, ~D1);
	 outportb(base, ~D1);

	 data[b] |= (inportb(base+1) & ACK) ? 1 : 0;

	 outportb(base, N64_HI);       /* clock high */
	 outportb(base, N64_HI);
	 outportb(base, N64_HI);
      }
   }

   /* failed pad comms */
   if ((data[1] & 0x80) || (data[1] & 0x40)) {
      for (i=0; i<4; i++)
	 data[i] = 0;
   }

   /* A */
   joy[joynum].button[0].b = (data[0] & 0x80) ? 1 : 0;

   /* B */
   joy[joynum].button[1].b = (data[0] & 0x40) ? 1 : 0;

   /* Z */
   joy[joynum].button[8].b = (data[0] & 0x20) ? 1 : 0;

   /* start */
   joy[joynum].button[9].b = (data[0] & 0x10) ? 1 : 0;

   /* direction pad up */
   joy[joynum].stick[1].axis[1].d1 = (data[0] & 0x08) ? 1 : 0;

   /* direction pad down */
   joy[joynum].stick[1].axis[1].d2 = (data[0] & 0x04) ? 1 : 0;

   /* direction pad left */
   joy[joynum].stick[1].axis[0].d1 = (data[0] & 0x02) ? 1 : 0;

   /* direction pad right */
   joy[joynum].stick[1].axis[0].d2 = (data[0] & 0x01) ? 1 : 0;

   /* setup analog controls for direction pad */
   for (i=0; i<2; i++) {
      if (joy[joynum].stick[1].axis[i].d1) {
	 joy[joynum].stick[1].axis[i].pos = -128;
      }
      else if (joy[joynum].stick[1].axis[i].d2) {
	 joy[joynum].stick[1].axis[i].pos = 128;
      }
      else {
	 joy[joynum].stick[1].axis[i].pos = 0;
      }
   }

   /* L */
   joy[joynum].button[6].b = (data[1] & 0x20) ? 1 : 0;

   /* R */
   joy[joynum].button[7].b = (data[1] & 0x10) ? 1 : 0;

   /* C up */
   joy[joynum].button[5].b = (data[1] & 0x08) ? 1 : 0;

   /* C down */
   joy[joynum].button[2].b = (data[1] & 0x04) ? 1 : 0;

   /* C left */
   joy[joynum].button[3].b = (data[1] & 0x02) ? 1 : 0;

   /* C right */
   joy[joynum].button[4].b = (data[1] & 0x01) ? 1 : 0;

   /* stick X axis */
   joy[joynum].stick[0].axis[0].pos = (signed char)data[2] * 1.6;

   /* stick Y axis */
   joy[joynum].stick[0].axis[1].pos = -(signed char)data[3] * 1.6;

   /* setup digital controls for analog stick */
   joy[joynum].stick[0].axis[0].d1 = (joy[joynum].stick[0].axis[0].pos < -64);
   joy[joynum].stick[0].axis[0].d2 = (joy[joynum].stick[0].axis[0].pos > 64);
   joy[joynum].stick[0].axis[1].d1 = (joy[joynum].stick[0].axis[1].pos < -64);
   joy[joynum].stick[0].axis[1].d2 = (joy[joynum].stick[0].axis[1].pos > 64);

   return 0; 
}



/* n64p1_poll:
 *  Input polling function.
 */
static int n64p1_poll(void)
{
   int joynum;

   for (joynum=0; joynum<num_joysticks; joynum++)
      n64_poll(LPT1_BASE, joynum);

   return 0;
}



/* n64p2_poll:
 *  Input polling function.
 */
static int n64p2_poll(void)
{
   int joynum;

   for (joynum=0; joynum<num_joysticks; joynum++)
      n64_poll(LPT2_BASE, joynum);

   return 0;
}



/* n64p3_poll:
 *  Input polling function.
 */
static int n64p3_poll(void)
{
   int joynum;

   for (joynum=0; joynum<num_joysticks; joynum++)
      n64_poll(LPT3_BASE, joynum);

   return 0;
}

