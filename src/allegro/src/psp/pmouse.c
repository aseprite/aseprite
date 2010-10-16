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
 *      PSP mouse driver.
 *      TODO:.
 *
 *      By sauron_le_noir some part of psp_mouse_timer_poll based on
 *                        the sdl code of Nekuz0r
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintpsp.h"

#include "math.h"
#include <pspctrl.h>

#ifndef ALLEGRO_PSP
#error something is wrong with the makefile
#endif

static int  psp_mouse_init(void);
static void psp_mouse_exit(void);
static void psp_mouse_position(int, int);
static void psp_mouse_set_range(int, int, int, int);
static void psp_mouse_get_mickeys(int *, int *);
static void psp_mouse_timer_poll(void);


MOUSE_DRIVER mouse_psp =
{
   MOUSE_PSP,
   empty_string,
   empty_string,
   "psp mouse",
   psp_mouse_init,
   psp_mouse_exit,
   NULL,       // AL_METHOD(void, poll, (void));
   psp_mouse_timer_poll,                 // AL_METHOD(void, timer_poll, (void));
   psp_mouse_position,
   psp_mouse_set_range,
   NULL,       // AL_METHOD(void, set_speed, (int xspeed, int yspeed));
   psp_mouse_get_mickeys,
   NULL,       // AL_METHOD(int,  analyse_data, (AL_CONST char *buffer, int size));
   NULL,       // AL_METHOD(void,  enable_hardware_cursor, (AL_CONST int mode));
   NULL        // AL_METHOD(int,  select_system_cursor, (AL_CONST int cursor));
};

static int mouse_minx = 0;
static int mouse_miny = 0;
static int mouse_maxx = 479;  /* 0 to 479 = 480 */
static int mouse_maxy = 271;  /* 0 to 272 = 272 */   /* 480x272 is the resolution of the psp display */

static SceCtrlData pad;
static int aStickX = 0;
static int aStickY = 0;
static float aStickR;
static float aStickA;
static int x,y;
static float Precision = 360.0f;



static int getSqrRadius(int X, int Y) 
{
   return sqrt((X * X) + (Y * Y));
}



static int psp_mouse_init(void)
{
   sceCtrlSetSamplingCycle(0);
   sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

   return 3; //Num of buttons.
}



static void psp_mouse_timer_poll(void)
{
   sceCtrlPeekBufferPositive(&pad, 1);

   if (pad.Buttons != 0)
   {
     sceCtrlReadBufferPositive(&pad, 1);
     _mouse_b = 0;
     if (pad.Buttons & PSP_CTRL_LTRIGGER) _mouse_b = 1;
     if (pad.Buttons & PSP_CTRL_RTRIGGER) _mouse_b = 2;
     if ( (pad.Buttons & PSP_CTRL_LTRIGGER) && (pad.Buttons & PSP_CTRL_RTRIGGER)) _mouse_b = 4;
   }
   aStickX = pad.Ly - 128;
   aStickY = pad.Lx - 128;

   aStickR = getSqrRadius(aStickX, aStickY);
   if (aStickR > 100) {
      if (getSqrRadius(aStickX, aStickY) > 30) {
         aStickA = ((atan2f(aStickX, -aStickY) + M_PI) / (M_PI * 2)) * Precision;
      } else {
         aStickA = -1;     
      }

      //1° a 89°
      if (aStickA >= 1 && aStickA <= (89 * (Precision / 360.0f))) {
         x += (10 * (1 - (aStickA / (Precision / 360.0f) / 90)));
         y -= (10 * (0 + (aStickA / (Precision / 360.0f) / 90)));
      }
      else if (aStickA >= (91 * (Precision / 360.0f)) && aStickA <= (179 * (Precision / 360.0f))) {
         x -= (10 * (0 + (((aStickA / (Precision / 360.0f)) - 89) / 90)));
         y -= (10 * (1 - (((aStickA / (Precision / 360.0f)) - 89) / 90)));
      }
      else if (aStickA >= (181 * (Precision / 360.0f)) && aStickA <= (269 * (Precision / 360.0f))) {
         x -= (10 * (1 - (((aStickA / (Precision / 360.0f)) - 179) / 90)));
         y += (10 * (0 + (((aStickA / (Precision / 360.0f)) - 179) / 90)));
      }
      else if (aStickA >= (271 * (Precision / 360.0f)) && aStickA <= (359 * (Precision / 360.0f))) {
         x += (10 * (0 + (((aStickA / (Precision / 360.0f)) - 269) / 90)));
         y += (10 * (1 - (((aStickA / (Precision / 360.0f)) - 269) / 90)));
      }
      else if (aStickA == 0) {
         x += 10;
      }
      else if (aStickA == (90 * (Precision / 360.0f))) {
         y -= 10;
      }
      else if (aStickA == (180 * (Precision / 360.0f))) {
         x -= 10;
      }
      else if (aStickR == (270 * (Precision / 360.0f))) {
         y += 10;
      }
   }
   if (x < 0)
      x = 0;

   if (x > 479)
      x = 479;

   if (y < 0)
      y = 0;

   if (y > 271)
      y = 271;

   _mouse_x = x;
   _mouse_y = y;
}



static void psp_mouse_position(int x, int y)
{
   _mouse_x = x;
   if (_mouse_x <  mouse_minx) _mouse_x = 0;
   if (_mouse_x > mouse_maxx)  _mouse_x = mouse_maxx;
   _mouse_y = y;
   if (_mouse_y < mouse_miny) _mouse_y = 0;
   if (_mouse_y > mouse_maxy) _mouse_y = mouse_maxy;
}



static void psp_mouse_set_range(int x1, int y1, int x2, int y2)
{
}



static void psp_mouse_get_mickeys(int *mickeyx, int *mickeyy)
{
   *mickeyx = 0;
   *mickeyy = 0;
}



static void psp_mouse_exit(void)
{
}
