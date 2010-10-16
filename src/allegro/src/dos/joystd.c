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
 *      DOS joystick routines.
 *
 *      By Shawn Hargreaves.
 *
 *      Based on code provided by Jonathan Tarbox and Marcel de Kogel.
 *
 *      CH Flightstick Pro and Logitech Wingman Extreme
 *      support by Fabian Nunez.
 *
 *      Matthew Bowie added support for 4-button joysticks.
 *
 *      Richard Mitton added support for 6-button joysticks.
 *
 *      Stefan Eilert added support for dual joysticks.
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
static int joy_init(void); 
static void joy_exit(void); 
static int joy_poll(void);
static int joy_save_data(void);
static int joy_load_data(void);
static AL_CONST char *joy_calibrate_name(int n);
static int joy_calibrate(int n);

static int poll(int *x, int *y, int *x2, int *y2, int poll_mask);



#define JOYSTICK_DRIVER_CONTENTS       \
   joy_init,                           \
   joy_exit,                           \
   joy_poll,                           \
   joy_save_data,                      \
   joy_load_data,                      \
   joy_calibrate_name,                 \
   joy_calibrate



JOYSTICK_DRIVER joystick_standard =
{
   JOY_TYPE_STANDARD,
   empty_string,
   empty_string,
   "Standard joystick",
   JOYSTICK_DRIVER_CONTENTS
};



JOYSTICK_DRIVER joystick_2pads =
{
   JOY_TYPE_2PADS,
   empty_string,
   empty_string,
   "Dual joysticks",
   JOYSTICK_DRIVER_CONTENTS
};



JOYSTICK_DRIVER joystick_4button =
{
   JOY_TYPE_4BUTTON,
   empty_string,
   empty_string,
   "4-button joystick",
   JOYSTICK_DRIVER_CONTENTS
};



JOYSTICK_DRIVER joystick_6button =
{
   JOY_TYPE_6BUTTON,
   empty_string,
   empty_string,
   "6-button joystick",
   JOYSTICK_DRIVER_CONTENTS
};



JOYSTICK_DRIVER joystick_8button =
{
   JOY_TYPE_8BUTTON,
   empty_string,
   empty_string,
   "8-button joystick",
   JOYSTICK_DRIVER_CONTENTS
};



JOYSTICK_DRIVER joystick_fspro =
{
   JOY_TYPE_FSPRO,
   empty_string,
   empty_string,
   "Flightstick Pro",
   JOYSTICK_DRIVER_CONTENTS
};



JOYSTICK_DRIVER joystick_wingex =
{
   JOY_TYPE_WINGEX,
   empty_string,
   empty_string,
   "Wingman Extreme",
   JOYSTICK_DRIVER_CONTENTS
};



/* flags describing different variants of the basic joystick */
#define JDESC_STICK2             0x0001
#define JDESC_4BUTTON            0x0002
#define JDESC_6BUTTON            0x0004
#define JDESC_8BUTTON            0x0008
#define JDESC_Y2_THROTTLE        0x0010
#define JDESC_Y2_HAT             0x0020
#define JDESC_FSPRO_HAT          0x0040



/* calibration state information */
#define JOYSTICK_CALIB_TL1             0x00010000
#define JOYSTICK_CALIB_BR1             0x00020000
#define JOYSTICK_CALIB_TL2             0x00040000
#define JOYSTICK_CALIB_BR2             0x00080000
#define JOYSTICK_CALIB_THRTL_MIN       0x00100000
#define JOYSTICK_CALIB_THRTL_MAX       0x00200000
#define JOYSTICK_CALIB_HAT_CENTRE      0x00400000
#define JOYSTICK_CALIB_HAT_LEFT        0x00800000
#define JOYSTICK_CALIB_HAT_DOWN        0x01000000
#define JOYSTICK_CALIB_HAT_RIGHT       0x02000000
#define JOYSTICK_CALIB_HAT_UP          0x04000000

#define JOYSTICK_CALIB_HAT    (JOYSTICK_CALIB_HAT_CENTRE |     \
			       JOYSTICK_CALIB_HAT_LEFT |       \
			       JOYSTICK_CALIB_HAT_DOWN |       \
			       JOYSTICK_CALIB_HAT_RIGHT |      \
			       JOYSTICK_CALIB_HAT_UP)



/* driver state information */
static int joystick_flags = 0;

static int joycentre_x, joycentre_y;
static int joyx_min, joyx_low_margin, joyx_high_margin, joyx_max;
static int joyy_min, joyy_low_margin, joyy_high_margin, joyy_max;

static int joycentre2_x, joycentre2_y;
static int joyx2_min, joyx2_low_margin, joyx2_high_margin, joyx2_max;
static int joyy2_min, joyy2_low_margin, joyy2_high_margin, joyy2_max;

static int joy_thr_min, joy_thr_max;

static int joy_hat_pos[5], joy_hat_threshold[4];

static int joy_old_x, joy_old_y;
static int joy2_old_x, joy2_old_y;



/* masks indicating which axis to read */
#define MASK_1X      1
#define MASK_1Y      2
#define MASK_2X      4
#define MASK_2Y      8



/* poll_mask:
 *  Returns a mask indicating which axes to poll.
 */
