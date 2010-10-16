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
 *      Joystick driver for PSX controllers.
 *
 *      By Richard Davies.
 *
 *      Based on sample code by Earle F. Philhower, III. from DirectPad
 *      Pro 4.9, for use with the DirectPad Pro parallel port interface.
 *
 *      See <http://www.ziplabel.com/dpadpro> for interface details.
 *
 *      Original parallel port interface and code by Juan Berrocal.
 *
 *      Digital, Analog, Dual Force (control), NegCon and Mouse
 *      information by T. Fujita. Dual Shock (vibrate) information by
 *      Dark Fader. Multi tap, G-con45 and Jogcon information by me
 *      (the weird stuff ;)
 *
 *      This driver recognises Digital, Analog, Dual Shock, Mouse, negCon,
 *      G-con45, Jogcon, Konami Lightgun and Multi tap devices.
 *
 *      Digital, Dual Shock, neGcon, G-con45, Jogcon and Multi tap devices
 *      have all been tested. The Analog (green mode or joystick), Mouse
 *      and Konami Lightgun devices have not. The position data is likely
 *      to be broken for the Konami Lightgun, and may also be broken for
 *      the Mouse.
 *
 *      The G-con45 needs to be connected to (and pointed at) a TV type
 *      monitor connected to your computer. The composite video out on my
 *      video card works fine for this.
 *
 *      The Sony Dual Shock or Namco Jogcon will reset themselves (to
 *      digital mode) after not being polled for 5 seconds. This is normal,
 *      the same thing happens on a Playstation, it's meant to stop any
 *      vibration in case the host machine crashes. However, if this
 *      happens to a Jogcon controller the mode button is disabled. To
 *      reenable the mode button on the Jogcon hold down the Start and
 *      Select buttons at the same time. Other mode switching controllers
 *      may have similar quirks.
 *
 *      Some people may have problems with the psx poll delay set at 3
 *      causing them twitchy controls (this depends on the controllers
 *      more than anything else).
 *
 *      It may be worthwhile to add calibration to some of the analog
 *      controls, although most controller types aren't really meant to
 *      be calibrated:
 *
 *      - My Dual Shock controller centres really badly; most of them do.
 *      - My neGcon centres really well (+/- 1).
 *      - The G-con45 needs calibration for it's centre aim and velocity.
 *      - The Jogcon calibrates itself when initialised.
 *
 *      To Do List:
 *
 *      - Verify Analog Joystick (Green mode) works.
 *      - Verify Mouse position works.
 *      - Verify MegaTap interface.
 *      - Add calibration for the G-con45, Dual Shock and neGcon controllers.
 *      - Implement Konami Lightgun aim.
 *      - Implement Jogcon force feedback.
 *      - Implement unsupported controllers (Ascii Sphere? Beat Mania Decks?)
 *
 *      If you can help with any of these then please let me know.
 *
 *
 *       -----------------------
 *      | o o o | o o o | o o o | Controller Port (looking into plug)
 *       \_____________________/
 *        1 2 3   4 5 6   7 8 9
 *
 *      Controller          Parallel
 *
 *      1 - data            10 (conport 1, 3, 4, 5, 6), 13 (conport 2)
 *      2 - command         2
 *      3 - 9V(shock)       +9V power supply terminal for Dual Shock
 *      4 - gnd             18,19 also -9V and/or -5V power supply terminal
 *      5 - V+              5, 6, 7, 8, 9 through diodes, or +5V power supply terminal
 *      6 - att             3 (conport 1, 2), 5 (conport 3), 6 (conport 4), 7 (conport 5), 8 (conport 6)
 *      7 - clock           4
 *      9 - ack             12 (conport 1, 3, 4, 5, 6), 14 (conport 2) **
 *
 *      ** There is an error in the DirectPad Pro documentation, which states that
 *         the second control port should have pin 9 connected to pin 15 on the
 *         parallel port. This should actually be pin 14 on the parallel port. To
 *         make things more confusing, this error is unlikely to prevent the
 *         interface from working properly. It's also possible that a change to the
 *         scanning code has been made in version 5.0 to prevent people from having
 *         to rewire their interfaces.
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

#define LPT_D0  0x01
#define LPT_D1  0x02
#define LPT_D2  0x04
#define LPT_D3  0x08
#define LPT_D4  0x10
#define LPT_D5  0x20
#define LPT_D6  0x40
#define LPT_D7  0x80
#define LPT_AUT 0x08
#define LPT_SEL 0x10
#define LPT_PAP 0x20
#define LPT_ACK 0x40



#define PSX_MAX_DATA     33     /* maximum possible length of controller packet in bytes */
#define PSX_MAX_DETECT   8      /* maximum number of controllers to install */
#define PSX_DELAY        3

				/* JAP code  EUR code    Name */

