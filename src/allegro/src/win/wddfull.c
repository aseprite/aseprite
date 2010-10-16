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
 *      DirectDraw gfx fullscreen drivers.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"


static struct BITMAP *init_directx_accel(int w, int h, int v_w, int v_h, int color_depth);
static struct BITMAP *init_directx_soft(int w, int h, int v_w, int v_h, int color_depth);
static struct BITMAP *init_directx_safe(int w, int h, int v_w, int v_h, int color_depth);
static void finalize_fullscreen_init(void);
static void switch_in_fullscreen(void);
static void switch_out_fullscreen(void);


GFX_DRIVER gfx_directx_accel =
{
   GFX_DIRECTX_ACCEL,
   empty_string,
   empty_string,
   "DirectDraw accel",
   init_directx_accel,
   gfx_directx_exit,
   NULL,                         // AL_METHOD(int, scroll, (int x, int y)); 
   gfx_directx_sync,
   gfx_directx_set_palette,
   NULL,                         // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                         // AL_METHOD(int, poll_scroll, (void));
   NULL,                         // AL_METHOD(void, enable_triple_buffer, (void));
   gfx_directx_create_video_bitmap,
   gfx_directx_destroy_video_bitmap,
   gfx_directx_show_video_bitmap,
   gfx_directx_request_video_bitmap,
   gfx_directx_create_system_bitmap,
   gfx_directx_destroy_system_bitmap,
   NULL,                         // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                         // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                         // AL_METHOD(void, hide_mouse, (void));
   NULL,                         // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                         // AL_METHOD(void, drawing_mode, (void));
   NULL,                         // AL_METHOD(void, save_video_state, (void*));
   NULL,                         // AL_METHOD(void, restore_video_state, (void*));
   NULL,                         // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   gfx_directx_fetch_mode_list,
   0, 0,                         // physical (not virtual!) screen size
   TRUE,                         // true if video memory is linear
   0,                            // bank size, in bytes
   0,                            // bank granularity, in bytes
   0,                            // video memory size, in bytes
   0,                            // physical address of video memory
   FALSE                         // true if driver runs windowed
};


GFX_DRIVER gfx_directx_soft =
{
   GFX_DIRECTX_SOFT,
   empty_string,
   empty_string,
   "DirectDraw soft",
   init_directx_soft,
   gfx_directx_exit,
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   gfx_directx_sync,
   gfx_directx_set_palette,
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int, poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   gfx_directx_create_video_bitmap,
   gfx_directx_destroy_video_bitmap,
   gfx_directx_show_video_bitmap,
   gfx_directx_request_video_bitmap,
   gfx_directx_create_system_bitmap,
   gfx_directx_destroy_system_bitmap,
   NULL,                        // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                        // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                        // AL_METHOD(void, hide_mouse, (void));
   NULL,                        // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   gfx_directx_fetch_mode_list,
   0, 0,                        // physical (not virtual!) screen size
   TRUE,                        // true if video memory is linear
   0,                           // bank size, in bytes
   0,                           // bank granularity, in bytes
   0,                           // video memory size, in bytes
   0,                           // physical address of video memory
   FALSE                        // true if driver runs windowed
};


GFX_DRIVER gfx_directx_safe =
{
   GFX_DIRECTX_SAFE,
   empty_string,
   empty_string,
   "DirectDraw safe",
   init_directx_safe,
   gfx_directx_exit,
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   gfx_directx_sync,
   gfx_directx_set_palette,
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int, poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   NULL, NULL, NULL, NULL, NULL, NULL,
   NULL,                        // AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus));
   NULL,                        // AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y));
   NULL,                        // AL_METHOD(void, hide_mouse, (void));
   NULL,                        // AL_METHOD(void, move_mouse, (int x, int y));
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   gfx_directx_fetch_mode_list,
   0, 0,                        // physical (not virtual!) screen size
   TRUE,                        // true if video memory is linear
   0,                           // bank size, in bytes
   0,                           // bank granularity, in bytes
   0,                           // video memory size, in bytes
   0,                           // physical address of video memory
   FALSE                        // true if driver runs windowed
};


static WIN_GFX_DRIVER win_gfx_fullscreen =
{
   FALSE,                       // true if driver has backing store
   switch_in_fullscreen,
   switch_out_fullscreen,
   NULL,                        // AL_METHOD(void, enter_sysmode, (void));
   NULL,                        // AL_METHOD(void, exit_sysmode, (void));
   NULL,                        // AL_METHOD(void, move, (int x, int y, int w, int h));
   NULL,                        // AL_METHOD(void, iconify, (void));
   NULL                         // AL_METHOD(void, paint, (RECT *));
};


/* When switching away from an 8-bit fullscreen program, the palette is lost.
 * This stores the palette so it can be restored when switching back in.
 */
static PALETTE wddfull_saved_palette;



/* init_directx_accel:
 *  Initializes the DirectDraw fullscreen hardware-accelerated driver.
 */
static struct BITMAP *init_directx_accel(int w, int h, int v_w, int v_h, int color_depth)
{
   struct BITMAP *bmp;

   _enter_critical();

   bmp = gfx_directx_init(&gfx_directx_accel, w, h, v_w, v_h, color_depth);

   if (bmp) {
      gfx_directx_enable_acceleration(&gfx_directx_accel);
      gfx_directx_enable_triple_buffering(&gfx_directx_accel);
      finalize_fullscreen_init();
   }

   _exit_critical();

   return bmp;
}



/* init_directx_soft:
 *  Initializes the DirectDraw fullscreen software-only driver.
 */
static struct BITMAP *init_directx_soft(int w, int h, int v_w, int v_h, int color_depth)
{
   struct BITMAP *bmp;

   _enter_critical();

   bmp = gfx_directx_init(&gfx_directx_soft, w, h, v_w, v_h, color_depth);

   if (bmp) {
      gfx_directx_enable_triple_buffering(&gfx_directx_soft);
      finalize_fullscreen_init();
   }

   _exit_critical();

   return bmp;
}



/* init_directx_safe:
 *  Initializes the DirectDraw fullscreen safe driver.
 */
static struct BITMAP *init_directx_safe(int w, int h, int v_w, int v_h, int color_depth)
{
   struct BITMAP *bmp;

   _enter_critical();

   bmp = gfx_directx_init(&gfx_directx_safe, w, h, v_w, v_h, color_depth);

   if (bmp)
      finalize_fullscreen_init();

   _exit_critical();

   return bmp;
}



/* finalize_fullscreen_init:
 *  Finalizes initialization of the three fullscreen drivers.
 */
static void finalize_fullscreen_init(void)
{
   /* connect to the system driver */
   win_gfx_driver = &win_gfx_fullscreen;

   /* set the default switching policy */
   set_display_switch_mode(SWITCH_AMNESIA);

   /* grab input devices */
   win_grab_input();
}



/* switch_in_fullscreen:
 *  Handles screen switched in.
 */
static void switch_in_fullscreen(void)
{
   restore_all_ddraw_surfaces();

   if (_color_depth == 8)
     set_palette(wddfull_saved_palette);
}



/* switch_out_fullscreen:
 *  Handles screen switched out.
 */
static void switch_out_fullscreen(void)
{
   if (_color_depth == 8)
      get_palette(wddfull_saved_palette);
}