static int poll_mask(void)
{
   int mask = MASK_1X | MASK_1Y;

   if (joystick_flags & (JDESC_STICK2 | JDESC_6BUTTON | JDESC_8BUTTON))
      mask |= (MASK_2X | MASK_2Y);

   if (joystick_flags & (JDESC_Y2_THROTTLE | JDESC_Y2_HAT))
      mask |= MASK_2Y;

   return mask;
}



/* averaged_poll:
 *  For calibration it is crucial that we get the right results, so we
 *  average out several attempts.
 */
static int averaged_poll(int *x, int *y, int *x2, int *y2, int mask)
{
   int x_tmp, y_tmp, x2_tmp, y2_tmp;
   int x_total, y_total, x2_total, y2_total;
   int c;

   #define AVERAGE_COUNT   4

   x_total = y_total = x2_total = y2_total = 0;

   for (c=0; c<AVERAGE_COUNT; c++) {
      if (poll(&x_tmp, &y_tmp, &x2_tmp, &y2_tmp, mask) != 0)
	 return -1;

      x_total += x_tmp;
      y_total += y_tmp;
      x2_total += x2_tmp;
      y2_total += y2_tmp;
   }

   *x = x_total / AVERAGE_COUNT;
   *y = y_total / AVERAGE_COUNT;
   *x2 = x2_total / AVERAGE_COUNT;
   *y2 = y2_total / AVERAGE_COUNT;

   return 0;
}



/* recalc_calibration_flags:
 *  Called after each calibration operation, to calculate what else might
 *  need to be measured for the current hardware.
 */
static void recalc_calibration_flags(void)
{
   #define FLAG_SET(n)  ((joystick_flags & (n)) == (n))

   /* stick 1 analogue input? */
   if (FLAG_SET(JOYSTICK_CALIB_TL1 | JOYSTICK_CALIB_BR1)) {
      joy[0].stick[0].flags |= JOYFLAG_ANALOGUE;
      joy[0].stick[0].flags &= ~JOYFLAG_CALIB_ANALOGUE;
   }
   else {
      joy[0].stick[0].flags &= ~JOYFLAG_ANALOGUE;
      joy[0].stick[0].flags |= JOYFLAG_CALIB_ANALOGUE;
   }

   /* stick 2 analogue input? */
   if (joystick_flags & JDESC_STICK2) {
      if (FLAG_SET(JOYSTICK_CALIB_TL2 | JOYSTICK_CALIB_BR2)) {
	 joy[1].stick[0].flags |= JOYFLAG_ANALOGUE;
	 joy[1].stick[0].flags &= ~JOYFLAG_CALIB_ANALOGUE;
      }
      else {
	 joy[1].stick[0].flags &= ~JOYFLAG_ANALOGUE;
	 joy[1].stick[0].flags |= JOYFLAG_CALIB_ANALOGUE;
      }
   }

   /* Wingman Extreme hat input? */
   if (joystick_flags & JDESC_Y2_HAT) {
      if (FLAG_SET(JOYSTICK_CALIB_HAT)) {
	 joy[0].stick[1].flags |= JOYFLAG_DIGITAL;
	 joy[0].stick[1].flags &= ~JOYFLAG_CALIB_DIGITAL;
      }
      else {
	 joy[0].stick[1].flags &= ~JOYFLAG_DIGITAL;
	 joy[0].stick[1].flags |= JOYFLAG_CALIB_DIGITAL;
      }
   }

   /* FSPro throttle input? */
   if (joystick_flags & JDESC_Y2_THROTTLE) {
      if (FLAG_SET(JOYSTICK_CALIB_THRTL_MIN | JOYSTICK_CALIB_THRTL_MAX)) {
	 joy[0].stick[2].flags |= JOYFLAG_ANALOGUE;
	 joy[0].stick[2].flags &= ~JOYFLAG_CALIB_ANALOGUE;
      }
      else {
	 joy[0].stick[2].flags &= ~JOYFLAG_ANALOGUE;
	 joy[0].stick[2].flags |= JOYFLAG_CALIB_ANALOGUE;
      }
   }
}



/* joy_init:
 *  Initialises the driver.
 */
