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
 *      Joystick driver for the Gravis GrIP.
 *      (Xterminator, GamepadPro, Stinger, ?)
 *
 *      By Robert J. Ragno, using the Gravis SDK and the other
 *      Allegro drivers as references.
 *
 *      TODO: 
 *       - Test on more devices
 *       - Add support for restricting joystick to 4-directions
 *       - Look into dropping the grip.gll support (with asm code)
 *       - Look into providing direction names, not just axis names
 *       - (analog detection)
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"
#include "grip.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* If this is true, throttles will be 'packed' to fit in the
 * smallest number of pseudo-sticks possible.
 */
#define PACK_THROTTLES     1


/* enable support for each type of control */
#define ENABLE_AXIS        1
#define ENABLE_ANALOG      1
#define ENABLE_POV         1
#define ENABLE_THROTTLE    1


/* driver functions */
static int grip_init(void);
static void grip_exit(void);
static int grip_poll(void);
static int grip_link(void);
static int grip_unlink(void);
static int read_grip(int joy_num);


/* hack to simplify axis names... */
#define SIMPLIFY_AXIS(foo)                                           \
   if (((foo[0] == 'X') || (foo[0] == 'Y')) && (foo[1] == '_'))      \
      foo[1] = '\0';


/* internal variables for extra joystick data */
static GRIP_SLOT slot_index[MAX_JOYSTICKS];
static GRIP_VALUE max_value[MAX_JOYSTICKS][(int)GRIP_CLASS_MAX+1];
static int throttle_button_start[MAX_JOYSTICKS];
static GRIP_CLASS stick_class[MAX_JOYSTICKS][MAX_JOYSTICK_STICKS];
static int fourway[MAX_JOYSTICKS][MAX_JOYSTICK_STICKS];



/* the normal GrIP driver */
JOYSTICK_DRIVER joystick_grip =
{
   JOY_TYPE_GRIP,
   empty_string,
   empty_string,
   "GrIP",
   grip_init,
   grip_exit,
   grip_poll,
   NULL, NULL,                /* no saving/loading data */
   NULL, NULL                 /* no calibrating */
};



/* only allows 4-way movement on digital pad */
JOYSTICK_DRIVER joystick_grip4 =
{
   JOY_TYPE_GRIP4,
   empty_string,
   empty_string,
   "GrIP 4-way",
   grip_init,
   grip_exit,
   grip_poll,
   NULL, NULL,                /* no saving/loading data */
   NULL, NULL                 /* no calibrating */
};



/* read_grip;
 *  Reads the GrIP controller pointed to by joy_num (0 or 1) and 
 *  sets its state; no useful return value currently.
 */
