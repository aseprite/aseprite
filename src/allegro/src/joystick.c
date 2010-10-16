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
 *      High level joystick input framework.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* dummy driver for when no joystick is present */
static int nojoy_ret0(void) { return 0; }
static void nojoy_void(void) { }


JOYSTICK_DRIVER joystick_none =
{
   JOY_TYPE_NONE,
   empty_string,
   empty_string,
   "No joystick",
   nojoy_ret0,
   nojoy_void,
   nojoy_ret0,
   NULL, NULL,
   NULL, NULL
};


int _joystick_installed = FALSE;

JOYSTICK_DRIVER *joystick_driver = NULL;

int _joy_type = JOY_TYPE_NONE;

JOYSTICK_INFO joy[MAX_JOYSTICKS];
int num_joysticks = 0;

static int joy_loading = FALSE;



/* clear_joystick_vars:
 *  Resets the joystick state variables to their default values.
 */
static void clear_joystick_vars(void)
{
   AL_CONST char *unused = get_config_text("unused");
   int i, j, k;

   #define ARRAY_SIZE(a)   ((int)sizeof((a)) / (int)sizeof((a)[0]))

   for (i=0; i<ARRAY_SIZE(joy); i++) {
      joy[i].flags = 0;
      joy[i].num_sticks = 0;
      joy[i].num_buttons = 0;

      for (j=0; j<ARRAY_SIZE(joy[i].stick); j++) {
	 joy[i].stick[j].flags = 0;
	 joy[i].stick[j].num_axis = 0;
	 joy[i].stick[j].name = unused;

	 for (k=0; k<ARRAY_SIZE(joy[i].stick[j].axis); k++) {
	    joy[i].stick[j].axis[k].pos = 0;
	    joy[i].stick[j].axis[k].d1 = FALSE;
	    joy[i].stick[j].axis[k].d2 = FALSE;
	    joy[i].stick[j].axis[k].name = unused;
	 }
      }

      for (j=0; j<ARRAY_SIZE(joy[i].button); j++) {
	 joy[i].button[j].b = FALSE;
	 joy[i].button[j].name = unused;
      }
   }

   num_joysticks = 0;
}



/* update_calib:
 *  Updates the calibration flags for the specified joystick structure.
 */
static void update_calib(int n)
{
   int c = FALSE;
   int i;

   for (i=0; i<joy[n].num_sticks; i++) {
      if (joy[n].stick[i].flags & (JOYFLAG_CALIB_DIGITAL | JOYFLAG_CALIB_ANALOGUE)) {
	 joy[n].stick[i].flags |= JOYFLAG_CALIBRATE;
	 c = TRUE;
      }
      else
	 joy[n].stick[i].flags &= ~JOYFLAG_CALIBRATE;
   }

   if (c)
      joy[n].flags |= JOYFLAG_CALIBRATE;
   else
      joy[n].flags &= ~JOYFLAG_CALIBRATE;
}



/* install_joystick:
 *  Initialises the joystick module.
 */
int install_joystick(int type)
{
   _DRIVER_INFO *driver_list;
   int c;

   if (_joystick_installed)
      return 0;

   clear_joystick_vars();

   usetc(allegro_error, 0);

   if (system_driver->joystick_drivers)
      driver_list = system_driver->joystick_drivers();
   else
      driver_list = _joystick_driver_list;

   /* search table for a specific driver */
   for (c=0; driver_list[c].driver; c++) { 
      if (driver_list[c].id == type) {
	 joystick_driver = driver_list[c].driver;
	 joystick_driver->name = joystick_driver->desc = get_config_text(joystick_driver->ascii_name);
	 _joy_type = type;
	 if (joystick_driver->init() != 0) {
	    if (!ugetc(allegro_error))
	       uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s not found"), joystick_driver->name);
	    joystick_driver = NULL; 
	    _joy_type = JOY_TYPE_NONE;
	    return -1;
	 }
	 break;
      }
   }

   /* autodetect driver */
   if (!joystick_driver) {
      if (!joy_loading) {
	 if (load_joystick_data(NULL) != -1)
	    return 0;
      }

      for (c=0; driver_list[c].driver; c++) {
	 if (driver_list[c].autodetect) {
	    joystick_driver = driver_list[c].driver;
	    joystick_driver->name = joystick_driver->desc = get_config_text(joystick_driver->ascii_name);
	    _joy_type = driver_list[c].id;
	    if (joystick_driver->init() == 0)
	       break;
	 }
      }
   }

   if (!driver_list[c].driver) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("No joysticks found"));
      return -1;
   }

   for (c=0; c<num_joysticks; c++)
      update_calib(c);

   poll_joystick();

   _add_exit_func(remove_joystick, "remove_joystick");
   _joystick_installed = TRUE;

   return 0;
}