static int joy_init(void)
{
   int i;

   /* store info about the stick type */
   switch (_joy_type) {

      case JOY_TYPE_STANDARD:
	 joystick_flags = 0;
	 break;

      case JOY_TYPE_2PADS:
	 joystick_flags = JDESC_STICK2;
	 break;

      case JOY_TYPE_4BUTTON:
	 joystick_flags = JDESC_4BUTTON;
	 break;

      case JOY_TYPE_6BUTTON:
	 joystick_flags = JDESC_4BUTTON | JDESC_6BUTTON;
	 break;

      case JOY_TYPE_8BUTTON:
	 joystick_flags = JDESC_4BUTTON | JDESC_8BUTTON;
	 break;

      case JOY_TYPE_FSPRO:
	 joystick_flags = JDESC_4BUTTON | JDESC_Y2_THROTTLE | JDESC_FSPRO_HAT;
	 break;

      case JOY_TYPE_WINGEX:
	 joystick_flags = JDESC_4BUTTON | JDESC_Y2_HAT;
	 break;

      default:
	 return -1;
   }

   joy_old_x = joy_old_y = 0;
   joy2_old_x = joy2_old_y = 0;

   /* make sure the hardware is really present */
   if (averaged_poll(&joycentre_x, &joycentre_y, &joycentre2_x, &joycentre2_y, poll_mask()) != 0)
      return -1;

   /* fill in the joystick structure */
   num_joysticks = (joystick_flags & JDESC_STICK2) ? 2 : 1;

   joy[0].flags = JOYFLAG_DIGITAL;

   /* how many buttons? */
   if (joystick_flags & JDESC_8BUTTON)
      joy[0].num_buttons = 8;
   else if (joystick_flags & JDESC_6BUTTON)
      joy[0].num_buttons = 6;
   else if (joystick_flags & JDESC_4BUTTON)
      joy[0].num_buttons = 4;
   else
      joy[0].num_buttons = 2;

   /* main analogue stick */
   joy[0].stick[0].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
   joy[0].stick[0].num_axis = 2;
   joy[0].stick[0].axis[0].name = get_config_text("X");
   joy[0].stick[0].axis[1].name = get_config_text("Y");
   joy[0].stick[0].name = get_config_text("Stick");
   joy[0].num_sticks = 1;

   /* hat control? */
   if (joystick_flags & (JDESC_FSPRO_HAT | JDESC_Y2_HAT)) {
      joy[0].stick[joy[0].num_sticks].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      joy[0].stick[joy[0].num_sticks].num_axis = 2;
      joy[0].stick[joy[0].num_sticks].axis[0].name = get_config_text("X");
      joy[0].stick[joy[0].num_sticks].axis[1].name = get_config_text("Y");
      joy[0].stick[joy[0].num_sticks].name = get_config_text("Hat");
      joy[0].num_sticks++;
   }

   /* throttle control? */
   if (joystick_flags & JDESC_Y2_THROTTLE) {
      joy[0].stick[joy[0].num_sticks].flags = JOYFLAG_UNSIGNED;
      joy[0].stick[joy[0].num_sticks].num_axis = 1;
      joy[0].stick[joy[0].num_sticks].axis[0].name = get_config_text("Throttle");
      joy[0].stick[joy[0].num_sticks].name = get_config_text("Throttle");
      joy[0].num_sticks++;
   }

   /* clone everything for a second joystick? */
   if (joystick_flags & JDESC_STICK2)
      joy[1] = joy[0];

   /* fill in the button names */
   for (i=0; i<2; i++) {
      joy[i].button[0].name = get_config_text("B1");
      joy[i].button[1].name = get_config_text("B2");

      if (joy[i].num_buttons > 2) {
	 joy[i].button[2].name = get_config_text("B3");
	 joy[i].button[3].name = get_config_text("B4");
      }

      if (joy[i].num_buttons > 4) {
	 joy[i].button[4].name = get_config_text("B5");
	 joy[i].button[5].name = get_config_text("B6");
      }

      if (joy[i].num_buttons > 6) {
	 joy[i].button[6].name = get_config_text("B7");
	 joy[i].button[7].name = get_config_text("B8");
      }
   }

   recalc_calibration_flags();

   return 0;
}



/* joy_exit:
 *  Shuts down the driver.
 */
static void joy_exit(void)
{
   joystick_flags = 0;
}



/* sort_out_middle_values:
 *  Sets up the values used by sort_out_analogue() to create a 'dead'
 *  region in the centre of the joystick range.
 */
static void sort_out_middle_values(int n)
{
   if (n == 0) {
      joyx_low_margin  = joycentre_x - (joycentre_x - joyx_min) / 8;
      joyx_high_margin = joycentre_x + (joyx_max - joycentre_x) / 8;
      joyy_low_margin  = joycentre_y - (joycentre_y - joyy_min) / 8;
      joyy_high_margin = joycentre_y + (joyy_max - joycentre_y) / 8;
   }
   else {
      joyx2_low_margin  = joycentre2_x - (joycentre2_x - joyx2_min) / 8;
      joyx2_high_margin = joycentre2_x + (joyx2_max - joycentre2_x) / 8;
      joyy2_low_margin  = joycentre2_y - (joycentre2_y - joyy2_min) / 8;
      joyy2_high_margin = joycentre2_y + (joyy2_max - joycentre2_y) / 8;
   }
}



/* calibrate_corner:
 *  For analogue access to the joystick, this is used to measure the top
 *  left and bottom right corner positions.
 */
