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
 *      Stuff for BeOS.
 *
 *      By Jason Wilkins.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif

SYSTEM_DRIVER system_beos = {
   SYSTEM_BEOS,
   empty_string,
   empty_string,
   "System",
   be_sys_init,
   be_sys_exit,
   be_sys_get_executable_name,
   be_sys_find_resource,
   be_sys_set_window_title,
   be_sys_set_close_button_callback,
   be_sys_message,
   NULL,  // AL_METHOD(void, assert, (char *msg));
   NULL,  // AL_METHOD(void, save_console_state, (void));
   NULL,  // AL_METHOD(void, restore_console_state, (void));
   NULL,  // AL_METHOD(struct BITMAP *, create_bitmap, (int color_depth, int width, int height));
   NULL,  // AL_METHOD(void, created_bitmap, (struct BITMAP *bmp));
   NULL,  // AL_METHOD(struct BITMAP *, create_sub_bitmap, (struct BITMAP *parent, int x, int y, int width, int height));
   NULL,  // AL_METHOD(void, created_sub_bitmap, (struct BITMAP *bmp, struct BITMAP *parent));
   NULL,  // AL_METHOD(int, destroy_bitmap, (struct BITMAP *bitmap));
   NULL,  // AL_METHOD(void, read_hardware_palette, (void));
   NULL,  // AL_METHOD(void, set_palette_range, (struct RGB *p, int from, int to, int vsync));
   NULL,  // AL_METHOD(struct GFX_VTABLE *, get_vtable, (int color_depth));
   be_sys_set_display_switch_mode,
   NULL,  // AL_METHOD(void, display_switch_lock, (int lock));
   be_sys_desktop_color_depth,
   be_sys_get_desktop_resolution,
   be_sys_get_gfx_safe_mode,
   be_sys_yield_timeslice,
   be_sys_create_mutex,
   be_sys_destroy_mutex,
   be_sys_lock_mutex,
   be_sys_unlock_mutex,
   NULL,  // AL_METHOD(_DRIVER_INFO *, gfx_drivers, (void));
   NULL,  // AL_METHOD(_DRIVER_INFO *, digi_drivers, (void));
   NULL,  // AL_METHOD(_DRIVER_INFO *, midi_drivers, (void));
   NULL,  // AL_METHOD(_DRIVER_INFO *, keyboard_drivers, (void));
   NULL,  // AL_METHOD(_DRIVER_INFO *, mouse_drivers, (void));
   NULL,  // AL_METHOD(_DRIVER_INFO *, joystick_drivers, (void));
   NULL   // AL_METHOD(_DRIVER_INFO *, timer_drivers, (void));
};
