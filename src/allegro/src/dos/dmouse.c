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
 *      DOS mouse module.
 *
 *      By Shawn Hargreaves.
 *
 *      3-button support added by Fabian Nunez.
 *
 *      Mark Wodrich added double-buffered drawing of the mouse pointer and
 *      the set_mouse_sprite_focus() function.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



static int mouse_mx = 0;            /* internal position, in mickeys */
static int mouse_my = 0;

static int mouse_sx = 2;            /* mickey -> pixel scaling factor */
static int mouse_sy = 2;

static int mouse_minx = 0;          /* mouse range */
static int mouse_miny = 0;
static int mouse_maxx = 319;
static int mouse_maxy = 199;

static int mymickey_x = 0;          /* for get_mouse_mickeys() */
static int mymickey_y = 0;
static int mymickey_ox = 0; 
static int mymickey_oy = 0;

#ifdef ALLEGRO_DJGPP
   static _go32_dpmi_seginfo mouse_seginfo;
#endif

static __dpmi_regs mouse_regs;


#define MICKEY_TO_COORD_X(n)        ((n) / mouse_sx)
#define MICKEY_TO_COORD_Y(n)        ((n) / mouse_sy)

#define COORD_TO_MICKEY_X(n)        ((n) * mouse_sx)
#define COORD_TO_MICKEY_Y(n)        ((n) * mouse_sy)


#define CLEAR_MICKEYS()             \
{                                   \
   __dpmi_regs regs;                \
   regs.x.ax = 11;                  \
   __dpmi_int(0x33, &regs);         \
   mymickey_ox = 0;                 \
   mymickey_oy = 0;                 \
}



/* this driver does all our own position tracking, avoiding int 0x33 bugs */
static int mick_init(void);
static void mick_exit(void);
static void mick_position(int x, int y);
static void mick_set_range(int x1, int y1, int x2, int y2);
static void mick_set_speed(int xspeed, int yspeed);
static void mick_get_mickeys(int *mickeyx, int *mickeyy);


MOUSE_DRIVER mousedrv_mickeys =
{
   MOUSEDRV_MICKEYS,
   empty_string,
   empty_string,
   "Mickey mouse",
   mick_init,
   mick_exit,
   NULL,
   NULL,
   mick_position,
   mick_set_range,
   mick_set_speed,
   mick_get_mickeys,
   NULL,
   NULL,
   NULL
};



/* this driver uses the int 0x33 movement callbacks */
static int int33_init(void);
static void int33_exit(void);
static void int33_position(int x, int y);
static void int33_set_range(int x1, int y1, int x2, int y2);
static void int33_set_speed(int xspeed, int yspeed);
static void int33_get_mickeys(int *mickeyx, int *mickeyy);


MOUSE_DRIVER mousedrv_int33 =
{
   MOUSEDRV_INT33,
   empty_string,
   empty_string,
   "Int 0x33 mouse",
   int33_init,
   int33_exit,
   NULL,
   NULL,
   int33_position,
   int33_set_range,
   int33_set_speed,
   int33_get_mickeys,
   NULL,
   NULL,
   NULL
};



/* this driver polls int 0x33 from a timer handler */
static int polling_init(void);
static void polling_exit(void);
static void polling_timer_poll(void);


MOUSE_DRIVER mousedrv_polling =
{
   MOUSEDRV_POLLING,
   empty_string,
   empty_string,
   "Polling mouse",
   polling_init,
   polling_exit,
   NULL,
   polling_timer_poll,
   int33_position,
   int33_set_range,
   int33_set_speed,
   int33_get_mickeys,
   NULL,
   NULL,
   NULL
};



/* this driver is a wrapper for the polling driver*/
static int winnt_init(void);


MOUSE_DRIVER mousedrv_winnt =
{
   MOUSEDRV_POLLING,
   empty_string,
   empty_string,
   "Windows NT mouse",
   winnt_init,
   polling_exit,
   NULL,
   polling_timer_poll,
   int33_position,
   int33_set_range,
   int33_set_speed,
   int33_get_mickeys,
   NULL,
   NULL,
   NULL
};