static int calibrate_corner(int stick, int corner)
{
   int flag, other, ret, nowhere;

   if (stick == 0) {
      /* stick 1 calibration */
      flag = (corner) ? JOYSTICK_CALIB_BR1 : JOYSTICK_CALIB_TL1;
      other = (corner) ? JOYSTICK_CALIB_TL1 : JOYSTICK_CALIB_BR1;

      if (corner)
	 ret = averaged_poll(&joyx_max, &joyy_max, &nowhere, &nowhere, MASK_1X | MASK_1Y);
      else
	 ret = averaged_poll(&joyx_min, &joyy_min, &nowhere, &nowhere, MASK_1X | MASK_1Y);
   }
   else {
      /* stick 2 calibration */
      flag = (corner) ? JOYSTICK_CALIB_BR2 : JOYSTICK_CALIB_TL2;
      other = (corner) ? JOYSTICK_CALIB_TL2 : JOYSTICK_CALIB_BR2;

      if (corner)
	 ret = averaged_poll(&nowhere, &nowhere, &joyx2_max, &joyy2_max, MASK_2X | MASK_2Y);
      else
	 ret = averaged_poll(&nowhere, &nowhere, &joyx2_min, &joyy2_min, MASK_2X | MASK_2Y);
   }

   if (ret != 0) {
      joystick_flags &= ~flag;
      return -1;
   }

   joystick_flags |= flag;

   /* once we've done both corners, we are ready for full analogue input */
   if (joystick_flags & other) {
      sort_out_middle_values(stick);
      recalc_calibration_flags();
   }

   return 0;
}



/* calibrate_joystick_tl:
 *  For backward compatibility with the old API.
 */
int calibrate_joystick_tl(void)
{
   int ret;

   if (!_joystick_installed)
      return -1;

   ret = calibrate_corner(0, 0);

   if ((ret == 0) && (joystick_flags & JDESC_STICK2))
      ret = calibrate_corner(1, 0);

   return ret;
}



/* calibrate_joystick_br:
 *  For backward compatibility with the old API.
 */
int calibrate_joystick_br(void)
{
   int ret;

   if (!_joystick_installed)
      return -1;

   ret = calibrate_corner(0, 1);

   if ((ret == 0) && (joystick_flags & JDESC_STICK2))
      ret = calibrate_corner(1, 1);

   return ret;
}



/* calibrate_joystick_throttle_min:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "minimum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_max().
 */
int calibrate_joystick_throttle_min(void)
{
   int dummy;

   if (!_joystick_installed)
      return -1;

   if (averaged_poll(&dummy, &dummy, &dummy, &joy_thr_min, MASK_2Y) != 0)
      return -1;

   /* prevent division by zero errors if user miscalibrated */
   if ((joystick_flags & JOYSTICK_CALIB_THRTL_MAX) && (joy_thr_min == joy_thr_max))
     joy_thr_min = 255 - joy_thr_max;

   joystick_flags |= JOYSTICK_CALIB_THRTL_MIN;
   recalc_calibration_flags();

   return 0;
}



/* calibrate_joystick_throttle_max:
 *  For analogue access to the FSPro's throttle, call this after 
 *  initialise_joystick(), with the throttle at the "maximum" extreme
 *  (the user decides whether this is all the way forwards or all the
 *  way back), and also call calibrate_joystick_throttle_min().
 */
int calibrate_joystick_throttle_max(void)
{
   int dummy;

   if (!_joystick_installed)
      return -1;

   if (averaged_poll(&dummy, &dummy, &dummy, &joy_thr_max, MASK_2Y) != 0)
      return -1;

   /* prevent division by zero errors if user miscalibrated */
   if ((joystick_flags & JOYSTICK_CALIB_THRTL_MIN) && (joy_thr_min == joy_thr_max))
     joy_thr_max = 255 - joy_thr_min;

   joystick_flags |= JOYSTICK_CALIB_THRTL_MAX;
   recalc_calibration_flags();

   return 0;
}



/* calibrate_joystick_hat:
 *  For access to the Wingman Extreme's hat (I think this will work on all 
 *  Thrustmaster compatible joysticks), call this after initialise_joystick(), 
 *  passing the JOY_HAT constant you wish to calibrate.
 */
int calibrate_joystick_hat(int direction)
{
   static int pos_const[] = 
   { 
      JOYSTICK_CALIB_HAT_CENTRE,
      JOYSTICK_CALIB_HAT_LEFT,
      JOYSTICK_CALIB_HAT_DOWN,
      JOYSTICK_CALIB_HAT_RIGHT,
      JOYSTICK_CALIB_HAT_UP
   };

   int dummy, value;

   if ((direction > JOY_HAT_UP) || (!_joystick_installed))
      return -1;

   if (averaged_poll(&dummy, &dummy, &dummy, &value, MASK_2Y) != 0)
      return -1;

   joy_hat_pos[direction] = value;
   joystick_flags |= pos_const[direction];

   /* when all directions have been calibrated, calculate deltas */
   if ((joystick_flags & JOYSTICK_CALIB_HAT) == JOYSTICK_CALIB_HAT) {
      joy_hat_threshold[0] = (joy_hat_pos[0] + joy_hat_pos[1]) / 2;
      joy_hat_threshold[1] = (joy_hat_pos[1] + joy_hat_pos[2]) / 2;
      joy_hat_threshold[2] = (joy_hat_pos[2] + joy_hat_pos[3]) / 2;
      joy_hat_threshold[3] = (joy_hat_pos[3] + joy_hat_pos[4]) / 2;

      recalc_calibration_flags();
   }

   return 0;
}



/* sort_out_analogue:
 *  There are a couple of problems with reading analogue input from the PC
 *  joystick. For one thing, joysticks tend not to centre repeatably, so
 *  we need a small 'dead' zone in the middle. Also a lot of joysticks aren't
 *  linear, so the positions less than centre need to be handled differently
 *  to those above the centre.
 */
