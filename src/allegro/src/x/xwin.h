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
 *      Wrappers for Xlib functions.
 *
 *      By Michael Bukin.
 *
 *      See readme.txt for copyright information.
 */

#ifndef __bma_xwin_h
#define __bma_xwin_h

#include "xalleg.h"

/* Defined in xmouse.c */
AL_VAR(int, _xwin_mouse_extended_range);

/* Defined in xwin.c.  */
AL_VAR(int, _xwin_last_line);
AL_VAR(int, _xwin_in_gfx_call);

/* The allegro X11 icon */
AL_VAR(void *, allegro_icon);


AL_FUNC(void, _xwin_set_warped_mouse_mode, (int permanent));
AL_FUNC(int, _xwin_open_display, (char *name));
AL_FUNC(void, _xwin_close_display, (void));
AL_FUNC(int, _xwin_create_window, (void));
AL_FUNC(void, _xwin_destroy_window, (void));
AL_FUNC(BITMAP*, _xwin_create_screen, (GFX_DRIVER *drv, int w, int h,
                                       int vw, int vh, int depth, int fullscreen));
AL_FUNC(void, _xwin_destroy_screen, (void));
AL_FUNC(BITMAP *, _xwin_rebuild_screen, (int w, int h, int color_depth));
AL_FUNC(void, _xwin_set_palette_range, (AL_CONST PALETTE p, int from, int to, int vsync));
AL_FUNC(void, _xwin_flush_buffers, (void));
AL_FUNC(void, _xwin_vsync, (void));
AL_FUNC(void, _xwin_handle_input, (void));
AL_FUNC(void, _xwin_enable_hardware_cursor, (int mode));
AL_FUNC(void, _xwin_redraw_window, (int x, int y, int w, int h));
AL_FUNC(int, _xwin_scroll_screen, (int x, int y));
AL_FUNC(void, _xwin_update_screen, (int x, int y, int w, int h));
AL_FUNC(void, _xwin_set_window_title, (AL_CONST char *name));
AL_FUNC(void, _xwin_change_keyboard_control, (int led, int on));
AL_FUNC(int, _xwin_get_pointer_mapping, (unsigned char map[], int nmap));
AL_FUNC(void, _xwin_init_keyboard_tables, (void));

#ifdef ALLEGRO_XWINDOWS_WITH_XCURSOR
AL_FUNC(int, _xwin_set_mouse_sprite, (struct BITMAP *sprite, int x, int y));
AL_FUNC(int, _xwin_show_mouse, (struct BITMAP *bmp, int x, int y));
AL_FUNC(void, _xwin_hide_mouse, (void));
AL_FUNC(void, _xwin_move_mouse, (int x, int y));
#endif

AL_FUNC(BITMAP*, _xdga_create_screen, (GFX_DRIVER *drv, int w, int h,
                                       int vw, int vh, int depth, int fullscreen));
AL_FUNC(void, _xdga_destroy_screen, (void));
AL_FUNC(void, _xdga_set_palette_range, (AL_CONST PALETTE p, int from, int to, int vsync));
AL_FUNC(int, _xdga_scroll_screen, (int x, int y));

AL_FUNC(GFX_MODE_LIST*, _xwin_fetch_mode_list, (void));

/* Defined in xvtable.c.  */
AL_FUNC(void, _xwin_replace_vtable, (struct GFX_VTABLE *vtable));
AL_FUNC(void, _xwin_drawing_mode, (void));
AL_FUNC(void, _xwin_lock, (BITMAP *bmp));
AL_FUNC(void, _xwin_unlock, (BITMAP *bmp));

/* Defined in xkeyboard.c. */
AL_FUNC(void, _xwin_keyboard_handler, (XKeyEvent *event, int dga2_hack));
AL_FUNC(void, _xwin_get_keyboard_mapping, (void));
AL_FUNC(void, _xwin_keyboard_focus_handler, (XFocusChangeEvent *event));

#endif /* !__bma_xwin_h */