/* this driver is a wrapper for the mickeys driver */
static int win2k_init(void);


MOUSE_DRIVER mousedrv_win2k =
{
   MOUSEDRV_MICKEYS,
   empty_string,
   empty_string,
   "Windows 2000 mouse",
   win2k_init,
   mick_exit,
   NULL,
   NULL,
   mick_position,
   mick_set_range,
   mick_set_speed,
   mick_get_mickeys,
   NULL,
   NULL,
   NULL
};



/* list the available drivers */
_DRIVER_INFO _mouse_driver_list[] =
{
   { MOUSEDRV_MICKEYS,  &mousedrv_mickeys,   TRUE  },
   { MOUSEDRV_INT33,    &mousedrv_int33,     TRUE  },
   { MOUSEDRV_POLLING,  &mousedrv_polling,   TRUE  },
   { MOUSEDRV_WINNT,    &mousedrv_winnt,     TRUE  },
   { MOUSEDRV_WIN2K,    &mousedrv_win2k,     TRUE  },
   { MOUSEDRV_NONE,     &mousedrv_none,      TRUE  },
   { 0,                 NULL,                0     }
};



/* init_mouse:
 *  Helper for initialising the int 0x33 driver and installing an RMCB.
 */
static int init_mouse(void (*handler)(__dpmi_regs *r))
{
   __dpmi_regs r;
   int num_buttons;

   /* initialise the int 0x33 driver */
   r.x.ax = 0;
   __dpmi_int(0x33, &r); 

   if (r.x.ax == 0)
      return -1;

   num_buttons = r.x.bx;
   if (num_buttons == 0xFFFF)
      num_buttons = 2;

   /* create and activate a real mode callback */
   if (handler) {
      LOCK_VARIABLE(mouse_regs);

      #ifdef ALLEGRO_DJGPP

	 /* djgpp version uses libc routines to set up the RMCB */
	 {
	    LOCK_VARIABLE(mouse_seginfo);

	    mouse_seginfo.pm_offset = (int)handler;
	    mouse_seginfo.pm_selector = _my_cs();

	    if (_go32_dpmi_allocate_real_mode_callback_retf(&mouse_seginfo, &mouse_regs) != 0)
	       return -1;

	    r.x.ax = 0x0C;
	    r.x.cx = 0x7F;
	    r.x.dx = mouse_seginfo.rm_offset;
	    r.x.es = mouse_seginfo.rm_segment;
	    __dpmi_int(0x33, &r);
	 }

      #elif defined ALLEGRO_WATCOM

	 /* Watcom version relies on the DOS extender to do it for us */
	 {
	    struct SREGS sregs;
	    union REGS inregs, outregs;

	    inregs.w.ax = 0x0C;
	    inregs.w.cx = 0x7F;
	    inregs.x.edx = _allocate_real_mode_callback(handler, &mouse_regs);
	    segread(&sregs);
	    sregs.es = _my_cs();
	    int386x(0x33, &inregs, &outregs, &sregs);
	 }

      #else
	 #error unknown platform
      #endif
   }

   return num_buttons;
}



/* mick_handler:
 *  Movement callback for mickey-mode driver.
 */
static void mick_handler(__dpmi_regs *r)
{
   int x = (signed short)r->x.si;
   int y = (signed short)r->x.di;

   _mouse_b = r->x.bx;

   mymickey_x += x - mymickey_ox;
   mymickey_y += y - mymickey_oy;

   mymickey_ox = x;
   mymickey_oy = y;

   _mouse_x = MICKEY_TO_COORD_X(mouse_mx + x);
   _mouse_y = MICKEY_TO_COORD_Y(mouse_my + y);

   if ((_mouse_x < mouse_minx) || (_mouse_x > mouse_maxx) ||
       (_mouse_y < mouse_miny) || (_mouse_y > mouse_maxy)) {

      _mouse_x = CLAMP(mouse_minx, _mouse_x, mouse_maxx);
      _mouse_y = CLAMP(mouse_miny, _mouse_y, mouse_maxy);

      mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
      mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

      CLEAR_MICKEYS();
   }

   _handle_mouse_input();
}