static int sort_out_analogue(int x, int min, int low_margin, int high_margin, int max)
{
   if (x < min) {
      return -128;
   }
   else if (x < low_margin) {
      return -128 + (x - min) * 128 / (low_margin - min);
   }
   else if (x <= high_margin) {
      return 0;
   }
   else if (x <= max) {
      return 128 - (max - x) * 128 / (max - high_margin);
   }
   else
      return 128;
}



/* joy_poll:
 *  Updates the joystick status variables.
 */
static int joy_poll(void)
{
   int x, y, x2, y2, i;
   unsigned char status;

   /* read the hardware */
   if (poll(&x, &y, &x2, &y2, poll_mask()) != 0)
      return -1;

   status = inportb(0x201);

   /* stick 1 position */
   if ((ABS(x-joy_old_x) <= x/4) && (ABS(y-joy_old_y) <= y/4)) {
      if ((joystick_flags & JOYSTICK_CALIB_TL1) && (joystick_flags & JOYSTICK_CALIB_BR1)) {
	 joy[0].stick[0].axis[0].pos = sort_out_analogue(x, joyx_min, joyx_low_margin, joyx_high_margin, joyx_max);
	 joy[0].stick[0].axis[1].pos = sort_out_analogue(y, joyy_min, joyy_low_margin, joyy_high_margin, joyy_max);
      }
      else {
	 joy[0].stick[0].axis[0].pos = x - joycentre_x;
	 joy[0].stick[0].axis[1].pos = y - joycentre_y;
      }

      joy[0].stick[0].axis[0].d1 = (x < (joycentre_x/2));
      joy[0].stick[0].axis[0].d2 = (x > (joycentre_x*4/3));
      joy[0].stick[0].axis[1].d1 = (y < (joycentre_y/2));
      joy[0].stick[0].axis[1].d2 = (y > (joycentre_y*4/3));
   }

   if (joystick_flags & JDESC_STICK2) {
      /* stick 2 position */
      if ((ABS(x2-joy2_old_x) <= x2/4) && (ABS(y2-joy2_old_y) <= y2/4)) {
	 if ((joystick_flags & JOYSTICK_CALIB_TL2) && (joystick_flags & JOYSTICK_CALIB_BR2)) {
	    joy[1].stick[0].axis[0].pos = sort_out_analogue(x2, joyx2_min, joyx2_low_margin, joyx2_high_margin, joyx2_max);
	    joy[1].stick[0].axis[1].pos = sort_out_analogue(y2, joyy2_min, joyy2_low_margin, joyy2_high_margin, joyy2_max);
	 }
	 else {
	    joy[1].stick[0].axis[0].pos = x2 - joycentre2_x;
	    joy[1].stick[0].axis[1].pos = y2 - joycentre2_y;
	 }

	 joy[1].stick[0].axis[0].d1 = (x2 < (joycentre2_x/2));
	 joy[1].stick[0].axis[0].d2 = (x2 > (joycentre2_x*3/2));
	 joy[1].stick[0].axis[1].d1 = (y2 < (joycentre2_y/2));
	 joy[1].stick[0].axis[1].d2 = (y2 > (joycentre2_y*3/2));
      }

      joy[1].button[0].b = ((status & 0x40) == 0);
      joy[1].button[1].b = ((status & 0x80) == 0);
   }

   /* button state */
   joy[0].button[0].b = ((status & 0x10) == 0);
   joy[0].button[1].b = ((status & 0x20) == 0);

   if (joystick_flags & JDESC_4BUTTON) {
      /* four button state */
      joy[0].button[2].b = ((status & 0x40) == 0);
      joy[0].button[3].b = ((status & 0x80) == 0);
   }

   if (joystick_flags & JDESC_6BUTTON) {
      /* six button state */
      joy[0].button[4].b = (x2 < 128);
      joy[0].button[5].b = (y2 < 128);
   }
   else if (joystick_flags & JDESC_8BUTTON) {
      /* eight button state */
      joy[0].button[4].b = (x2 < (joycentre2_x/2));
      joy[0].button[5].b = (y2 < (joycentre2_y/2));
      joy[0].button[6].b = (x2 > (joycentre2_x*3/2));
      joy[0].button[7].b = (y2 > (joycentre2_y*3)/2);
   }

   if (joystick_flags & JDESC_Y2_THROTTLE) {
      /* throttle */
      if ((joystick_flags & JOYSTICK_CALIB_THRTL_MIN) && (joystick_flags & JOYSTICK_CALIB_THRTL_MAX)) {
	 i = (y2 - joy_thr_min) * 255 / (joy_thr_max - joy_thr_min);
	 joy[0].stick[2].axis[0].pos = CLAMP(0, i, 255);
	 if (joy[0].stick[2].axis[0].pos > 255*2/3)
	    joy[0].stick[2].axis[0].d2 = TRUE;
	 else
	    joy[0].stick[2].axis[0].d2 = FALSE;
      } 
   }

   if (joystick_flags & JDESC_FSPRO_HAT) {
      /* FSPro hat control (accessed via special button values) */
      joy[0].stick[1].axis[0].pos = 0;
      joy[0].stick[1].axis[1].pos = 0;
      joy[0].stick[1].axis[0].d1 = joy[0].stick[1].axis[0].d2 = 0;
      joy[0].stick[1].axis[1].d1 = joy[0].stick[1].axis[1].d2 = 0;

      if ((status & 0x30) == 0) {
	 joy[0].button[0].b = FALSE;
	 joy[0].button[1].b = FALSE;
	 joy[0].button[2].b = FALSE;
	 joy[0].button[3].b = FALSE;

	 switch (status & 0xC0) {

	    case 0x00:
	       /* up */
	       joy[0].stick[1].axis[1].pos = -128;
	       joy[0].stick[1].axis[1].d1 = TRUE;
	       break;

	    case 0x40:
	       /* right */
	       joy[0].stick[1].axis[0].pos = 128;
	       joy[0].stick[1].axis[0].d2 = TRUE;
	       break;

	    case 0x80:
	       /* down */
	       joy[0].stick[1].axis[1].pos = 128;
	       joy[0].stick[1].axis[1].d2 = TRUE;
	       break;

	    case 0xC0:
	       /* left */
	       joy[0].stick[1].axis[0].pos = -128;
	       joy[0].stick[1].axis[0].d1 = TRUE;
	       break;
	 }
      }
   }

   if (joystick_flags & JDESC_Y2_HAT) {
      /* Wingman Extreme hat control (accessed via the y2 pot) */
      joy[0].stick[1].axis[0].pos = 0;
      joy[0].stick[1].axis[1].pos = 0;
      joy[0].stick[1].axis[0].d1 = joy[0].stick[1].axis[0].d2 = 0;
      joy[0].stick[1].axis[1].d1 = joy[0].stick[1].axis[1].d2 = 0;

      if ((joystick_flags & JOYSTICK_CALIB_HAT) == JOYSTICK_CALIB_HAT) {
	 if (y2 >= joy_hat_threshold[0]) {
	    /* centre */
	 }
	 else if (y2 >= joy_hat_threshold[1]) {
	    /* left */
	    joy[0].stick[1].axis[0].pos = -128;
	    joy[0].stick[1].axis[0].d1 = TRUE;
	 }
	 else if (y2 >= joy_hat_threshold[2]) {
	    /* down */
	    joy[0].stick[1].axis[1].pos = 128;
	    joy[0].stick[1].axis[1].d2 = TRUE;
	 }
	 else if (y2 >= joy_hat_threshold[3]) {
	    /* right */
	    joy[0].stick[1].axis[0].pos = 128;
	    joy[0].stick[1].axis[0].d2 = TRUE;
	 }
	 else {
	    /* up */
	    joy[0].stick[1].axis[1].pos = -128;
	    joy[0].stick[1].axis[1].d1 = TRUE;
	 }
      } 
   } 

   joy_old_x = x;
   joy_old_y = y;

   joy2_old_x = x2;
   joy2_old_y = y2;

   return 0;
}