static int read_grip(int j)
{
   GRIP_BITFIELD buttons_packed;
   GRIP_SLOT slot_num;
   GRIP_CLASS last_class;
   GRIP_INDEX axis;
   int a, b, s, state;

   slot_num = slot_index[j];

   /* sticks must be grouped by type for axis grouping to work! */
   axis = 0;
   last_class = stick_class[j][0];

   for (s=0; s<joy[j].num_sticks; s++) {
      int reverse_factor;     /* messy way to account for up/down reversal */

      if ((_joy_type == JOY_TYPE_GRIP4) &&
	  (stick_class[j][s] == GRIP_CLASS_AXIS) &&
	  (joy[j].stick[s].num_axis == 2)) {

	 fourway[j][s] = (joy[j].stick[s].axis[0].d1) +
			 (joy[j].stick[s].axis[0].d2 << 1) +
			 (joy[j].stick[s].axis[1].d1 << 2) +
			 (joy[j].stick[s].axis[1].d2 << 3);
      }

      /* reset axis count for new stick type */
      if (stick_class[j][s] != last_class)
	 axis = 0;

      last_class = stick_class[j][s];

      for (a=0; a<joy[j].stick[s].num_axis; a++) {
	 if ((a == 1) && (stick_class[j][s] != GRIP_CLASS_THROTTLE))
	    reverse_factor = -1;
	 else
	    reverse_factor = 1;

	 state = _GrGetValue(slot_num, stick_class[j][s], axis);
	 joy[j].stick[s].axis[a].pos = state * 256 / max_value[j][(int)stick_class[j][s]] - 128;
	 joy[j].stick[s].axis[a].pos *= reverse_factor;

	 /* don't trigger down direction if throttle */
	 if (stick_class[j][s] != GRIP_CLASS_THROTTLE)
	    joy[j].stick[s].axis[a].d1 = joy[j].stick[s].axis[a].pos < -64;

	 joy[j].stick[s].axis[a].d2 = joy[j].stick[s].axis[a].pos > 64;
	 axis++;
      }
   }

   /* handle 4-way restriction */
   if (_joy_type == JOY_TYPE_GRIP4) {
      for (s=0; s<joy[j].num_sticks; s++) {
	 if ((stick_class[j][s] != GRIP_CLASS_AXIS) || 
	     (joy[j].stick[s].num_axis != 2))
	    continue;

	 if ((fourway[j][s] & 1) && (joy[j].stick[s].axis[0].d1)) {
	    joy[j].stick[s].axis[0].d2 = 0;
	    joy[j].stick[s].axis[1].d1 = 0;
	    joy[j].stick[s].axis[1].d2 = 0;
	 }

	 if ((fourway[j][s] & 2) && (joy[j].stick[s].axis[0].d2)) {
	    joy[j].stick[s].axis[0].d1 = 0;
	    joy[j].stick[s].axis[1].d1 = 0;
	    joy[j].stick[s].axis[1].d2 = 0;
	 }

	 if ((fourway[j][s] & 4) && (joy[j].stick[s].axis[1].d1)) {
	    joy[j].stick[s].axis[1].d2 = 0;
	    joy[j].stick[s].axis[0].d1 = 0;
	    joy[j].stick[s].axis[0].d2 = 0;
	 }

	 if ((fourway[j][s] & 8) && (joy[j].stick[s].axis[1].d2)) {
	    joy[j].stick[s].axis[1].d1 = 0;
	    joy[j].stick[s].axis[0].d1 = 0;
	    joy[j].stick[s].axis[0].d2 = 0;
	 }
      }
   }

   /* read buttons */
   buttons_packed = _GrGetPackedValues(slot_num, GRIP_CLASS_BUTTON,
				       0, throttle_button_start[j]-1);

   for (b=0; b<throttle_button_start[j]; b++)
      joy[j].button[b].b = !(!(buttons_packed & (1 << b)));

   return 0;
}



/* grip_init:
 *  Initializes the driver.
 */