END_OF_STATIC_FUNCTION(mick_handler);



/* mick_position:
 *  Sets the position of the mickey-mode mouse.
 */
static void mick_position(int x, int y)
{
   DISABLE();

   _mouse_x = x;
   _mouse_y = y;

   mouse_mx = COORD_TO_MICKEY_X(x);
   mouse_my = COORD_TO_MICKEY_Y(y);

   CLEAR_MICKEYS();

   mymickey_x = mymickey_y = 0;

   ENABLE();
}



/* mick_set_range:
 *  Sets the range of the mickey-mode mouse.
 */
static void mick_set_range(int x1, int y1, int x2, int y2)
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

   CLEAR_MICKEYS();

   ENABLE();
}



/* mick_set_speed:
 *  Sets the speed of the mickey-mode mouse.
 */
static void mick_set_speed(int xspeed, int yspeed)
{
   DISABLE();

   mouse_sx = MAX(1, xspeed);
   mouse_sy = MAX(1, yspeed);

   mouse_mx = COORD_TO_MICKEY_X(_mouse_x);
   mouse_my = COORD_TO_MICKEY_Y(_mouse_y);

   CLEAR_MICKEYS();

   ENABLE();
}



/* mick_get_mickeys:
 *  Reads the mickey-mode count.
 */
static void mick_get_mickeys(int *mickeyx, int *mickeyy)
{
   int temp_x = mymickey_x;
   int temp_y = mymickey_y;

   mymickey_x -= temp_x;
   mymickey_y -= temp_y;

   *mickeyx = temp_x;
   *mickeyy = temp_y;
}



/* mick_lock:
 *  Locks the mickeys stuff.
 */
static void mick_lock(void)
{
   LOCK_VARIABLE(mousedrv_mickeys);
   LOCK_VARIABLE(mouse_mx);
   LOCK_VARIABLE(mouse_my);
   LOCK_VARIABLE(mouse_sx);
   LOCK_VARIABLE(mouse_sy);
   LOCK_VARIABLE(mouse_minx);
   LOCK_VARIABLE(mouse_miny);
   LOCK_VARIABLE(mouse_maxx);
   LOCK_VARIABLE(mouse_maxy);
   LOCK_VARIABLE(mymickey_x);
   LOCK_VARIABLE(mymickey_y);
   LOCK_VARIABLE(mymickey_ox);
   LOCK_VARIABLE(mymickey_oy);
   LOCK_FUNCTION(mick_handler);
}



/* mick_init:
 *  Initialises the mickey-mode driver.
 */
static int mick_init(void)
{
   if (os_type == OSTYPE_WINNT)
      return -1;

   mick_lock();

   return init_mouse(mick_handler);
}



/* win2k_init:
 *  Initialises the win2k driver.
 */
static int win2k_init(void)
{
   /* only for use under Win2k */
   if (os_type != OSTYPE_WINNT)
      return -1;

   mick_lock();

   return init_mouse(mick_handler);
}



/* mick_exit:
 *  Shuts down the mickey-mode driver.
 */
static void mick_exit(void)
{
   __dpmi_regs r;

   r.x.ax = 0x0C;                /* install NULL callback */
   r.x.cx = 0;
   r.x.dx = 0;
   r.x.es = 0;
   __dpmi_int(0x33, &r);

   #ifdef ALLEGRO_DJGPP
      _go32_dpmi_free_real_mode_callback(&mouse_seginfo);
   #endif
}



/* int33_handler:
 *  Movement callback for the int 0x33 driver.
 */
static void int33_handler(__dpmi_regs *r)
{
   _mouse_x = r->x.cx / 8;
   _mouse_y = r->x.dx / 8;
   _mouse_b = r->x.bx;

   _handle_mouse_input();
}

END_OF_STATIC_FUNCTION(int33_handler);



/* int33_position:
 *  Sets the int 0x33 cursor position.
 */