/* poll:
 *  Polls the joystick to read the axis position. Returns raw position
 *  values, zero for success, non-zero if no joystick found. This has
 *  to come after the routines that call it, to make sure it will never
 *  be inlined (that could shift alignment and upset the timing values).
 */
static int poll(int *x, int *y, int *x2, int *y2, int poll_mask)
{
   int c, d;

   *x = *y = *x2 = *y2 = 0;

   _enter_critical();

   outportb(0x201, 0);

   for (c=0; c<100000; c++) {
      d = inportb(0x201);

      if (d & MASK_1X)
	 (*x)++;

      if (d & MASK_1Y)
	 (*y)++;

      if (d & MASK_2X)
	 (*x2)++;

      if (d & MASK_2Y)
	 (*y2)++;

      if (!(d & poll_mask))
	 break;
   } 

   _exit_critical();

   if (((poll_mask & MASK_1X) && (*x >= 100000)) ||
       ((poll_mask & MASK_1Y) && (*y >= 100000)) ||
       ((poll_mask & MASK_2X) && (*x2 >= 100000)) ||
       ((poll_mask & MASK_2Y) && (*y2 >= 100000)))
      return -1;

   return 0;
}



/* joy_save_data:
 *  Writes calibration data into the config file.
 */
static int joy_save_data(void)
{
   char tmp1[128], tmp2[128];
   char *j = uconvert_ascii("joystick", tmp1);

   set_config_int(j, uconvert_ascii("joystick_flags", tmp2),    joystick_flags);

   set_config_int(j, uconvert_ascii("joycentre_x", tmp2),       joycentre_x);
   set_config_int(j, uconvert_ascii("joycentre_y", tmp2),       joycentre_y);
   set_config_int(j, uconvert_ascii("joyx_min", tmp2),          joyx_min);
   set_config_int(j, uconvert_ascii("joyx_low_margin", tmp2),   joyx_low_margin);
   set_config_int(j, uconvert_ascii("joyx_high_margin", tmp2),  joyx_high_margin);
   set_config_int(j, uconvert_ascii("joyx_max", tmp2),          joyx_max);
   set_config_int(j, uconvert_ascii("joyy_min", tmp2),          joyy_min);
   set_config_int(j, uconvert_ascii("joyy_low_margin", tmp2),   joyy_low_margin);
   set_config_int(j, uconvert_ascii("joyy_high_margin", tmp2),  joyy_high_margin);
   set_config_int(j, uconvert_ascii("joyy_max", tmp2),          joyy_max);

   set_config_int(j, uconvert_ascii("joycentre2_x", tmp2),      joycentre2_x);
   set_config_int(j, uconvert_ascii("joycentre2_y", tmp2),      joycentre2_y);
   set_config_int(j, uconvert_ascii("joyx2_min", tmp2),         joyx2_min);
   set_config_int(j, uconvert_ascii("joyx2_low_margin", tmp2),  joyx2_low_margin);
   set_config_int(j, uconvert_ascii("joyx2_high_margin", tmp2), joyx2_high_margin);
   set_config_int(j, uconvert_ascii("joyx2_max", tmp2),         joyx2_max);
   set_config_int(j, uconvert_ascii("joyy2_min", tmp2),         joyy2_min);
   set_config_int(j, uconvert_ascii("joyy2_low_margin", tmp2),  joyy2_low_margin);
   set_config_int(j, uconvert_ascii("joyy2_high_margin", tmp2), joyy2_high_margin);
   set_config_int(j, uconvert_ascii("joyy2_max", tmp2),         joyy2_max);

   set_config_int(j, uconvert_ascii("joythr_min", tmp2),        joy_thr_min);
   set_config_int(j, uconvert_ascii("joythr_max", tmp2),        joy_thr_max);

   set_config_int(j, uconvert_ascii("joyhat_0", tmp2),          joy_hat_threshold[0]);
   set_config_int(j, uconvert_ascii("joyhat_1", tmp2),          joy_hat_threshold[1]);
   set_config_int(j, uconvert_ascii("joyhat_2", tmp2),          joy_hat_threshold[2]);
   set_config_int(j, uconvert_ascii("joyhat_3", tmp2),          joy_hat_threshold[3]);

   return 0;
}