#define PSX_MOUSE        1      /* SCPH-1030 SCPH-????   Mouse */
#define PSX_NEGCON       2      /* SLPH-0001 SLEH-0003   Namco neGcon
				   SLPH-0007             Nasca Pachinco Handle (!)
				   SLPH-0015             ? Volume Controller (!)
				   SLPH-???? SLEH-0006(?)MadKatz Steering Wheel */
#define PSX_KONAMIGUN    3      /* SLPH-???? SLPH-????   Konami Lightgun */
#define PSX_DIGITAL      4      /* SCPH-1010 SCPH 1080 E Controller
				   SCPH-1110 SCPH-????   Analog Joystick - Digital Mode
				   SCPH-1150 SCPH-1200 E Dual Shock Analog Controller - Digital Mode 
				   SLPH-???? SLEH-0011   Ascii Resident Evil Pad
				   SLPH-???? SLEH-0004   Namco Arcade Stick */
#define PSX_ANALOG_GREEN 5      /* SCPH-1110 SCPH-????   Analog Joystick - Analog Mode
				   SCPH-1150 SCPH-1200 E Dual Shock Analog Controller - Analog Green Mode */
#define PSX_GUNCON       6      /* SLPH-???? SLEH-0007   Namco G-con45 */
#define PSX_ANALOG_RED   7      /* SCPH-1150 SCPH-1200 E Dual Shock Analog Controller - Analog Red Mode */
#define PSX_JOGCON       14     /* SLPH-???? SLEH-0020   Namco Jogcon */
/*      PSX_MULTITAP           SCPH-1070 SCPH-1070 E Multi tap */



/* driver functions */
static int psx1_init(void);
static int psx2_init(void);
static int psx3_init(void);
static void psx_exit(int base);
static void psx1_exit(void);
static void psx2_exit(void);
static void psx3_exit(void);
static int psx1_poll(void);
static int psx2_poll(void);
static int psx3_poll(void);
static int psx_detect(int base, int conport, int tap);



JOYSTICK_DRIVER joystick_psx1 =
{
   JOY_TYPE_PSXPAD_LPT1,
   NULL,
   NULL,
   "PSXpad-LPT1",
   psx1_init,
   psx1_exit,
   psx1_poll,
   NULL, NULL,
   NULL, NULL
};



JOYSTICK_DRIVER joystick_psx2 =
{
   JOY_TYPE_PSXPAD_LPT2,
   NULL,
   NULL,
   "PSXpad-LPT2",
   psx2_init,
   psx2_exit,
   psx2_poll,
   NULL, NULL,
   NULL, NULL
};



JOYSTICK_DRIVER joystick_psx3 =
{
   JOY_TYPE_PSXPAD_LPT3,
   NULL,
   NULL,
   "PSXpad-LPT3",
   psx3_init,
   psx3_exit,
   psx3_poll,
   NULL, NULL,
   NULL, NULL
};



static unsigned char psx_parallel_out = 0xff;

static int psx_first_poll = 1;

typedef struct {
   int conport;
   int tap;
   int type;
} PSX_INFO;

static PSX_INFO psx_detected[PSX_MAX_DETECT];



/* psx_init:
 *  Initialise joy[] for Allegro depending on what's detected.
 */