static int grip_init(void)
{
   static char *name_b[] =
   {
      "L", "R", "A", "B", "C", "D", "E", "F", "Select", "Start",
      "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9",
      "B10", "B11", "B12", "B13", "B14", "B15", "B16", "B17", "B18"
   };

   char tmpstr[1287];
   GRIP_BITFIELD slots = 0;
   int i = 0;

   if (grip_link()) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Could not find GRIP.GLL"));
      return -1;
   }

   if (!_GrInitialize()) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Could not initialize GrIP system"));
      grip_unlink();
      return -1;
   }

   _GrRefresh(0);

   num_joysticks = 0;
   slots = _GrGetSlotMap();

   /* fill slot_index with the slot number of each joystick found */
   for (i=1; i<=12; i++) {
      if (slots & (1 << i)) {
	 slot_index[num_joysticks] = i;
	 num_joysticks++;
	 if (num_joysticks >= MAX_JOYSTICKS)
	    break;
      }
   }

   if (num_joysticks == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("No GrIP devices available"));
      grip_exit();
      return -1;
   }

   /* loop over each joystick (active GrIP slot) */
   for (i=0; i<num_joysticks; i++) {
      GRIP_BITFIELD class_map = 0;

      joy[i].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
      class_map = _GrGetClassMap(slot_index[i]);

      /* Standard GrIP features: */
      /* AXIS ** POV HAT ** VELOCITY ** (ANALOG) ** (THROTTLE) ** BUTTONS */

      joy[i].num_sticks = 0;

      /* check for an axis... (digital control) */
      if ((ENABLE_AXIS) && (class_map & (1 << GRIP_CLASS_AXIS))) {
	 int last_axis = 0;
	 int stick = 0;

	 last_axis = _GrGetMaxIndex(slot_index[i], GRIP_CLASS_AXIS);
	 joy[i].num_sticks = last_axis/2 + 1;
	 if (joy[i].num_sticks > MAX_JOYSTICK_STICKS) {
	    joy[i].num_sticks = MAX_JOYSTICK_STICKS;
	    last_axis = MAX_JOYSTICK_STICKS*2;
	 }

	 max_value[i][GRIP_CLASS_AXIS] = _GrGetMaxValue(slot_index[i], GRIP_CLASS_AXIS);

	 /* loop over each actual stick on the device (often just 1)
	  * We arbitrarily split the axes up into pairs... GrIP gives no
	  * info on how this can be identified
	  */
	 for (stick=0; stick < joy[i].num_sticks; stick++) {
	    stick_class[i][stick] = GRIP_CLASS_AXIS;

	    /* name stick... :( */
	    _al_sane_strncpy(tmpstr, "Stick", sizeof(tmpstr));
	    if (joy[i].num_sticks > 1) {
	       tmpstr[strlen(tmpstr)+2] = '\0';
	       tmpstr[strlen(tmpstr)+1] = '1'+stick;
	       tmpstr[strlen(tmpstr)] = ' ';
	    }

	    joy[i].stick[stick].name = get_config_text(tmpstr);
	    joy[i].stick[stick].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;

	    /* name axes */
	    joy[i].stick[stick].num_axis = ((stick*2) == last_axis) ? 1 : 2;
	    _GrGetControlName(slot_index[i], GRIP_CLASS_AXIS, stick*2, tmpstr);
	    if (tmpstr[0] == '\0')
	       _al_sane_strncpy(tmpstr, "X", sizeof(tmpstr));

	    SIMPLIFY_AXIS(tmpstr);

	    joy[i].stick[stick].axis[0].name = get_config_text(tmpstr);
	    joy[i].stick[stick].axis[0].pos = 0;
	    joy[i].stick[stick].axis[0].d1 = 0;
	    joy[i].stick[stick].axis[0].d2 = 0;

	    if (joy[i].stick[stick].num_axis == 2) {
	       _GrGetControlName(slot_index[i], GRIP_CLASS_AXIS, stick*2 + 1, tmpstr);
	       if (tmpstr[0] == '\0')
		  _al_sane_strncpy(tmpstr, "Y", sizeof(tmpstr));

	       SIMPLIFY_AXIS(tmpstr);

	       joy[i].stick[stick].axis[1].name = get_config_text(tmpstr);
	       joy[i].stick[stick].axis[1].pos = 0;
	       joy[i].stick[stick].axis[1].d1 = 0;
	       joy[i].stick[stick].axis[1].d2 = 0;
	    }
	 }
      }

      /* check for Analog pads (as in the Xterminator) */
      if ((ENABLE_ANALOG) &&
	  (class_map & (1 << GRIP_CLASS_ANALOG)) &&
	  (_GrGetMaxIndex(slot_index[i], GRIP_CLASS_ANALOG) > 0)) {
	 int num_pads = 0;
	 int stick = 0;
	 int analog_start;

	 max_value[i][GRIP_CLASS_ANALOG] = _GrGetMaxValue(slot_index[i], GRIP_CLASS_ANALOG);
	 num_pads = (_GrGetMaxIndex(slot_index[i], GRIP_CLASS_ANALOG) + 1) / 2;
	 analog_start = joy[i].num_sticks;
	 joy[i].num_sticks += num_pads;
	 if (joy[i].num_sticks > MAX_JOYSTICK_STICKS) {
	    num_pads -= joy[i].num_sticks - MAX_JOYSTICK_STICKS;
	    joy[i].num_sticks = MAX_JOYSTICK_STICKS;
	 }

	 /* loop over each pair of analog pads on the device (often just 1) */
	 for (stick=analog_start; stick < analog_start+num_pads; stick++) {
	    stick_class[i][stick] = GRIP_CLASS_ANALOG;

	    /* name pad... :( */
	    _al_sane_strncpy(tmpstr, "Analog", sizeof(tmpstr));
	    if (num_pads > 1) {
	       tmpstr[strlen(tmpstr)+2] = '\0';
	       tmpstr[strlen(tmpstr)+1] = '1'+stick-analog_start;
	       tmpstr[strlen(tmpstr)] = ' ';
	    }

	    joy[i].stick[stick].name = get_config_text(tmpstr);
	    joy[i].stick[stick].flags = JOYFLAG_ANALOG | JOYFLAG_SIGNED;

	    /* name axes */
	    joy[i].stick[stick].num_axis = 2;
	    _GrGetControlName(slot_index[i], GRIP_CLASS_ANALOG, (stick-analog_start)*2, tmpstr);
	    if (tmpstr[0] == '\0')
	       _al_sane_strncpy(tmpstr, "X", sizeof(tmpstr));

	    SIMPLIFY_AXIS(tmpstr);

	    joy[i].stick[stick].axis[0].name = get_config_text(tmpstr);
	    joy[i].stick[stick].axis[0].pos = 0;
	    joy[i].stick[stick].axis[0].d1 = 0;
	    joy[i].stick[stick].axis[0].d2 = 0;

	    _GrGetControlName(slot_index[i], GRIP_CLASS_ANALOG, (stick-analog_start)*2 + 1, tmpstr);
	    if (tmpstr[0] == '\0')
	       _al_sane_strncpy(tmpstr, "Y", sizeof(tmpstr));

	    SIMPLIFY_AXIS(tmpstr);

	    joy[i].stick[stick].axis[1].name = get_config_text(tmpstr);
	    joy[i].stick[stick].axis[1].pos = 0;
	    joy[i].stick[stick].axis[1].d1 = 0;
	    joy[i].stick[stick].axis[1].d2 = 0;
	 }
      }

      /* check for a Point-of-View Hat... */
      if ((ENABLE_POV) &&
	  (class_map & (1 << GRIP_CLASS_POV_HAT)) &&
	  (_GrGetMaxIndex(slot_index[i], GRIP_CLASS_POV_HAT) > 0)) {
	 int num_hats = 0;
	 int stick = 0;
	 int pov_start;

	 max_value[i][GRIP_CLASS_POV_HAT] = _GrGetMaxValue(slot_index[i], GRIP_CLASS_POV_HAT);
	 num_hats = (_GrGetMaxIndex(slot_index[i], GRIP_CLASS_POV_HAT) + 1) / 2;
	 pov_start = joy[i].num_sticks;
	 joy[i].num_sticks += num_hats;
	 if (joy[i].num_sticks > MAX_JOYSTICK_STICKS) {
	    num_hats -= joy[i].num_sticks - MAX_JOYSTICK_STICKS;
	    joy[i].num_sticks = MAX_JOYSTICK_STICKS;
	 }

	 /* loop over each pov hat on the device (often just 1) */
	 for (stick=pov_start; stick < pov_start+num_hats; stick++) {
	    stick_class[i][stick] = GRIP_CLASS_POV_HAT;

	    /* name hat... :( */
	    _al_sane_strncpy(tmpstr, "Hat", sizeof(tmpstr));
	    if (num_hats > 1) {
	       tmpstr[strlen(tmpstr)+2] = '\0';
	       tmpstr[strlen(tmpstr)+1] = '1'+stick-pov_start;
	       tmpstr[strlen(tmpstr)] = ' ';
	    }

	    joy[i].stick[stick].name = get_config_text(tmpstr);
	    joy[i].stick[stick].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;

	    /* name axes */
	    joy[i].stick[stick].num_axis = 2;
	    _GrGetControlName(slot_index[i], GRIP_CLASS_POV_HAT, (stick-pov_start)*2, tmpstr);
	    if (tmpstr[0] == '\0')
	       _al_sane_strncpy(tmpstr, "X", sizeof(tmpstr));

	    SIMPLIFY_AXIS(tmpstr);

	    joy[i].stick[stick].axis[0].name = get_config_text(tmpstr);
	    joy[i].stick[stick].axis[0].pos = 0;
	    joy[i].stick[stick].axis[0].d1 = 0;
	    joy[i].stick[stick].axis[0].d2 = 0;

	    _GrGetControlName(slot_index[i], GRIP_CLASS_POV_HAT, (stick-pov_start)*2 + 1, tmpstr);
	    if (tmpstr[0] == '\0')
	       _al_sane_strncpy(tmpstr, "Y", sizeof(tmpstr));

	    SIMPLIFY_AXIS(tmpstr);

	    joy[i].stick[stick].axis[1].name = get_config_text(tmpstr);
	    joy[i].stick[stick].axis[1].pos = 0;
	    joy[i].stick[stick].axis[1].d1 = 0;
	    joy[i].stick[stick].axis[1].d2 = 0;
	 }
      }

      /* check for throttles */
      if ((ENABLE_THROTTLE) &&
	  (class_map & (1 << GRIP_CLASS_THROTTLE))) {
	 int num_throt = 0;
	 int num_throt_sticks = 0;
	 int num_throt_extra_axes = 0;
	 int stick = 0;
	 int throttle_start;
	 int throttle_count;

	 max_value[i][GRIP_CLASS_THROTTLE] = _GrGetMaxValue(slot_index[i], GRIP_CLASS_THROTTLE);
	 num_throt = _GrGetMaxIndex(slot_index[i], GRIP_CLASS_THROTTLE) + 1;

	 if (PACK_THROTTLES) {
	    num_throt_sticks = num_throt / MAX_JOYSTICK_AXIS;
	    num_throt_extra_axes = num_throt % MAX_JOYSTICK_AXIS;
	 }
	 else {
	    num_throt_sticks = num_throt;
	    num_throt_extra_axes = 0;
	 }

	 throttle_start = joy[i].num_sticks;
	 joy[i].num_sticks += num_throt_sticks;
	 if (joy[i].num_sticks > MAX_JOYSTICK_STICKS) {
	    num_throt_sticks -= joy[i].num_sticks - MAX_JOYSTICK_STICKS;
	    joy[i].num_sticks = MAX_JOYSTICK_STICKS;
	 }

	 /* account for leftover axes, if packing: */
	 if ((num_throt_extra_axes) && (joy[i].num_sticks < MAX_JOYSTICK_STICKS)) {
	    joy[i].num_sticks += 1;
	    num_throt_sticks++;
	 }
	 else
	    num_throt_extra_axes = 0;

	 /* loop over each throttle on the device */
	 throttle_count = 0;
	 for (stick=throttle_start; stick < throttle_start+num_throt_sticks; stick++) {
	    int last_axis, a;

	    if (PACK_THROTTLES) {
	       if ((stick == throttle_start+num_throt_sticks-1) &&
		   (num_throt_extra_axes))
		  last_axis = num_throt_extra_axes;
	       else
		  last_axis = MAX_JOYSTICK_AXIS;
	    }
	    else
	       last_axis = 1;

	    stick_class[i][stick] = GRIP_CLASS_THROTTLE;
	    joy[i].stick[stick].flags = JOYFLAG_ANALOG | JOYFLAG_SIGNED;
	    joy[i].stick[stick].num_axis = last_axis;

	    /* name throttle... */
	    _al_sane_strncpy(tmpstr, "Throttle", sizeof(tmpstr));
	    if (num_throt_sticks > 1) {
	       tmpstr[strlen(tmpstr)+2] = '\0';
	       tmpstr[strlen(tmpstr)+1] = '1'+stick-throttle_start;
	       tmpstr[strlen(tmpstr)] = ' ';
	    }

	    joy[i].stick[stick].name = get_config_text(tmpstr);

	    for (a=0; a<last_axis; a++) {
	       /* name axis */
	       _GrGetControlName(slot_index[i], GRIP_CLASS_THROTTLE, throttle_count, tmpstr);
	       if (tmpstr[0] == '\0')
		  _al_sane_strncpy(tmpstr, "Throttle", sizeof(tmpstr));

	       joy[i].stick[stick].axis[a].name = get_config_text(tmpstr);
	       joy[i].stick[stick].axis[a].pos = 0;
	       joy[i].stick[stick].axis[a].d1 = 0;
	       joy[i].stick[stick].axis[a].d2 = 0;

	       throttle_count++;
	    }
	 }
      }

      /* DON'T check for velocity - because I don't know what it is, anyway.
       * Besides, if the Xterminator doesn't have it, you don't need it.
       */

      /* be silly and check for buttons... */
      joy[i].num_buttons = 0;
      if (class_map & (1 << GRIP_CLASS_BUTTON)) {
	 int b = 0;

	 joy[i].num_buttons = _GrGetMaxIndex(slot_index[i], GRIP_CLASS_BUTTON) + 1;
	 if (joy[i].num_buttons > MAX_JOYSTICK_BUTTONS)
	    joy[i].num_buttons = MAX_JOYSTICK_BUTTONS;

	 for (b=0; b<joy[i].num_buttons; b++) {
	    _GrGetControlName(slot_index[i], GRIP_CLASS_BUTTON, b, tmpstr);
	    if (tmpstr[0] == '\0')
	       _al_sane_strncpy(tmpstr, name_b[b], sizeof(tmpstr));
	    joy[i].button[b].name = get_config_text(tmpstr);
	    joy[i].button[b].b = 0;
	 }
      }

      throttle_button_start[i] = joy[i].num_buttons;
   }

   return 0;
}