/* joy_load_data:
 *  Loads calibration data from the config file.
 */
static int joy_load_data(void)
{
   char tmp1[128], tmp2[128];
   char *j = uconvert_ascii("joystick", tmp1);

   joystick_flags    = get_config_int(j, uconvert_ascii("joystick_flags", tmp2), joystick_flags);

   joycentre_x       = get_config_int(j, uconvert_ascii("joycentre_x", tmp2), joycentre_x);
   joycentre_y       = get_config_int(j, uconvert_ascii("joycentre_y", tmp2), joycentre_y);
   joyx_min          = get_config_int(j, uconvert_ascii("joyx_min", tmp2), joyx_min);
   joyx_low_margin   = get_config_int(j, uconvert_ascii("joyx_low_margin", tmp2), joyx_low_margin);
   joyx_high_margin  = get_config_int(j, uconvert_ascii("joyx_high_margin", tmp2), joyx_high_margin);
   joyx_max          = get_config_int(j, uconvert_ascii("joyx_max", tmp2), joyx_max);
   joyy_min          = get_config_int(j, uconvert_ascii("joyy_min", tmp2), joyy_min);
   joyy_low_margin   = get_config_int(j, uconvert_ascii("joyy_low_margin", tmp2), joyy_low_margin);
   joyy_high_margin  = get_config_int(j, uconvert_ascii("joyy_high_margin", tmp2), joyy_high_margin);
   joyy_max          = get_config_int(j, uconvert_ascii("joyy_max", tmp2), joyy_max);

   joycentre2_x      = get_config_int(j, uconvert_ascii("joycentre2_x", tmp2), joycentre2_x);
   joycentre2_y      = get_config_int(j, uconvert_ascii("joycentre2_y", tmp2), joycentre2_y);
   joyx2_min         = get_config_int(j, uconvert_ascii("joyx2_min", tmp2), joyx2_min);
   joyx2_low_margin  = get_config_int(j, uconvert_ascii("joyx_low2_margin", tmp2), joyx2_low_margin);
   joyx2_high_margin = get_config_int(j, uconvert_ascii("joyx_high2_margin", tmp2), joyx2_high_margin);
   joyx2_max         = get_config_int(j, uconvert_ascii("joyx2_max", tmp2), joyx2_max);
   joyy2_min         = get_config_int(j, uconvert_ascii("joyy2_min", tmp2), joyy2_min);
   joyy2_low_margin  = get_config_int(j, uconvert_ascii("joyy2_low_margin", tmp2), joyy2_low_margin);
   joyy2_high_margin = get_config_int(j, uconvert_ascii("joyy2_high_margin", tmp2), joyy2_high_margin);
   joyy2_max         = get_config_int(j, uconvert_ascii("joyy2_max", tmp2), joyy2_max);

   joy_thr_min       = get_config_int(j, uconvert_ascii("joythr_min", tmp2), joy_thr_min);
   joy_thr_max       = get_config_int(j, uconvert_ascii("joythr_max", tmp2), joy_thr_max);

   joy_hat_threshold[0] = get_config_int(j, uconvert_ascii("joyhat_0", tmp2), joy_hat_threshold[0]);
   joy_hat_threshold[1] = get_config_int(j, uconvert_ascii("joyhat_1", tmp2), joy_hat_threshold[1]);
   joy_hat_threshold[2] = get_config_int(j, uconvert_ascii("joyhat_2", tmp2), joy_hat_threshold[2]);
   joy_hat_threshold[3] = get_config_int(j, uconvert_ascii("joyhat_3", tmp2), joy_hat_threshold[3]);

   joy_old_x = joy_old_y = 0;

   recalc_calibration_flags();

   return 0;
}