static int psx_init(int base)
{
   int i, j, type;

   num_joysticks = 0;

   /* test for the presence of controllers on port base */
   for (i=0; i<6; i++) {
      for (j=1; j<5; j++) {
	 type = psx_detect(base, i, j);

	 if (type != -1) {
	    psx_detected[num_joysticks].conport = i;
	    psx_detected[num_joysticks].tap = j;
	    psx_detected[num_joysticks].type = type;
	    num_joysticks++;
	 }

	 if (num_joysticks == PSX_MAX_DETECT)
	    i = 6;
      }
   }

   if (!num_joysticks)
      return -1;

   for (i=0; i<num_joysticks; i++) {
      switch (psx_detected[i].type) {

	 case PSX_MOUSE:
	    joy[i].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;

	    joy[i].num_sticks = 1;

	    /* position */
	    joy[i].stick[0].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	    joy[i].stick[0].num_axis = 2;
	    joy[i].stick[0].axis[0].name = get_config_text("Position X");
	    joy[i].stick[0].axis[1].name = get_config_text("Position y");
	    joy[i].stick[0].name = get_config_text("Position");

	    joy[i].num_buttons = 2;

	    joy[i].button[0].name = get_config_text("Right");
	    joy[i].button[1].name = get_config_text("Left");
	    break;

	 case PSX_NEGCON:
	    joy[i].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;

	    joy[i].num_sticks = 3;

	    /* direction pad */
	    joy[i].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
	    joy[i].stick[0].num_axis = 2;
	    joy[i].stick[0].axis[0].name = get_config_text("Direction Pad X");
	    joy[i].stick[0].axis[1].name = get_config_text("Direction Pad Y");
	    joy[i].stick[0].name = get_config_text("Direction Pad");

	    /* twist */
	    joy[i].stick[1].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	    joy[i].stick[1].num_axis = 1;
	    joy[i].stick[1].name = joy[i].stick[1].axis[0].name = get_config_text("Twist");

	    /* analogue buttons */
	    joy[i].stick[2].flags = JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
	    joy[i].stick[2].num_axis = 3;
	    joy[i].stick[2].name = get_config_text("Analogue Buttons");

	    /* I Button */
	    joy[i].stick[2].axis[0].name = get_config_text("I Button");

	    /* II Button */
	    joy[i].stick[2].axis[1].name = get_config_text("II Button");

	    /* L1 Button */
	    joy[i].stick[2].axis[2].name = get_config_text("L1 Button");

	    joy[i].num_buttons = 4;

	    joy[i].button[0].name = get_config_text("A");
	    joy[i].button[1].name = get_config_text("B");
	    joy[i].button[2].name = get_config_text("R1");
	    joy[i].button[3].name = get_config_text("Start");
	    break;

	 case PSX_GUNCON:
	    joy[i].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;

	    joy[i].num_sticks = 1;

	    /* aim */
	    joy[i].stick[0].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	    joy[i].stick[0].num_axis = 2;
	    joy[i].stick[0].axis[0].name = get_config_text("Aim X");
	    joy[i].stick[0].axis[1].name = get_config_text("Aim y");
	    joy[i].stick[0].name = get_config_text("Aim");

	    joy[i].num_buttons = 3;

	    joy[i].button[0].name = get_config_text("Trigger");
	    joy[i].button[1].name = get_config_text("A");
	    joy[i].button[2].name = get_config_text("B");
	    break;

	 case PSX_KONAMIGUN:
	    joy[i].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;

	    joy[i].num_sticks = 1;

	    /* aim */
	    joy[i].stick[0].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	    joy[i].stick[0].num_axis = 2;
	    joy[i].stick[0].axis[0].name = get_config_text("Aim X");
	    joy[i].stick[0].axis[1].name = get_config_text("Aim y");
	    joy[i].stick[0].name = get_config_text("Aim");

	    joy[i].num_buttons = 3;

	    joy[i].button[0].name = get_config_text("Trigger");
	    joy[i].button[1].name = get_config_text("Back");
	    joy[i].button[2].name = get_config_text("Start");
	    break;

	 case PSX_DIGITAL:
	 case PSX_ANALOG_GREEN:
	 case PSX_ANALOG_RED:
	 case PSX_JOGCON:
	 default:
	    joy[i].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;

	    joy[i].num_sticks = 4;

	    /* left stick */
	    joy[i].stick[0].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	    joy[i].stick[0].num_axis = 2;
	    joy[i].stick[0].axis[0].name = get_config_text("Left Stick X");
	    joy[i].stick[0].axis[1].name = get_config_text("Left Stick Y");
	    joy[i].stick[0].name = get_config_text("Left Stick");

	    /* right stick */
	    joy[i].stick[1].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	    joy[i].stick[1].num_axis = 2;
	    joy[i].stick[1].axis[0].name = get_config_text("Right Stick X");
	    joy[i].stick[1].axis[1].name = get_config_text("Right Stick Y");
	    joy[i].stick[1].name = get_config_text("Right Stick");

	    /* direction pad */
	    joy[i].stick[2].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
	    joy[i].stick[2].num_axis = 2;
	    joy[i].stick[2].axis[0].name = get_config_text("Direction Pad X");
	    joy[i].stick[2].axis[1].name = get_config_text("Direction Pad Y");
	    joy[i].stick[2].name = get_config_text("Direction Pad");

	    /* jogcon dial */
	    joy[i].stick[3].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	    joy[i].stick[3].num_axis = 1;
	    joy[i].stick[3].name = joy[i].stick[3].axis[0].name = get_config_text("Dial");

	    joy[i].num_buttons = 12;

	    joy[i].button[0].name = get_config_text("Cross");
	    joy[i].button[1].name = get_config_text("Circle");
	    joy[i].button[2].name = get_config_text("Square");
	    joy[i].button[3].name = get_config_text("Triangle");
	    joy[i].button[4].name = get_config_text("L1");
	    joy[i].button[5].name = get_config_text("R1");
	    joy[i].button[6].name = get_config_text("L2");
	    joy[i].button[7].name = get_config_text("R2");
	    joy[i].button[8].name = get_config_text("Select");
	    joy[i].button[9].name = get_config_text("Start");
	    joy[i].button[10].name = get_config_text("L3");
	    joy[i].button[11].name = get_config_text("R3");
	    break;
      }
   }

   return 0;
}



static int psx1_init(void)
{
   return psx_init(LPT1_BASE);
}



static int psx2_init(void)
{
   return psx_init(LPT2_BASE);
}



static int psx3_init(void)
{
   return psx_init(LPT3_BASE);
}



/*
 * powers down all parallel port driven conports on port base
 * disabled - otherwise prevents Sony Dual Shock controller crash protection
 */

static void psx1_exit(void)
{
   /*outportb(LPT1_BASE + 0, 0); */
}



