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



GFX_DRIVER gfx_beos_bwindowscreen_accel = {
   GFX_BWINDOWSCREEN_ACCEL,           // int id;
   empty_string,                      // char *name;
   empty_string,                      // char *desc;
   "BWindowScreen accel",             // char *ascii_name;
   be_gfx_bwindowscreen_accel_init,   // AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth));
   be_gfx_bwindowscreen_exit,         // AL_METHOD(void, exit, (struct BITMAP *b));
   be_gfx_bwindowscreen_scroll,       // AL_METHOD(int, scroll, (int x, int y));
   be_gfx_vsync,                      // AL_METHOD(void, vsync, (void));
   be_gfx_bwindowscreen_set_palette,  // AL_METHOD(void, set_palette, (struct RGB *p, int from, int to, int vsync));
   be_gfx_bwindowscreen_request_scroll,// AL_METHOD(int, request_scroll, (int x, int y));
   be_gfx_bwindowscreen_poll_scroll,  // AL_METHOD(int, poll_scroll, (void));
   NULL,                              // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,                              // AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap));
   be_gfx_bwindowscreen_request_video_bitmap,// AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                              // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                              // AL_METHOD(void, hide_mouse, (void));
   NULL,                              // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                              // AL_METHOD(void, drawing_mode, (void));
   NULL,                              // AL_METHOD(void, save_state, (void));
   NULL,                              // AL_METHOD(void, restore_state, (void));
   NULL,                              // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   be_gfx_bwindowscreen_fetch_mode_list,// AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                              // int w, h;  /* physical (not virtual!) screen size */
   TRUE,                              // int linear;  /* true if video memory is linear */
   0,                                 // long bank_size;  /* bank size, in bytes */
   0,                                 // long bank_gran;  /* bank granularity, in bytes */
   0,                                 // long vid_mem;  /* video memory size, in bytes */
   0,                                 // long vid_phys_base;  /* physical address of video memory */
   FALSE                              // int windowed;  /* true if driver runs windowed */
};



GFX_DRIVER gfx_beos_bwindowscreen = {
   GFX_BWINDOWSCREEN,                 // int id;
   empty_string,                      // char *name;
   empty_string,                      // char *desc;
   "BWindowScreen",                   // char *ascii_name;
   be_gfx_bwindowscreen_init,         // AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth));
   be_gfx_bwindowscreen_exit,         // AL_METHOD(void, exit, (struct BITMAP *b));
   be_gfx_bwindowscreen_scroll,       // AL_METHOD(int, scroll, (int x, int y));
   be_gfx_vsync,                      // AL_METHOD(void, vsync, (void));
   be_gfx_bwindowscreen_set_palette,  // AL_METHOD(void, set_palette, (struct RGB *p, int from, int to, int vsync));
   be_gfx_bwindowscreen_request_scroll,// AL_METHOD(int, request_scroll, (int x, int y));
   be_gfx_bwindowscreen_poll_scroll,  // AL_METHOD(int, poll_scroll, (void));
   NULL,                              // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,                              // AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap));
   be_gfx_bwindowscreen_request_video_bitmap,// AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                              // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                              // AL_METHOD(void, hide_mouse, (void));
   NULL,                              // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                              // AL_METHOD(void, drawing_mode, (void));
   NULL,                              // AL_METHOD(void, save_state, (void));
   NULL,                              // AL_METHOD(void, restore_state, (void));
   NULL,                              // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   be_gfx_bwindowscreen_fetch_mode_list,// AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                              // int w, h;  /* physical (not virtual!) screen size */
   TRUE,                              // int linear;  /* true if video memory is linear */
   0,                                 // long bank_size;  /* bank size, in bytes */
   0,                                 // long bank_gran;  /* bank granularity, in bytes */
   0,                                 // long vid_mem;  /* video memory size, in bytes */
   0,                                 // long vid_phys_base;  /* physical address of video memory */
   FALSE                              // int windowed;  /* true if driver runs windowed */
};



