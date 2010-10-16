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
 *      Windows joystick high-level helper functions.
 *
 *      By Eric Botcazou.
 *
 *      Based on code by Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
#endif

#ifndef ALLEGRO_WINDOWS
   #error something is wrong with the makefile
#endif


static char name_x[] = "X";
static char name_y[] = "Y";
static char name_stick[] = "stick";
static char name_throttle[] = "throttle";
static char name_rudder[] = "rudder";
static char name_slider[] = "slider";
static char name_hat[] = "hat";
static char *name_b[MAX_JOYSTICK_BUTTONS] = {
   "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8",
   "B9", "B10", "B11", "B12", "B13", "B14", "B15", "B16",
   "B17", "B18", "B19", "B20", "B21", "B22", "B23", "B24",
   "B25", "B26", "B27", "B28", "B29", "B30", "B31", "B32"
};

#define JOY_POVFORWARD_WRAP  36000



/* win_update_joystick_status:
 *  Updates the specified joystick info structure by translating
 *  Windows joystick status into Allegro joystick status.
 */
int win_update_joystick_status(int n, WINDOWS_JOYSTICK_INFO *win_joy)
{
   int n_stick, n_axis, n_but, max_stick, win_axis, p;

   if (n >= num_joysticks)
      return -1;

   /* sticks */
   n_stick = 0;
   win_axis = 0;

   /* skip hat at this point, it will be handled later */
   if (win_joy->caps & JOYCAPS_HASPOV)
      max_stick = joy[n].num_sticks - 1;
   else
      max_stick = joy[n].num_sticks;

   for (n_stick = 0; n_stick < max_stick; n_stick++) {
      for (n_axis = 0; n_axis < joy[n].stick[n_stick].num_axis; n_axis++) {
         p = win_joy->axis[win_axis];

         /* set pos of analog stick */
         if (joy[n].stick[n_stick].flags & JOYFLAG_ANALOGUE) {
            if (joy[n].stick[n_stick].flags & JOYFLAG_SIGNED)
               joy[n].stick[n_stick].axis[n_axis].pos = p - 128;
            else
               joy[n].stick[n_stick].axis[n_axis].pos = p;
         }

         /* set pos of digital stick */
         if (joy[n].stick[n_stick].flags & JOYFLAG_DIGITAL) {
            if (p < 64)
               joy[n].stick[n_stick].axis[n_axis].d1 = TRUE;
            else
               joy[n].stick[n_stick].axis[n_axis].d1 = FALSE;

            if (p > 192)
               joy[n].stick[n_stick].axis[n_axis].d2 = TRUE;
            else
               joy[n].stick[n_stick].axis[n_axis].d2 = FALSE;
         }

         win_axis++;
      }
   }

   /* hat */
   if (win_joy->caps & JOYCAPS_HASPOV) {
      /* emulate analog joystick */
      joy[n].stick[n_stick].axis[0].pos = 0;
      joy[n].stick[n_stick].axis[1].pos = 0;

      /* left */
      if ((win_joy->hat > JOY_POVBACKWARD) && (win_joy->hat < JOY_POVFORWARD_WRAP)) {
         joy[n].stick[n_stick].axis[0].d1 = TRUE;
         joy[n].stick[n_stick].axis[0].pos = -128;
      }
      else {
         joy[n].stick[n_stick].axis[0].d1 = FALSE;
      }

      /* right */
      if ((win_joy->hat > JOY_POVFORWARD) && (win_joy->hat < JOY_POVBACKWARD)) {
         joy[n].stick[n_stick].axis[0].d2 = TRUE;
         joy[n].stick[n_stick].axis[0].pos = +128;
      }
      else {
         joy[n].stick[n_stick].axis[0].d2 = FALSE;
      }

      /* forward */
      if (((win_joy->hat > JOY_POVLEFT) && (win_joy->hat <= JOY_POVFORWARD_WRAP)) ||
          ((win_joy->hat >= JOY_POVFORWARD) && (win_joy->hat < JOY_POVRIGHT))) {
         joy[n].stick[n_stick].axis[1].d1 = TRUE;
         joy[n].stick[n_stick].axis[1].pos = -128;
      }
      else {
         joy[n].stick[n_stick].axis[1].d1 = FALSE;
      }

      /* backward */
      if ((win_joy->hat > JOY_POVRIGHT) && (win_joy->hat < JOY_POVLEFT)) {
         joy[n].stick[n_stick].axis[1].d2 = TRUE;
         joy[n].stick[n_stick].axis[1].pos = +128;
      }
      else {
         joy[n].stick[n_stick].axis[1].d2 = FALSE;
      }
   }

   /* buttons */
   for (n_but = 0; n_but < win_joy->num_buttons; n_but++)
      joy[n].button[n_but].b = win_joy->button[n_but];

   return 0;
}