static void psx2_exit(void)
{
   /*outportb(LPT2_BASE + 0, 0); */
}



static void psx3_exit(void)
{
   /*outportb(LPT3_BASE + 0, 0); */
}



/*
 * sets clock of any conport connected to parallel port base 
 */
static void psx_clk(int base, int on)
{
   const int clk = LPT_D2;      /* bit 3 base + 0 (pin 4 parallel port) */

   if (on) {
      /* set controller clock high */
      psx_parallel_out |= clk;
   }
   else {
      /* set controller clock low */
      psx_parallel_out &= ~clk;
   }

   outportb(base+0, psx_parallel_out);
}



/*
 * sets att for conport connected to parallel port base
 */
static void psx_sel(int base, int conport, int on)
{
   /* bits 3-7 base + 0 (pins 5 to 9 parallel port) */
   const int power = LPT_D3 | LPT_D4 | LPT_D5 | LPT_D6 | LPT_D7;
   /* bits 1-6 base + 0 (pins 3, 5, 6, 7 and 8 parallel port) */
   static unsigned char att_array[] = {LPT_D1, LPT_D1, LPT_D3, LPT_D4, LPT_D5, LPT_D6};
   unsigned char att = att_array[conport];

   /* powers up all parallel port driven conports */
   psx_parallel_out |= power;

   if (on) {
      /* set controller att high */
      psx_parallel_out |= att;
   }
   else {
      /* set controller att low */
      psx_parallel_out &= ~att;
   }

   outportb(base+0, psx_parallel_out);
}



/*
 * sets command for any conport connected to parallel port base
 */
static void psx_cmd(int base, int on)
{
   const int cmd = LPT_D0;      /* bit 0 base + 0 (pin 2 parallel port) */

   if (on) {
      /* set controller cmd high */
      psx_parallel_out |= cmd;
   }
   else {
      /* set controller cmd low */
      psx_parallel_out &= ~cmd;
   }

   outportb(base+0, psx_parallel_out);
}



/*
 * tests data for conport connected to parallel port base, returns 1 if high
 */
static int psx_dat(int base, int conport)
{
   unsigned char data;

   if (conport == 1)
      data = LPT_SEL;
   else
      data = LPT_ACK;

   if (inportb(base + 1) & data)
      return 1;
   else
      return 0;
}



/*
 * tests ack for conport connected to parallel port base, returns 1 if high
 */
static int psx_ack(int base, int conport)
{
   unsigned char ack;

   if (conport == 1)
      ack = LPT_AUT;
   else
      ack = LPT_PAP;

   if (inportb(base + 1) & ack)
      return 1;
   else
      return 0;
}



/*
 * wait for PSX_DELAY * (outportb() execution time) 
 */
static void psx_delay(int base)
{
   int i;

   for (i=0; i<PSX_DELAY; i++)
      outportb(base+0, psx_parallel_out);
}



/* 
 * send byte as a command to conport connected to parallel port base
 * assumes clock high and the attention for conport 
 */
static unsigned char psx_sendbyte(int base, int conport, unsigned char byte, int wait)
{
   int i;
   unsigned char data;

   data = 0;

   /* for each bit in byte */
   for (i=0; i<8; i++) {
      psx_delay(base);
      psx_cmd(base, byte & (1<<i));    /* send the (i+1)th bit of byte to any listening controller */
      psx_clk(base, 0);                /* clock low */
      psx_delay(base);
      data |= (psx_dat(base, conport) ? (1<<i) : 0);  /* read the (i+1)th bit of data from conport */
      psx_clk(base, 1);                /* clock high */
   }

   /* delay for 30 and wait for controller ack */
   for (i=0; wait && i < 30 && psx_ack(base, conport); i++)
      ;

   return data;
}



/*
 * sets clock high and gets the attention of conport, use before psx_sendbyte()
 */
static void psx_sendinit(int base, int conport)
{
   psx_sel(base, conport, 1);   /* set att on for conport */
   psx_clk(base, 1);            /* clock high */
   psx_cmd(base, 1);            /* set command on for conport */
   psx_delay(base);
   psx_delay(base);
   psx_delay(base);
   psx_delay(base);
   psx_delay(base);
   psx_sel(base, conport, 0);   /* set att off for conport */
   psx_delay(base);
   psx_delay(base);
   psx_delay(base);
   psx_delay(base);
}



/*
 * use after psx_sendbyte()
 */
static void psx_sendclose(int base, int conport)
{
   psx_delay(base);
   psx_delay(base);
   psx_sel(base, conport, 1);   /* set att on for conport */
   psx_cmd(base, 1);            /* set command on for conport */
   psx_clk(base, 1);            /* clock high */
   psx_delay(base);
}



/* 
 * send string as a series of commands to conport connected to parallel port base
 */
