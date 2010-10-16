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
 *      Linux console mouse module.
 *
 *      By George Foot.
 *
 *      This file contains a generic framework for different mouse 
 *      types to hook into.  See lmseps2.c for an example.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "linalleg.h"


static int mouse_mx = 0;            /* internal position, in mickeys */
static int mouse_my = 0;

static int mouse_sx = 128;          /* mickey -> pixel scaling factor */
static int mouse_sy = 128;

static int mouse_minx = 0;          /* mouse range */
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;          /* for get_mouse_mickeys() */
static int mymickey_y = 0;

#define MICKEY_TO_COORD_X(n)        ((n) * mouse_sx / 256)
#define MICKEY_TO_COORD_Y(n)        ((n) * mouse_sy / 256)

#define COORD_TO_MICKEY_X(n)        ((n) * 256 / mouse_sx)
#define COORD_TO_MICKEY_Y(n)        ((n) * 256 / mouse_sy)


/* __al_linux_mouse_position:
 *  Sets the position of the mickey-mode mouse.
 */
void __al_linux_mouse_position(int x, int y)
{
   DISABLE();

   _mouse_x = x;
   _mouse_y = y;

   mouse_mx = COORD_TO_MICKEY_X(x);
   mouse_my = COORD_TO_MICKEY_Y(y);

   mymickey_x = mymickey_y = 0;

   ENABLE();
}

/* __al_linux_mouse_set_range:
 *  Sets the range of the mickey-mode mouse.
 */
void __al_linux_mouse_set_range(int x1, int y1, int x2, int y2)
{
   mouse_minx = x1;
   mouse_miny = y1;
   mouse_maxx = x2;
   mouse_maxy = y2;

   DISABLE();

   _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
   _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);

   mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
   mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

   ENABLE();
}

/* __al_linux_mouse_set_speed:
 *  Sets the speed of the mickey-mode mouse.
 */
void __al_linux_mouse_set_speed(int xspeed, int yspeed)
{
   int scale;

   if (gfx_driver)
      scale = 256*gfx_driver->w/320;
   else
      scale = 256;

   DISABLE();

   mouse_sx = scale / MAX(1, xspeed);
   mouse_sy = scale / MAX(1, yspeed);

   mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
   mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

   ENABLE();
}

/* __al_linux_mouse_get_mickeys:
 *  Reads the mickey-mode count.
 */
void __al_linux_mouse_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}

/* __al_linux_mouse_handler:
 *  Movement callback for mickey-mode driver.
 */
void __al_linux_mouse_handler (int x, int y, int z, int b)
{
   _mouse_b = b;

   mymickey_x += x;
   mymickey_y -= y;

   mouse_mx += x;
   mouse_my -= y;

   _mouse_x = MICKEY_TO_COORD_X(mouse_mx);
   _mouse_y = MICKEY_TO_COORD_Y(mouse_my);
   _mouse_z += z;

   if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx) ||
       (_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {

      _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
      _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);

      mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
      mouse_my = COORD_TO_MICKEY_Y(_mouse_y);
   }

   _handle_mouse_input();
}


static int update_mouse (void);
static void resume_mouse (void);
static void suspend_mouse (void);

static STD_DRIVER std_mouse = {
   STD_MOUSE,
   update_mouse,
   resume_mouse,
   suspend_mouse,
   -1   /* fd -- filled in later */
};


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>


static int resume_count;
static INTERNAL_MOUSE_DRIVER *internal_driver;


/* update_mouse:
 *  Reads data from the mouse, attempting to process it.
 */
static int update_mouse (void)
{
   static unsigned char buf[1024];
   static int bytes_in_buffer = 0;
   int bytes_read;
   fd_set set;
   struct timeval tv = { 0, 0 };

   if (resume_count <= 0) return 0;

   FD_ZERO(&set);
   FD_SET(std_mouse.fd, &set);
   if (select (FD_SETSIZE, &set, NULL, NULL, &tv) <= 0)
      return 0;

   bytes_read = read(std_mouse.fd, &buf[bytes_in_buffer], sizeof(buf) - bytes_in_buffer);
   if (bytes_read <= 0)
      return 0;

   bytes_in_buffer += bytes_read;

   while (bytes_in_buffer && (bytes_read = internal_driver->process (buf, bytes_in_buffer))) {
      int i;
      bytes_in_buffer -= bytes_read;
      for (i = 0; i < bytes_in_buffer; i++)
	 buf[i] = buf[i+bytes_read];
   }

   return 1;
}


/* activate_mouse:
 *  Put mouse device into the mode we want.
 */
static void activate_mouse (void)
{
}

/* deactivate_mouse:
 *  Restore the original settings.
 */
static void deactivate_mouse (void)
{
}

/* suspend_mouse:
 *  Suspend our mouse handling.
 */
static void suspend_mouse (void)
{
	resume_count--;
	if (resume_count == 0)
		deactivate_mouse();
}

/* resume_mouse:
 *  Start/resume Allegro mouse handling.
 */
static void resume_mouse (void)
{
	if (resume_count == 0)
		activate_mouse();
	resume_count++;
}


/* __al_linux_mouse_init:
 *  Sets up the mouse driver.
 */
int __al_linux_mouse_init (INTERNAL_MOUSE_DRIVER *drv)
{
	internal_driver = drv;
	std_mouse.fd = drv->device;

	/* Mouse is disabled now. */
	resume_count = 0;

	/* Register our driver (enables it too). */
	__al_linux_add_standard_driver (&std_mouse);

	return internal_driver->num_buttons;
}

/* __al_linux_mouse_exit:
 *  Removes our driver.
 */
void __al_linux_mouse_exit (void)
{
	__al_linux_remove_standard_driver (&std_mouse);
}

