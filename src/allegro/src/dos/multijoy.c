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
 *      Drivers for multisystem joysticks (Atari, Commodore 64 etc.)
 *      with 9-pin connectors.
 *
 *      By Fabrizio Gennari.
 *
 *      JOY_TYPE_DB9_LPT[123]
 *
 *      Supports 2 two-button joysticks. Port 1 is compatible with Linux
 *      joy-db9 driver (multisystem 2-button), and port 2 is compatible
 *      with Atari interface for DirectPad Pro.
 *
 *      Based on joy-db9 driver for Linux by Vojtech Pavlik
 *      and on Atari interface for DirectPad Pro by Earle F. Philhower, III
 *
 *      Interface pinout
 *
 *
 *      Parallel port                           Joystick port 1
 *       1----------------------------------------------------1
 *      14----------------------------------------------------2
 *      16----------------------------------------------------3
 *      17----------------------------------------------------4
 *      11----------------------------------------------------6
 *      12----------------------------------------------------7 (button 2)
 *      18----------------------------------------------------8
 *
 *                                              Joystick port 2
 *       2----------------------------------------------------1
 *       3----------------------------------------------------2
 *       4----------------------------------------------------3
 *       5----------------------------------------------------4
 *       6----------------------------------------------------6
 *       7----------------------------------------------------7 (button 2)
 *      19----------------------------------------------------8
 *
 *      Advantages
 *
 *      * Simpler to build (no diodes required)
 *      * Autofire will work (if joystick supports it)
 *
 *      Drawbacks
 *
 *      * The parallel port must be in SPP (PS/2) mode in order for this
 *        driver to work. In Normal mode, port 2 won't work because data
 *        pins are not inputs. In EPP/ECP PS/2 mode, directions for
 *        port 1 won't work (buttons will) beacuse control pins are not
 *        inputs.
 *
 *       * The parallel port should not require pull-up resistors
 *         (however, newer ones don't)
 *
 *      JOY_TYPE_TURBOGRAFX_LPT[123]
 *
 *      Supports up to 7 joysticks, each one with up to 5 buttons.
 *
 *      Original interface and driver by Steffen Schwenke
 *      See <http://www.burg-halle.de/~schwenke/parport.html> for details
 *      on how to build the interface
 *
 *      Advantages
 *
 *      * Exploits the parallel port to its limits
 *
 *      Drawbacks
 *
 *      * Autofire will not work
 */


#include "allegro.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif

#define LPT1_BASE 0x378
#define LPT2_BASE 0x278
#define LPT3_BASE 0x3bc

/* driver functions */
static int db91_init(void);
static int db92_init(void);
static int db93_init(void);
static void db91_exit(void);
static void db92_exit(void);
static void db93_exit(void);
static int db91_poll(void);
static int db92_poll(void);
static int db93_poll(void);

static int tgx1_init(void);
static int tgx2_init(void);
static int tgx3_init(void);
static void tgx1_exit(void);
static void tgx2_exit(void);
static void tgx3_exit(void);
static int tgx1_poll(void);
static int tgx2_poll(void);
static int tgx3_poll(void);

JOYSTICK_DRIVER joystick_db91 =
{
   JOY_TYPE_DB9_LPT1,
   NULL,
   NULL,
   "DB9-LPT1",
   db91_init,
   db91_exit,
   db91_poll,
   NULL, NULL,
   NULL, NULL
};

JOYSTICK_DRIVER joystick_db92 =
{
   JOY_TYPE_DB9_LPT2,
   NULL,
   NULL,
   "DB9-LPT2",
   db92_init,
   db92_exit,
   db92_poll,
   NULL, NULL,
   NULL, NULL
};

JOYSTICK_DRIVER joystick_db93 =
{
   JOY_TYPE_DB9_LPT3,
   NULL,
   NULL,
   "DB9-LPT3",
   db93_init,
   db93_exit,
   db93_poll,
   NULL, NULL,
   NULL, NULL
};

JOYSTICK_DRIVER joystick_tgx1 =
{
   JOY_TYPE_TURBOGRAFX_LPT1,
   NULL,
   NULL,
   "TGX-LPT1",
   tgx1_init,
   tgx1_exit,
   tgx1_poll,
   NULL, NULL,
   NULL, NULL
};

JOYSTICK_DRIVER joystick_tgx2 =
{
   JOY_TYPE_TURBOGRAFX_LPT2,
   NULL,
   NULL,
   "TGX-LPT2",
   tgx2_init,
   tgx2_exit,
   tgx2_poll,
   NULL, NULL,
   NULL, NULL
};

JOYSTICK_DRIVER joystick_tgx3 =
{
   JOY_TYPE_TURBOGRAFX_LPT3,
   NULL,
   NULL,
   "TGX-LPT3",
   tgx3_init,
   tgx3_exit,
   tgx3_poll,
   NULL, NULL,
   NULL, NULL
};

#define STATUS_PORT_INVERT  0x80
#define CONTROL_PORT_INVERT 0x0B

static int db9_init (int base)
{
   num_joysticks = 2;

   joy[0].flags = joy[1].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
   joy[0].num_sticks = joy[1].num_sticks = 1;
   joy[0].num_buttons = joy[1].num_buttons = 2;
   joy[0].stick[0].flags = 
   joy[1].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
   joy[0].stick[0].num_axis = 
   joy[1].stick[0].num_axis = 2;
   joy[0].stick[0].axis[0].name = 
   joy[1].stick[0].axis[0].name = get_config_text("Position x");
   joy[0].stick[0].axis[1].name = 
   joy[1].stick[0].axis[1].name = get_config_text("Position y");
   joy[0].stick[0].name = 
   joy[1].stick[0].name = get_config_text("Position");
   joy[0].button[0].name = 
   joy[1].button[0].name = get_config_text("Button 1");
   joy[0].button[1].name = 
   joy[1].button[0].name = get_config_text("Button 2");

   /* turns the 4 control bits and the 8 data bits into inputs */

   outportb(base + 2, 0x2F ^ CONTROL_PORT_INVERT);

   return 0;
}

static int db91_init(void)
{
   return db9_init(LPT1_BASE);
}

static int db92_init(void)
{
   return db9_init(LPT2_BASE);
}

static int db93_init(void)
{
   return db9_init(LPT3_BASE);
}

static void db91_exit(void)
{
   outportb(LPT1_BASE + 2    , 0 ^ CONTROL_PORT_INVERT);
   outportb(LPT1_BASE + 0x402, 0x15);
}

static void db92_exit(void)
{
   outportb(LPT2_BASE + 2    , 0 ^ CONTROL_PORT_INVERT);
   outportb(LPT2_BASE + 0x402, 0x15);
}

static void db93_exit(void)
{
   outportb(LPT3_BASE + 2    , 0 ^ CONTROL_PORT_INVERT);
   outportb(LPT3_BASE + 0x402, 0x15);
}

static void db9_poll(int base)
{
   unsigned char data, control, status;

   data    = inportb (base);
   status  = inportb (base + 1) ^ STATUS_PORT_INVERT;
   control = inportb (base + 2) ^ CONTROL_PORT_INVERT;

   joy[1].stick[0].axis[0].d1 = (data & 4) ? 0 : 1;
   joy[1].stick[0].axis[0].d2 = (data & 8) ? 0 : 1;
   joy[1].stick[0].axis[1].d1 = (data & 1) ? 0 : 1;
   joy[1].stick[0].axis[1].d2 = (data & 2) ? 0 : 1;

   joy[1].stick[0].axis[0].pos = ((data & 8) ? 0 : 128) - ((data & 4) ? 0 : 128);
   joy[1].stick[0].axis[1].pos = ((data & 2) ? 0 : 128) - ((data & 1) ? 0 : 128);

   joy[1].button[0].b = (data & 16) ? 0 : 1;
   joy[1].button[1].b = (data & 32) ? 0 : 1;

   joy[0].stick[0].axis[0].d1 = (control & 4) ? 0 : 1;
   joy[0].stick[0].axis[0].d2 = (control & 8) ? 0 : 1;
   joy[0].stick[0].axis[1].d1 = (control & 1) ? 0 : 1;
   joy[0].stick[0].axis[1].d2 = (control & 2) ? 0 : 1;

   joy[0].stick[0].axis[0].pos = ((control & 8) ? 0 : 128) - ((control & 4) ? 0 : 128);
   joy[0].stick[0].axis[1].pos = ((control & 2) ? 0 : 128) - ((control & 1) ? 0 : 128);

   joy[0].button[0].b = (status & 128) ? 0 : 1;
   joy[0].button[1].b = (status & 32 ) ? 0 : 1;
}

static int db91_poll(void)
{
   db9_poll (LPT1_BASE);
   return 0;
}

static int db92_poll(void)
{
   db9_poll (LPT2_BASE);
   return 0;
}

static int db93_poll(void)
{
   db9_poll (LPT3_BASE);
   return 0;
}

static int tgx_init(int base)
{
   int number;
   num_joysticks=7;
   for (number=0;number<7;number++){
	joy[number].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
	joy[number].num_sticks = 1;
	joy[number].num_buttons = 5;
	joy[number].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
	joy[number].stick[0].num_axis = 2;
	joy[number].stick[0].axis[0].name = get_config_text ("Position x");
	joy[number].stick[0].axis[1].name = get_config_text ("Position y");
	joy[number].stick[0].name = get_config_text ("Position");
	joy[number].button[0].name = get_config_text("Button 1");
	joy[number].button[1].name = get_config_text("Button 2");
	joy[number].button[2].name = get_config_text("Button 3");
	joy[number].button[3].name = get_config_text("Button 4");
	joy[number].button[4].name = get_config_text("Button 5");
   }

   /* turns the 4 control bits into inputs */

   outportb(base + 2, 0x0F ^ CONTROL_PORT_INVERT);

   /* If the parallel port is ECP, ensure it is in standard mode */

   outportb(base + 0x402, 0x04);

   return 0;
}

static int tgx1_init(void)
{
   return tgx_init(LPT1_BASE);
}

static int tgx2_init(void)
{
   return tgx_init(LPT2_BASE);
}

static int tgx3_init(void)
{
   return tgx_init(LPT3_BASE);
}

static void tgx1_exit(void)
{
   outportb (LPT1_BASE + 2, 0 ^ CONTROL_PORT_INVERT);
}

static void tgx2_exit(void)
{
   outportb (LPT2_BASE + 2, 0 ^ CONTROL_PORT_INVERT);
}

static void tgx3_exit(void)
{
   outportb (LPT3_BASE + 2, 0 ^ CONTROL_PORT_INVERT);
}

static void tgx_poll(int base)
{
   unsigned char control, status;
   int number;

   for (number=0;number<7;number++){

	outportb (base, ~(1 << number));

	status  = inportb (base + 1) ^ STATUS_PORT_INVERT;
	control = inportb (base + 2) ^ CONTROL_PORT_INVERT;

	joy[number].stick[0].axis[0].d1 = (status & 64 ) ? 0 : 1;
	joy[number].stick[0].axis[0].d2 = (status & 128) ? 0 : 1;
	joy[number].stick[0].axis[1].d1 = (status & 16 ) ? 0 : 1;
	joy[number].stick[0].axis[1].d2 = (status & 32 ) ? 0 : 1;

	joy[number].stick[0].axis[0].pos = ((status & 128) ? 0 : 128) - ((status & 64) ? 0 : 128);
	joy[number].stick[0].axis[1].pos = ((status & 32 ) ? 0 : 128) - ((status & 16) ? 0 : 128);

	joy[number].button[0].b = (status  & 8) ? 0 : 1;
	joy[number].button[1].b = (control & 2) ? 0 : 1;
	joy[number].button[2].b = (control & 4) ? 0 : 1;
	joy[number].button[3].b = (control & 1) ? 0 : 1;
	joy[number].button[4].b = (control & 8) ? 0 : 1;
   }
}
static int tgx1_poll(void)
{
   tgx_poll (LPT1_BASE);
   return 0;
}

static int tgx2_poll(void)
{
   tgx_poll (LPT2_BASE);
   return 0;
}

static int tgx3_poll(void)
{
   tgx_poll (LPT3_BASE);
   return 0;
}