static void psx_sendstring(int base, int conport, int string[])
{
   int i;

   psx_sendinit(base, conport);

   /* for each byte in string */
   for (i=0; string[i+1] != -1; i++) {
      /* send byte i and wait for conport ack */
      psx_sendbyte(base, conport, (unsigned char)string[i], 1);
   }

   /* send the last byte in string and don't wait for ack */
   psx_sendbyte(base, conport, (unsigned char)string[i], 0);

   psx_sendclose(base, conport);
}



/*
 * sends the dual shock command sequence to conport:tap on port base
 */
/*
static void psx_vibrate(int base, int conport, int tap, int shock, int rumble)
{
   static int dualshock_string[4][12] =
   {
      { -1, 0x43, 0x00, 0x01, 0x00, 0x01, -1 },
      { -1, 0x4d, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x01, -1 },
      { -1, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, -1 },
      { -1, 0x42, 0x00, -1, -1, 0x01, -1 }
   };

   int i;

   dualshock_string[0][0] = tap;
   dualshock_string[1][0] = tap;
   dualshock_string[2][0] = tap;
   dualshock_string[3][0] = tap;

   dualshock_string[3][3] = shock;
   dualshock_string[3][4] = rumble;

   for (i=0; i<4; i++) {
      psx_delay(base);
      psx_delay(base);
      psx_delay(base);
      psx_sendstring(base, conport, dualshock_string[i]);
   }
}
*/



/*
 * sends force feedback/shock init command sequence to conport:tap on port base
 * (also initialises crash protection for some controllers)
 */
static void psx_vibrate_init(int base, int conport, int tap)
{
   static int jogcon_string[3][12] =
   {
      { -1, 0x43, 0x00, 0x01, 0x00, 0x01, -1 },
      { -1, 0x4d, 0x00, 0x00, 0x01, 0xff, 0xff, 0xff, 0xff, 0x01, -1 },
      { -1, 0x43, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, -1 },
   };

   int i;

   jogcon_string[0][0] = tap;
   jogcon_string[1][0] = tap;
   jogcon_string[2][0] = tap;

   for (i=0; i<3; i++) {
      psx_delay(base);
      psx_delay(base);
      psx_delay(base);
      psx_sendstring(base, conport, jogcon_string[i]);
   }
}



/*
 * tests for the presence of a controller on conport:tap connected to base
 * returns the type if present, otherwise -1
 *
 */
static int psx_detect(int base, int conport, int tap)
{
   unsigned char data[PSX_MAX_DATA];
   int psx_type, packet_length;

   psx_sendinit(base, conport);

   data[0] = psx_sendbyte(base, conport, tap, 1);
   data[1] = psx_sendbyte(base, conport, 0x42, 1);

   psx_sendclose(base, conport);

   psx_type = (data[1] & 0xf0) >> 4;

   packet_length = 3 + (2 * (data[1] & 0x0f));

   /* if the packet is of a legal length */
   if (packet_length < PSX_MAX_DATA)
      /* return the controller type */
      return psx_type;
   else
      return -1;
}



/*
 * retrieves controller data for conport:tap connected to base
 */