GFX_DRIVER gfx_beos_bdirectwindow = {
   GFX_BDIRECTWINDOW,                 // int id;
   empty_string,                      // char *name;
   empty_string,                      // char *desc;
   "BDirectWindow",                   // char *ascii_name;
   be_gfx_bdirectwindow_init,         // AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth));
   be_gfx_bdirectwindow_exit,         // AL_METHOD(void, exit, (struct BITMAP *b));
   NULL,                              // AL_METHOD(int, scroll, (int x, int y));
   be_gfx_vsync,                      // AL_METHOD(void, vsync, (void));
   be_gfx_bdirectwindow_set_palette,  // AL_METHOD(void, set_palette, (struct RGB *p, int from, int to, int vsync));
   NULL,                              // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                              // AL_METHOD(int, poll_scroll, (void));
   NULL,                              // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,                              // AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                              // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                              // AL_METHOD(void, hide_mouse, (void));
   NULL,                              // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                              // AL_METHOD(void, drawing_mode, (void));
   NULL,                              // AL_METHOD(void, save_state, (void));
   NULL,                              // AL_METHOD(void, restore_state, (void));
   NULL,                              // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                              // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                              // int w, h;  /* physical (not virtual!) screen size */
   TRUE,                              // int linear;  /* true if video memory is linear */
   0,                                 // long bank_size;  /* bank size, in bytes */
   0,                                 // long bank_gran;  /* bank granularity, in bytes */
   0,                                 // long vid_mem;  /* video memory size, in bytes */
   0,                                 // long vid_phys_base;  /* physical address of video memory */
   TRUE                               // int windowed;  /* true if driver runs windowed */
};



GFX_DRIVER gfx_beos_bwindow = {
   GFX_BWINDOW,                       // int id;
   empty_string,                      // char *name;
   empty_string,                      // char *desc;
   "BWindow",                         // char *ascii_name;
   be_gfx_bwindow_init,               // AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth));
   be_gfx_bwindow_exit,               // AL_METHOD(void, exit, (struct BITMAP *b));
   NULL,                              // AL_METHOD(int, scroll, (int x, int y));
   be_gfx_vsync,                      // AL_METHOD(void, vsync, (void));
   be_gfx_bwindow_set_palette,        // AL_METHOD(void, set_palette, (struct RGB *p, int from, int to, int vsync));
   NULL,                              // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                              // AL_METHOD(int, poll_scroll, (void));
   NULL,                              // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,                              // AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                              // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                              // AL_METHOD(void, hide_mouse, (void));
   NULL,                              // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                              // AL_METHOD(void, drawing_mode, (void));
   NULL,                              // AL_METHOD(void, save_state, (void));
   NULL,                              // AL_METHOD(void, restore_state, (void));
   NULL,                              // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                              // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                              // int w, h;  /* physical (not virtual!) screen size */
   TRUE,                              // int linear;  /* true if video memory is linear */
   0,                                 // long bank_size;  /* bank size, in bytes */
   0,                                 // long bank_gran;  /* bank granularity, in bytes */
   0,                                 // long vid_mem;  /* video memory size, in bytes */
   0,                                 // long vid_phys_base;  /* physical address of video memory */
   TRUE                               // int windowed;  /* true if driver runs windowed */
};



GFX_DRIVER gfx_beos_overlay = {
   GFX_BWINDOW_OVERLAY,               // int id;
   empty_string,                      // char *name;
   empty_string,                      // char *desc;
   "BWindow (overlay)",               // char *ascii_name;
   be_gfx_overlay_init,               // AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth));
   be_gfx_overlay_exit,               // AL_METHOD(void, exit, (struct BITMAP *b));
   NULL,                              // AL_METHOD(int, scroll, (int x, int y));
   be_gfx_vsync,                      // AL_METHOD(void, vsync, (void));
   NULL,                              // AL_METHOD(void, set_palette, (struct RGB *p, int from, int to, int vsync));
   NULL,                              // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                              // AL_METHOD(int, poll_scroll, (void));
   NULL,                              // AL_METHOD(void, enable_triple_buffer, (void));
   NULL,                              // AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height));
   NULL,                              // AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap));
   NULL,                              // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                              // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                              // AL_METHOD(void, hide_mouse, (void));
   NULL,                              // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                              // AL_METHOD(void, drawing_mode, (void));
   NULL,                              // AL_METHOD(void, save_state, (void));
   NULL,                              // AL_METHOD(void, restore_state, (void));
   NULL,                              // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                              // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                              // int w, h;  /* physical (not virtual!) screen size */
   TRUE,                              // int linear;  /* true if video memory is linear */
   0,                                 // long bank_size;  /* bank size, in bytes */
   0,                                 // long bank_gran;  /* bank granularity, in bytes */
   0,                                 // long vid_mem;  /* video memory size, in bytes */
   0,                                 // long vid_phys_base;  /* physical address of video memory */
   FALSE                              // int windowed;  /* true if driver runs windowed */
};