static void int33_position(int x, int y)
{
   __dpmi_regs r;

   _mouse_x = x;
   _mouse_y = y;

   r.x.ax = 4;
   r.x.cx = x * 8;
   r.x.dx = y * 8;
   __dpmi_int(0x33, &r); 

   r.x.ax = 11;
   __dpmi_int(0x33, &r); 
}



/* int33_set_range:
 *  Sets the int 0x33 movement range.
 */
static void int33_set_range(int x1, int y1, int x2, int y2)
{
   __dpmi_regs r;

   r.x.ax = 7;                   /* set horizontal range */
   r.x.cx = x1 * 8;
   r.x.dx = x2 * 8;
   __dpmi_int(0x33, &r);

   r.x.ax = 8;                   /* set vertical range */
   r.x.cx = y1 * 8;
   r.x.dx = y2 * 8;
   __dpmi_int(0x33, &r);

   r.x.ax = 3;                   /* check the position */
   __dpmi_int(0x33, &r);

   _mouse_x = r.x.cx / 8;
   _mouse_y = r.x.dx / 8;
}



/* int33_set_speed:
 *  Sets the int 0x33 movement speed.
 */
static void int33_set_speed(int xspeed, int yspeed)
{
   __dpmi_regs r;

   r.x.ax = 15;
   r.x.cx = xspeed;
   r.x.dx = yspeed;
   __dpmi_int(0x33, &r); 
}



/* int33_get_mickeys:
 *  Reads the int 0x33 mickey count.
 */
static void int33_get_mickeys(int *mickeyx, int *mickeyy)
{
   __dpmi_regs r;

   r.x.ax = 11;
   __dpmi_int(0x33, &r); 

   *mickeyx = (signed short)r.x.cx;
   *mickeyy = (signed short)r.x.dx;
}



/* int33_init:
 *  Initialises the int 0x33 driver.
 */
static int int33_init(void)
{
   if (os_type == OSTYPE_WINNT)
      return -1;

   LOCK_VARIABLE(mousedrv_int33);
   LOCK_FUNCTION(int33_handler);

   return init_mouse(int33_handler);
}



/* int33_exit:
 *  Shuts down the int 0x33 driver.
 */ 
static void int33_exit(void)
{
   __dpmi_regs r;

   r.x.ax = 0x0C;                /* install NULL callback */
   r.x.cx = 0;
   r.x.dx = 0;
   r.x.es = 0;
   __dpmi_int(0x33, &r);

   r.x.ax = 15;                  /* reset sensitivity */
   r.x.cx = 8;
   r.x.dx = 16;
   __dpmi_int(0x33, &r);

   #ifdef ALLEGRO_DJGPP
      _go32_dpmi_free_real_mode_callback(&mouse_seginfo);
   #endif
}



/* polling_timer_poll:
 *  Handler for the periodic timer polling mode.
 */
static void polling_timer_poll(void)
{
   __dpmi_regs r;

   r.x.ax = 3; 
   __dpmi_int(0x33, &r);

   _mouse_b = r.x.bx;
   _mouse_x = r.x.cx / 8;
   _mouse_y = r.x.dx / 8;
}

END_OF_STATIC_FUNCTION(polling_timer_poll);



/* polling_init:
 *  Initialises the periodic timer driver.
 */
static int polling_init(void)
{
   if (os_type == OSTYPE_WINNT)
      return -1;

   LOCK_VARIABLE(mousedrv_polling);
   LOCK_FUNCTION(polling_timer_poll);

   return init_mouse(NULL);
}



/* winnt_init:
 *  Initialises the WinNT driver.
 */
static int winnt_init(void)
{
   /* only for use under WinNT */
   if (os_type != OSTYPE_WINNT)
      return -1;

   LOCK_VARIABLE(mousedrv_polling);
   LOCK_FUNCTION(polling_timer_poll);

   return init_mouse(NULL);
}



/* polling_exit:
 *  Shuts down the periodic timer driver.
 */
static void polling_exit(void)
{
   __dpmi_regs r;

   r.x.ax = 15;                  /* reset sensitivity */
   r.x.cx = 8;
   r.x.dx = 16;
   __dpmi_int(0x33, &r); 
}


