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
 *      Mouse routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_MOUSE_H
#define ALLEGRO_MOUSE_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

#define MOUSEDRV_AUTODETECT  -1
#define MOUSEDRV_NONE         0


typedef struct MOUSE_DRIVER
{
   int  id;
   AL_CONST char *name;
   AL_CONST char *desc;
   AL_CONST char *ascii_name;
   AL_METHOD(int,  init, (void));
   AL_METHOD(void, exit, (void));
   AL_METHOD(void, poll, (void));
   AL_METHOD(void, timer_poll, (void));
   AL_METHOD(void, position, (int x, int y));
   AL_METHOD(void, set_range, (int x1, int y_1, int x2, int y2));
   AL_METHOD(void, set_speed, (int xspeed, int yspeed));
   AL_METHOD(void, get_mickeys, (int *mickeyx, int *mickeyy));
   AL_METHOD(int,  analyse_data, (AL_CONST char *buffer, int size));
   AL_METHOD(void, enable_hardware_cursor, (int mode));
   AL_METHOD(int,  select_system_cursor, (int cursor));
} MOUSE_DRIVER;


AL_VAR(MOUSE_DRIVER, mousedrv_none);
AL_VAR(MOUSE_DRIVER *, mouse_driver);
AL_ARRAY(_DRIVER_INFO, _mouse_driver_list);

AL_FUNC(int, install_mouse, (void));
AL_FUNC(void, remove_mouse, (void));

AL_FUNC(int, poll_mouse, (void));
AL_FUNC(int, mouse_needs_poll, (void));

AL_FUNC(void, enable_hardware_cursor, (void));
AL_FUNC(void, disable_hardware_cursor, (void));

/* Mouse cursors */
#define MOUSE_CURSOR_NONE        0
#define MOUSE_CURSOR_ALLEGRO     1
#define MOUSE_CURSOR_ARROW       2
#define MOUSE_CURSOR_BUSY        3
#define MOUSE_CURSOR_QUESTION    4
#define MOUSE_CURSOR_EDIT        5
#define AL_NUM_MOUSE_CURSORS        6

AL_VAR(struct BITMAP *, mouse_sprite);
AL_VAR(int, mouse_x_focus);
AL_VAR(int, mouse_y_focus);

AL_VAR(volatile int, mouse_x);
AL_VAR(volatile int, mouse_y);
AL_VAR(volatile int, mouse_z);
AL_VAR(volatile int, mouse_w);
AL_VAR(volatile int, mouse_b);
AL_VAR(volatile int, mouse_pos);

AL_VAR(volatile int, freeze_mouse_flag);

#define MOUSE_FLAG_MOVE             1
#define MOUSE_FLAG_LEFT_DOWN        2
#define MOUSE_FLAG_LEFT_UP          4
#define MOUSE_FLAG_RIGHT_DOWN       8
#define MOUSE_FLAG_RIGHT_UP         16
#define MOUSE_FLAG_MIDDLE_DOWN      32
#define MOUSE_FLAG_MIDDLE_UP        64
#define MOUSE_FLAG_MOVE_Z           128
#define MOUSE_FLAG_MOVE_W           256

AL_FUNCPTR(void, mouse_callback, (int flags));

AL_FUNC(void, show_mouse, (struct BITMAP *bmp));
AL_FUNC(void, scare_mouse, (void));
AL_FUNC(void, scare_mouse_area, (int x, int y, int w, int h));
AL_FUNC(void, unscare_mouse, (void));
AL_FUNC(void, position_mouse, (int x, int y));
AL_FUNC(void, position_mouse_z, (int z));
AL_FUNC(void, position_mouse_w, (int w));
AL_FUNC(void, set_mouse_range, (int x1, int y_1, int x2, int y2));
AL_FUNC(void, set_mouse_speed, (int xspeed, int yspeed));
AL_FUNC(void, select_mouse_cursor, (int cursor));
AL_FUNC(void, set_mouse_cursor_bitmap, (int cursor, struct BITMAP *bmp));
AL_FUNC(void, set_mouse_sprite_focus, (int x, int y));
AL_FUNC(void, get_mouse_mickeys, (int *mickeyx, int *mickeyy));
AL_FUNC(void, set_mouse_sprite, (struct BITMAP *sprite));
AL_FUNC(int, show_os_cursor, (int cursor));
AL_FUNC(int, mouse_on_screen, (void));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_MOUSE_H */