/* win_add_joystick:
 *  Adds a new joystick (fills in a new joystick info structure).
 */
int win_add_joystick(WINDOWS_JOYSTICK_INFO *win_joy)
{
   int n_stick, n_but, max_stick, win_axis;

   if (num_joysticks == MAX_JOYSTICKS-1)
      return -1;

   joy[num_joysticks].flags = JOYFLAG_ANALOGUE | JOYFLAG_DIGITAL;

   /* how many sticks ? */
   n_stick = 0;

   if (win_joy->num_axes > 0) {
      win_axis = 0;

      /* main analogue stick */
      if (win_joy->num_axes > 1) {
         joy[num_joysticks].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
         joy[num_joysticks].stick[n_stick].axis[0].name = (win_joy->axis_name[0] ? win_joy->axis_name[0] : name_x);
         joy[num_joysticks].stick[n_stick].axis[1].name = (win_joy->axis_name[1] ? win_joy->axis_name[1] : name_y);
         joy[num_joysticks].stick[n_stick].name = name_stick;

         /* Z-axis: throttle */
         if (win_joy->caps & JOYCAPS_HASZ) {
            joy[num_joysticks].stick[n_stick].num_axis = 3;
            joy[num_joysticks].stick[n_stick].axis[2].name = (win_joy->axis_name[2] ? win_joy->axis_name[2] : name_throttle);
            win_axis += 3;
         }
         else {
            joy[num_joysticks].stick[n_stick].num_axis = 2;
            win_axis += 2;
         }

         n_stick++;
      }

      /* first 1-axis stick: rudder */
      if (win_joy->caps & JOYCAPS_HASR) {
         joy[num_joysticks].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
         joy[num_joysticks].stick[n_stick].num_axis = 1;
         joy[num_joysticks].stick[n_stick].axis[0].name = "";
         joy[num_joysticks].stick[n_stick].name = (win_joy->axis_name[win_axis] ? win_joy->axis_name[win_axis] : name_rudder);
         win_axis++;
         n_stick++;
      }

      max_stick = (win_joy->caps & JOYCAPS_HASPOV ? MAX_JOYSTICK_STICKS-1 : MAX_JOYSTICK_STICKS);

      /* other 1-axis sticks: sliders */
      while ((win_axis < win_joy->num_axes) && (n_stick < max_stick)) {
         joy[num_joysticks].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_ANALOGUE | JOYFLAG_UNSIGNED;
         joy[num_joysticks].stick[n_stick].num_axis = 1;
         joy[num_joysticks].stick[n_stick].axis[0].name = "";
         joy[num_joysticks].stick[n_stick].name = (win_joy->axis_name[win_axis] ? win_joy->axis_name[win_axis] : name_slider);
         win_axis++;
         n_stick++;
      }

      /* hat */
      if (win_joy->caps & JOYCAPS_HASPOV) {
         joy[num_joysticks].stick[n_stick].flags = JOYFLAG_DIGITAL | JOYFLAG_SIGNED;
         joy[num_joysticks].stick[n_stick].num_axis = 2;
         joy[num_joysticks].stick[n_stick].axis[0].name = "left/right";
         joy[num_joysticks].stick[n_stick].axis[1].name = "up/down";
         joy[num_joysticks].stick[n_stick].name = (win_joy->hat_name ? win_joy->hat_name : name_hat);
         n_stick++;
      }
   }

   joy[num_joysticks].num_sticks = n_stick;

   /* how many buttons ? */
   joy[num_joysticks].num_buttons = win_joy->num_buttons;

   /* fill in the button names */
   for (n_but = 0; n_but < joy[num_joysticks].num_buttons; n_but++)
      joy[num_joysticks].button[n_but].name = (win_joy->button_name[n_but] ? win_joy->button_name[n_but] : name_b[n_but]);

   num_joysticks++;

   return 0;
}



/* win_remove_all_joysticks:
 *  Removes all registered joysticks.
 */
void win_remove_all_joysticks(void)
{
   num_joysticks = 0;
}