static void psx_poll(int base, int conport, int tap, int type, int joynum)
{
   unsigned char data[PSX_MAX_DATA];
   int i, psx_type, packet_length;

   psx_sendinit(base, conport);

   data[0] = psx_sendbyte(base, conport, tap, 1);
   data[1] = psx_sendbyte(base, conport, 0x42, 1);

   psx_type = (data[1] & 0xf0) >> 4;

   packet_length = 3 + (2 * (data[1] & 0x0f));

   /* if the packet is of a legal length */
   if (packet_length < PSX_MAX_DATA) {
      /* read the remaining packet */
      for (i=2; i<packet_length; i++)
	 data[i] = psx_sendbyte(base, conport, 0x00, 1);
   }

   /* setup the controller data for Allegro */
   switch (type) {

      case PSX_NEGCON:
	 /* direction pad left */
	 joy[joynum].stick[0].axis[0].d1 = data[3] & 0x80 ? 0 : 1;

	 /* direction pad down */
	 joy[joynum].stick[0].axis[1].d2 = data[3] & 0x40 ? 0 : 1;

	 /* direction pad right */
	 joy[joynum].stick[0].axis[0].d2 = data[3] & 0x20 ? 0 : 1;

	 /* direction pad up */
	 joy[joynum].stick[0].axis[1].d1 = data[3] & 0x10 ? 0 : 1;

	 /* setup analog controls for direction pad */
	 for (i=0; i<2; i++) {
	    if (joy[joynum].stick[0].axis[i].d1)
	       joy[joynum].stick[0].axis[i].pos = -128;
	    else if (joy[joynum].stick[0].axis[i].d2)
	       joy[joynum].stick[0].axis[i].pos = 128;
	    else
	       joy[joynum].stick[0].axis[i].pos = 0;
	 }

	 /* A */
	 joy[joynum].button[0].b = data[4] & 0x20 ? 0 : 1;

	 /* B */
	 joy[joynum].button[1].b = data[4] & 0x10 ? 0 : 1;

	 /* R1 */
	 joy[joynum].button[2].b = data[4] & 0x08 ? 0 : 1;

	 /* start */
	 joy[joynum].button[3].b = data[3] & 0x08 ? 0 : 1;

	 /* twist */
	 joy[joynum].stick[1].axis[0].pos = data[5] - 128;

	 /* setup digital controls for twist */
	 joy[joynum].stick[1].axis[0].d1 = joy[joynum].stick[1].axis[0].pos < -64;
	 joy[joynum].stick[1].axis[0].d2 = joy[joynum].stick[1].axis[0].pos > 64;

	 /* I */
	 joy[joynum].stick[2].axis[0].pos = data[6];
	 joy[joynum].stick[2].axis[0].d1 = data[6] > 0x7f;

	 /* II */
	 joy[joynum].stick[2].axis[1].pos = data[7];
	 joy[joynum].stick[2].axis[1].d1 = data[7] > 0x7f;

	 /* L1 */
	 joy[joynum].stick[2].axis[2].pos = data[7];
	 joy[joynum].stick[2].axis[2].d1 = data[8] > 0xdf;
	 break;

      case PSX_MOUSE:
	 /* right button */
	 joy[joynum].button[0].b = data[4] & 0x04 ? 0 : 1;

	 /* left button */
	 joy[joynum].button[1].b = data[4] & 0x08 ? 0 : 1;

	 /* position x axis */
	 joy[joynum].stick[0].axis[0].pos -= (signed char)data[6];

	 /* position y axis */
	 joy[joynum].stick[0].axis[1].pos -= (signed char)data[5];

	 /* setup digital controls for position */
	 joy[joynum].stick[0].axis[0].d1 = joy[joynum].stick[1].axis[0].pos < -64;
	 joy[joynum].stick[0].axis[0].d2 = joy[joynum].stick[1].axis[0].pos > 64;
	 joy[joynum].stick[0].axis[1].d1 = joy[joynum].stick[1].axis[1].pos < -64;
	 joy[joynum].stick[0].axis[1].d2 = joy[joynum].stick[1].axis[1].pos > 64;

	 /* set position boundaries */
	 if (joy[joynum].stick[0].axis[0].pos > 128)
	    joy[joynum].stick[0].axis[0].pos = 128;
	 else if (joy[joynum].stick[0].axis[0].pos < -128)
	    joy[joynum].stick[0].axis[0].pos = -128;
	 if (joy[joynum].stick[0].axis[1].pos > 128)
	    joy[joynum].stick[0].axis[1].pos = 128;
	 else if (joy[joynum].stick[0].axis[1].pos < -128)
	    joy[joynum].stick[0].axis[1].pos = -128;
	 break;

      case PSX_GUNCON:
	 /* trigger */
	 joy[joynum].button[0].b = data[4] & 0x20 ? 0 : 1;

	 /* A */
	 joy[joynum].button[1].b = data[3] & 0x08 ? 0 : 1;

	 /* B */
	 joy[joynum].button[2].b = data[4] & 0x40 ? 0 : 1;

	 /* aim */
	 /* bad data */
	 if (!(data[5] == 0x01)) {
	    /* too light and too dark */
	    /*if (!(data[7] == 0x05) && !(data[7] == 0x0a)) */
	    {
	       /* aim X */
	       joy[joynum].stick[0].axis[0].pos = (((data[6] & 0x01) << 8) + data[5]) - 256;

	       /* aim y */
	       joy[joynum].stick[0].axis[1].pos = data[7] - 128;
	    }
	 }

	 /* setup digital controls for aim */
	 joy[joynum].stick[0].axis[0].d1 = joy[joynum].stick[0].axis[0].pos < -64;
	 joy[joynum].stick[0].axis[0].d2 = joy[joynum].stick[0].axis[0].pos > 64;
	 joy[joynum].stick[0].axis[1].d1 = joy[joynum].stick[0].axis[1].pos < -64;
	 joy[joynum].stick[0].axis[1].d2 = joy[joynum].stick[0].axis[1].pos > 64;
	 break;

      case PSX_KONAMIGUN:
	 /* trigger */
	 joy[joynum].button[0].b = data[4] & 0x80 ? 0 : 1;

	 /* back */
	 joy[joynum].button[1].b = data[4] & 0x40 ? 0 : 1;

	 /* start */
	 joy[joynum].button[2].b = data[3] & 0x08 ? 0 : 1;

	 /* aim X axis - probably wrong */
	 joy[joynum].stick[0].axis[0].pos = data[5] - 128;

	 /* aim Y axis - probably wrong */
	 joy[joynum].stick[0].axis[1].pos = data[6] - 128;

	 /* setup digital controls for aim */
	 joy[joynum].stick[0].axis[0].d1 = joy[joynum].stick[0].axis[0].pos < -64;
	 joy[joynum].stick[0].axis[0].d2 = joy[joynum].stick[0].axis[0].pos > 64;
	 joy[joynum].stick[0].axis[1].d1 = joy[joynum].stick[0].axis[1].pos < -64;
	 joy[joynum].stick[0].axis[1].d2 = joy[joynum].stick[0].axis[1].pos > 64;
	 break;

      case PSX_DIGITAL:
      case PSX_ANALOG_GREEN:
      case PSX_ANALOG_RED:
      case PSX_JOGCON:
      default:
	 /* direction pad left */
	 joy[joynum].stick[2].axis[0].d1 = data[3] & 0x80 ? 0 : 1;

	 /* direction pad down */
	 joy[joynum].stick[2].axis[1].d2 = data[3] & 0x40 ? 0 : 1;

	 /* direction pad right */
	 joy[joynum].stick[2].axis[0].d2 = data[3] & 0x20 ? 0 : 1;

	 /* direction pad up */
	 joy[joynum].stick[2].axis[1].d1 = data[3] & 0x10 ? 0 : 1;

	 /* setup analog controls for direction pad */
	 for (i=0; i<2; i++) {
	    if (joy[joynum].stick[2].axis[i].d1)
	       joy[joynum].stick[2].axis[i].pos = -128;
	    else if (joy[joynum].stick[2].axis[i].d2)
	       joy[joynum].stick[2].axis[i].pos = 128;
	    else
	       joy[joynum].stick[2].axis[i].pos = 0;
	 }

	 switch (psx_type) {

	    case PSX_DIGITAL:
	       /* mirror direction pad for unused axes */
	       joy[joynum].stick[0].axis[0].pos = joy[joynum].stick[1].axis[0].pos = joy[joynum].stick[3].axis[0].pos = joy[joynum].stick[2].axis[0].pos;
	       joy[joynum].stick[0].axis[1].pos = joy[joynum].stick[1].axis[1].pos = joy[joynum].stick[2].axis[1].pos;

	       joy[joynum].stick[0].axis[0].d1 = joy[joynum].stick[1].axis[0].d1 = joy[joynum].stick[3].axis[0].d1 = joy[joynum].stick[2].axis[0].d1;
	       joy[joynum].stick[0].axis[0].d2 = joy[joynum].stick[1].axis[0].d2 = joy[joynum].stick[3].axis[0].d2 = joy[joynum].stick[2].axis[0].d2;
	       joy[joynum].stick[0].axis[1].d1 = joy[joynum].stick[1].axis[1].d1 = joy[joynum].stick[2].axis[1].d1;
	       joy[joynum].stick[0].axis[1].d2 = joy[joynum].stick[1].axis[1].d2 = joy[joynum].stick[2].axis[1].d2;
	       break;

	    case PSX_JOGCON:
	       /* dial */
	       joy[joynum].stick[3].axis[0].pos = ((data[6] << 7) + (data[5] >> 1)) - (data[6] & 0x80 ? 32768 : 0);

	       /* setup digital controls for dial */
	       joy[joynum].stick[3].axis[0].d1 = data[7] & 0x02;
	       joy[joynum].stick[3].axis[0].d2 = data[7] & 0x01;

	       /* mirror direction pad for unused axes */
	       joy[joynum].stick[0].axis[0].pos = joy[joynum].stick[1].axis[0].pos = joy[joynum].stick[2].axis[0].pos;
	       joy[joynum].stick[0].axis[1].pos = joy[joynum].stick[1].axis[1].pos = joy[joynum].stick[2].axis[1].pos;

	       joy[joynum].stick[0].axis[0].d1 = joy[joynum].stick[1].axis[0].d1 = joy[joynum].stick[2].axis[0].d1;
	       joy[joynum].stick[0].axis[0].d2 = joy[joynum].stick[1].axis[0].d2 = joy[joynum].stick[2].axis[0].d2;
	       joy[joynum].stick[0].axis[1].d1 = joy[joynum].stick[1].axis[1].d1 = joy[joynum].stick[2].axis[1].d1;
	       joy[joynum].stick[0].axis[1].d2 = joy[joynum].stick[1].axis[1].d2 = joy[joynum].stick[2].axis[1].d2;
	       break;

	       /* analog or unknown controller */
	    default:
	       /* left stick X axis */
	       joy[joynum].stick[0].axis[0].pos = data[7] - 128;

	       /* left stick Y axis */
	       joy[joynum].stick[0].axis[1].pos = data[8] - 128;

	       /* right stick X axis */
	       joy[joynum].stick[1].axis[0].pos = data[5] - 128;

	       /* right stick Y axis */
	       joy[joynum].stick[1].axis[1].pos = data[6] - 128;

	       /* setup digital controls for left and right stick */
	       joy[joynum].stick[0].axis[0].d1 = joy[joynum].stick[0].axis[0].pos < -64;
	       joy[joynum].stick[0].axis[0].d2 = joy[joynum].stick[0].axis[0].pos > 64;
	       joy[joynum].stick[0].axis[1].d1 = joy[joynum].stick[0].axis[1].pos < -64;
	       joy[joynum].stick[0].axis[1].d2 = joy[joynum].stick[0].axis[1].pos > 64;
	       joy[joynum].stick[1].axis[0].d1 = joy[joynum].stick[1].axis[0].pos < -64;
	       joy[joynum].stick[1].axis[0].d2 = joy[joynum].stick[1].axis[0].pos > 64;
	       joy[joynum].stick[1].axis[1].d1 = joy[joynum].stick[1].axis[1].pos < -64;
	       joy[joynum].stick[1].axis[1].d2 = joy[joynum].stick[1].axis[1].pos > 64;

	       /* mirror direction pad for unused axes */
	       joy[joynum].stick[3].axis[0].pos = joy[joynum].stick[2].axis[0].pos;

	       joy[joynum].stick[3].axis[0].d1 = joy[joynum].stick[2].axis[0].d1;
	       joy[joynum].stick[3].axis[0].d2 = joy[joynum].stick[2].axis[0].d2;
	       break;
	 }

	 /* cross */
	 joy[joynum].button[0].b = data[4] & 0x40 ? 0 : 1;

	 /* circle */
	 joy[joynum].button[1].b = data[4] & 0x20 ? 0 : 1;

	 /* square */
	 joy[joynum].button[2].b = data[4] & 0x80 ? 0 : 1;

	 /* triangle */
	 joy[joynum].button[3].b = data[4] & 0x10 ? 0 : 1;

	 /* L1 */
	 joy[joynum].button[4].b = data[4] & 0x04 ? 0 : 1;

	 /* R1 */
	 joy[joynum].button[5].b = data[4] & 0x08 ? 0 : 1;

	 /* L2 */
	 joy[joynum].button[6].b = data[4] & 0x01 ? 0 : 1;

	 /* R2 */
	 joy[joynum].button[7].b = data[4] & 0x02 ? 0 : 1;

	 /* select */
	 joy[joynum].button[8].b = data[3] & 0x01 ? 0 : 1;

	 /* start */
	 joy[joynum].button[9].b = data[3] & 0x08 ? 0 : 1;

	 /* L3 */
	 joy[joynum].button[10].b = data[3] & 0x02 ? 0 : 1;

	 /* R3 */
	 joy[joynum].button[11].b = data[3] & 0x04 ? 0 : 1;

	 /* fix to allow analog mode switching for Jogcon after 5 seconds without polling */
	 if ((joy[joynum].button[8].b) && (joy[joynum].button[9].b))
	    psx_vibrate_init(base, conport, tap);

	 break;
   }
}