/* grip_exit:
 *  Shuts down the driver.
 */
static void grip_exit(void)
{
   _GrShutdown();
   grip_unlink();
}



/* grip_poll:
 *  Updates the joystick status variables.
 *  Assumes GrIP driver has been validly initialized.
 */
static int grip_poll(void)
{
   int i, all_good;

   all_good = 0;
   _GrRefresh(0);

   for (i=0; i<num_joysticks; i++) {
      if (read_grip(i) == 1)
	 all_good = -1;
   }

   return all_good;
}



static int already_linked = 0;



/* The following intiallization routine is mostly taken from
 * the Gravis SDK.
 */
static int grip_link(void)
{
   char name[1024], tmp[64], *s;
   PACKFILE *f;
   long size;
   void *image;

   /* Search for the GrIP library: GRIP.GLL
    *
    * First search in the current directory, then search in any directories
    * specified by the "SET GRIP=" environment variable, then search in any
    * directories in the path, and finally search in "C:\GRIP\".
    */

   if (already_linked)
      return 0;

 #ifdef ALLEGRO_DJGPP

   s = searchpath("grip.gll");
   if (s) {
      do_uconvert(s, U_ASCII, name, U_CURRENT, sizeof(name));
   }
   else {

 #else

   do_uconvert("grip.gll", U_ASCII, name, U_CURRENT, sizeof(name));
   if (!exists(name)) {

 #endif

      s = getenv("GRIP");
      if (s) {
	 do_uconvert(s, U_ASCII, name, U_CURRENT, sizeof(name) - ucwidth(OTHER_PATH_SEPARATOR));
	 put_backslash(name);
	 ustrzcat(name, sizeof(name), uconvert_ascii("grip.gll", tmp));
      }
   }

   if (!exists(name))
      do_uconvert("c:\\grip\\grip.gll", U_ASCII, name, U_CURRENT, sizeof(name));

   size = file_size_ex(name);

   /* if size is greater than 32k, this is definitely not the correct file */
   if ((size <= 0) || (size > 32*1024))
      return -1;

   /* open the loadable library file size */
   if ((f = pack_fopen(name, F_READ)) == NULL)
      return -1;

   if ((image = _AL_MALLOC(size)) == NULL) {
      pack_fclose(f);
      return -1;
   }

   pack_fread(image, size, f);
   pack_fclose(f);

   /* link to the library (and free up the temporary memory) */
   if (!_GrLink(image, size)) {
      _AL_FREE(image);
      return -1;
   }

   _AL_FREE(image);
   already_linked = 1;
   return 0;
}



static int grip_unlink(void)
{
   already_linked = 0;

   if (!_GrUnlink())
      return -1;

   return 0;
}