/* remove_joystick:
 *  Shuts down the joystick module.
 */
void remove_joystick(void)
{
   if (_joystick_installed) {
      joystick_driver->exit();

      joystick_driver = NULL;
      _joy_type = JOY_TYPE_NONE;

      clear_joystick_vars();

      _remove_exit_func(remove_joystick);
      _joystick_installed = FALSE;
   }
}



/* poll_joystick:
 *  Reads the current input state into the joystick status variables.
 */
int poll_joystick(void)
{
   if ((joystick_driver) && (joystick_driver->poll))
      return joystick_driver->poll();

   return -1; 
}



/* save_joystick_data:
 *  After calibrating a joystick, this function can be used to save the
 *  information into the specified file, from where it can later be 
 *  restored by calling load_joystick_data().
 */
int save_joystick_data(AL_CONST char *filename)
{
   char tmp1[64], tmp2[64];

   if (filename) {
      push_config_state();
      set_config_file(filename);
   }

   set_config_id(uconvert_ascii("joystick", tmp1), uconvert_ascii("joytype", tmp2), _joy_type);

   if ((joystick_driver) && (joystick_driver->save_data))
      joystick_driver->save_data();

   if (filename)
      pop_config_state();

   return 0;
}



/* load_joystick_data:
 *  Restores a set of joystick calibration data previously saved by
 *  save_joystick_data().
 */
int load_joystick_data(AL_CONST char *filename)
{
   char tmp1[64], tmp2[64];
   int ret, c;

   joy_loading = TRUE;

   if (_joystick_installed)
      remove_joystick();

   if (filename) {
      push_config_state();
      set_config_file(filename);
   }

   _joy_type = get_config_id(uconvert_ascii("joystick", tmp1), uconvert_ascii("joytype", tmp2), -1);

   if (_joy_type < 0) {
      _joy_type = JOY_TYPE_NONE;
      ret = -1;
   }
   else {
      ret = install_joystick(_joy_type);

      if (ret == 0) {
	 if (joystick_driver->load_data)
	    ret = joystick_driver->load_data();
      }
      else
	 ret = -2;
   }

   if (filename)
      pop_config_state();

   if (ret == 0) {
      for (c=0; c<num_joysticks; c++)
	 update_calib(c);

      poll_joystick();
   }

   joy_loading = FALSE;

   return ret;
}



/* calibrate_joystick_name:
 *  Returns the name of the next calibration operation to be performed on
 *  the specified stick.
 */
AL_CONST char *calibrate_joystick_name(int n)
{
   if ((!joystick_driver) || (!joystick_driver->calibrate_name) ||
       (!(joy[n].flags & JOYFLAG_CALIBRATE)))
      return NULL;

   return joystick_driver->calibrate_name(n);
}



/* calibrate_joystick:
 *  Performs the next caliabration operation required for the specified stick.
 */
int calibrate_joystick(int n)
{
   int ret;

   if ((!joystick_driver) || (!joystick_driver->calibrate) ||
       (!(joy[n].flags & JOYFLAG_CALIBRATE)))
      return -1;

   ret = joystick_driver->calibrate(n);

   if (ret == 0)
      update_calib(n);

   return ret;
}



/* initialise_joystick:
 *  Bodge function to preserve backward compatibility with the old API.
 */
int initialise_joystick(void)
{
   int type = _joy_type;

   if (_joystick_installed)
      remove_joystick();

   #ifdef JOY_TYPE_STANDARD
      if (type == JOY_TYPE_NONE)
	 type = JOY_TYPE_STANDARD;
   #endif

   return install_joystick(type);
}