static int psx1_poll(void)
{
   int joynum;

   /* send Jogcon init to all controllers */
   if (psx_first_poll) {
      for (joynum=0; joynum<num_joysticks; joynum++) {
	 psx_sendinit(LPT1_BASE, psx_detected[joynum].conport);
	 psx_vibrate_init(LPT1_BASE, psx_detected[joynum].conport, psx_detected[joynum].tap);
	 psx_sendclose(LPT1_BASE, psx_detected[joynum].conport);
      }

      psx_first_poll = 0;
   }

   for (joynum=0; joynum<num_joysticks; joynum++) {
      psx_poll(LPT1_BASE, psx_detected[joynum].conport, psx_detected[joynum].tap, psx_detected[joynum].type, joynum);
   }

   return 0;
}



static int psx2_poll(void)
{
   int joynum;

   /* send Jogcon init to all controllers */
   if (psx_first_poll) {
      for (joynum=0; joynum<num_joysticks; joynum++) {
	 psx_sendinit(LPT2_BASE, psx_detected[joynum].conport);
	 psx_vibrate_init(LPT2_BASE, psx_detected[joynum].conport, psx_detected[joynum].tap);
	 psx_sendclose(LPT2_BASE, psx_detected[joynum].conport);
      }

      psx_first_poll = 0;
   }

   for (joynum=0; joynum<num_joysticks; joynum++) {
      psx_poll(LPT2_BASE, psx_detected[joynum].conport, psx_detected[joynum].tap, psx_detected[joynum].type, joynum);
   }

   return 0;
}



static int psx3_poll(void)
{
   int joynum;

   /* send Jogcon init to all controllers */
   if (psx_first_poll) {
      for (joynum=0; joynum<num_joysticks; joynum++) {
	 psx_sendinit(LPT3_BASE, psx_detected[joynum].conport);
	 psx_vibrate_init(LPT3_BASE, psx_detected[joynum].conport, psx_detected[joynum].tap);
	 psx_sendclose(LPT3_BASE, psx_detected[joynum].conport);
      }

      psx_first_poll = 0;
   }

   for (joynum=0; joynum<num_joysticks; joynum++) {
      psx_poll(LPT3_BASE, psx_detected[joynum].conport, psx_detected[joynum].tap, psx_detected[joynum].type, joynum);
   }

   return 0;
}