/* next_calib_action:
 *  Returns a flag indicating the next thing that needs to be calibrated.
 */
static int next_calib_action(int stick)
{
   if (stick == 0) {
      /* stick 1 analogue input? */
      if (!(joystick_flags & JOYSTICK_CALIB_TL1))
	 return JOYSTICK_CALIB_TL1;

      if (!(joystick_flags & JOYSTICK_CALIB_BR1))
	 return JOYSTICK_CALIB_BR1;

      /* FSPro throttle input? */
      if (joystick_flags & JDESC_Y2_THROTTLE) {
	 if (!(joystick_flags & JOYSTICK_CALIB_THRTL_MIN))
	    return JOYSTICK_CALIB_THRTL_MIN;

	 if (!(joystick_flags & JOYSTICK_CALIB_THRTL_MAX))
	    return JOYSTICK_CALIB_THRTL_MAX;
      }

      /* Wingman Extreme hat input? */
      if (joystick_flags & JDESC_Y2_HAT) {
	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_CENTRE))
	    return JOYSTICK_CALIB_HAT_CENTRE;

	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_LEFT))
	    return JOYSTICK_CALIB_HAT_LEFT;

	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_DOWN))
	    return JOYSTICK_CALIB_HAT_DOWN;

	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_RIGHT))
	    return JOYSTICK_CALIB_HAT_RIGHT;

	 if (!(joystick_flags & JOYSTICK_CALIB_HAT_UP))
	    return JOYSTICK_CALIB_HAT_UP;
      }
   }
   else if (stick == 1) {
      if (joystick_flags & JDESC_STICK2) {
	 /* stick 2 analogue input? */
	 if (!(joystick_flags & JOYSTICK_CALIB_TL2))
	    return JOYSTICK_CALIB_TL2;

	 if (!(joystick_flags & JOYSTICK_CALIB_BR2))
	    return JOYSTICK_CALIB_BR2;
      }
   }

   return 0;
}



/* joy_calibrate_name:
 *  Returns the name of the next calibration operation.
 */
static AL_CONST char *joy_calibrate_name(int n)
{
   switch (next_calib_action(n)) {

      case JOYSTICK_CALIB_TL1:         return get_config_text("Move stick to the top left");         break;
      case JOYSTICK_CALIB_BR1:         return get_config_text("Move stick to the bottom right");     break;
      case JOYSTICK_CALIB_TL2:         return get_config_text("Move stick 2 to the top left");       break;
      case JOYSTICK_CALIB_BR2:         return get_config_text("Move stick 2 to the bottom right");   break;
      case JOYSTICK_CALIB_THRTL_MIN:   return get_config_text("Set throttle to minimum");            break;
      case JOYSTICK_CALIB_THRTL_MAX:   return get_config_text("Set throttle to maximum");            break;
      case JOYSTICK_CALIB_HAT_CENTRE:  return get_config_text("Centre the hat");                     break;
      case JOYSTICK_CALIB_HAT_LEFT:    return get_config_text("Move the hat left");                  break;
      case JOYSTICK_CALIB_HAT_DOWN:    return get_config_text("Move the hat down");                  break;
      case JOYSTICK_CALIB_HAT_RIGHT:   return get_config_text("Move the hat right");                 break;
      case JOYSTICK_CALIB_HAT_UP:      return get_config_text("Move the hat up");                    break;
   }

   return NULL;
}



/* joy_calibrate:
 *  Performs the next calibration operation.
 */
static int joy_calibrate(int n)
{
   switch (next_calib_action(n)) {

      case JOYSTICK_CALIB_TL1:
	 return calibrate_corner(0, 0);
	 break;

      case JOYSTICK_CALIB_BR1:
	 return calibrate_corner(0, 1);
	 break;

      case JOYSTICK_CALIB_TL2:
	 return calibrate_corner(1, 0);
	 break;

      case JOYSTICK_CALIB_BR2:
	 return calibrate_corner(1, 1);
	 break;

      case JOYSTICK_CALIB_THRTL_MIN:
	 return calibrate_joystick_throttle_min();
	 break;

      case JOYSTICK_CALIB_THRTL_MAX:
	 return calibrate_joystick_throttle_max();
	 break;

      case JOYSTICK_CALIB_HAT_CENTRE:
	 return calibrate_joystick_hat(JOY_HAT_CENTRE);
	 break;

      case JOYSTICK_CALIB_HAT_LEFT:
	 return calibrate_joystick_hat(JOY_HAT_LEFT);
	 break;

      case JOYSTICK_CALIB_HAT_DOWN:
	 return calibrate_joystick_hat(JOY_HAT_DOWN);
	 break;

      case JOYSTICK_CALIB_HAT_RIGHT:
	 return calibrate_joystick_hat(JOY_HAT_RIGHT);
	 break;

      case JOYSTICK_CALIB_HAT_UP:
	 return calibrate_joystick_hat(JOY_HAT_UP);
	 break;
   }

   return -1;
}


