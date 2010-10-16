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
 *      DirectDraw overlay gfx driver.
 *
 *      By Stefan Schimanski.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"

#define PREFIX_I                "al-wddovl INFO: "
#define PREFIX_W                "al-wddovl WARNING: "
#define PREFIX_E                "al-wddovl ERROR: "


static int gfx_directx_show_video_bitmap_ovl(struct BITMAP *bitmap);
static int gfx_directx_request_video_bitmap_ovl(struct BITMAP *bitmap);
static struct BITMAP *init_directx_ovl(int w, int h, int v_w, int v_h, int color_depth);
static void gfx_directx_ovl_exit(struct BITMAP *b);


GFX_DRIVER gfx_directx_ovl =
{
   GFX_DIRECTX_OVL,
   empty_string,
   empty_string,
   "DirectDraw overlay",
   init_directx_ovl,
   gfx_directx_ovl_exit,
   NULL,                        // AL_METHOD(int, scroll, (int x, int y)); 
   gfx_directx_sync,
   gfx_directx_set_palette,
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // gfx_directx_poll_scroll,
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   gfx_directx_create_video_bitmap,
   gfx_directx_destroy_video_bitmap,
   gfx_directx_show_video_bitmap_ovl,
   gfx_directx_request_video_bitmap_ovl,
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
   NULL,                        // AL_METHOD(int, fetch_mode_list, (void));
   0, 0,                        // int w, h;                     /* physical (not virtual!) screen size */
   TRUE,                        // int linear;                   /* true if video memory is linear */
   0,                           // long bank_size;               /* bank size, in bytes */
   0,                           // long bank_gran;               /* bank granularity, in bytes */
   0,                           // long vid_mem;                 /* video memory size, in bytes */
   0,                           // long vid_phys_base;           /* physical address of video memory */
   TRUE                         // int windowed;                 /* true if driver runs windowed */
};


static void switch_in_overlay(void);
static void show_overlay(void);
static void move_overlay(int, int, int, int);
static void hide_overlay(void);
static void paint_overlay(RECT *);


static WIN_GFX_DRIVER win_gfx_driver_overlay =
{
   TRUE,
   switch_in_overlay,
   NULL,                        // AL_METHOD(void, switch_out, (void));
   NULL,                        // AL_METHOD(void, enter_sysmode, (void));
   NULL,                        // AL_METHOD(void, exit_sysmode, (void));
   move_overlay,
   hide_overlay,
   paint_overlay
};


static char gfx_driver_desc[256] = EMPTY_STRING;
static DDRAW_SURFACE *overlay_surface = NULL;
static BOOL overlay_visible = FALSE;
static HBRUSH overlay_brush;



/* switch_in_overlay:
 *  Handles window switched in.
 */
static void switch_in_overlay(void)
{
   restore_all_ddraw_surfaces();
   show_overlay();
}



/* update_overlay:
 *  Moves and resizes the overlay surface to make it fit
 *  into the specified area.
 */
static int update_overlay(int x, int y, int w, int h)
{
   HRESULT hr;
   RECT dest_rect;

   dest_rect.left = x;
   dest_rect.top = y;
   dest_rect.right = x + w;
   dest_rect.bottom = y + h;

   /* show the overlay surface */
   hr = IDirectDrawSurface2_UpdateOverlay(overlay_surface->id, NULL,
                                          gfx_directx_primary_surface->id, &dest_rect,
                                          DDOVER_SHOW | DDOVER_KEYDEST, NULL);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't display overlay (%x)\n", hr);
      IDirectDrawSurface2_UpdateOverlay(overlay_surface->id, NULL,
                                        gfx_directx_primary_surface->id, NULL,
                                        DDOVER_HIDE, NULL);
      return -1;
   }

   return 0;
}



/* show_overlay:
 *  Makes the overlay surface visible on the primary surface.
 */
static void show_overlay(void)
{
   if (overlay_surface) {
      overlay_visible = TRUE;
      update_overlay(wnd_x, wnd_y, wnd_width, wnd_height);
   }
}



/* move_overlay:
 *  Moves the overlay surface.
 */
static void move_overlay(int x, int y, int w, int h)
{
   RECT window_rect;
   HWND allegro_wnd = win_get_window();
   int xmod;

   /* handle hardware limitations */
   if ((ddcaps.dwCaps & DDCAPS_ALIGNBOUNDARYDEST) && (xmod = x%ddcaps.dwAlignBoundaryDest)) {
      GetWindowRect(allegro_wnd, &window_rect);
      SetWindowPos(allegro_wnd, 0, window_rect.left + xmod, window_rect.top,
                   0, 0, SWP_NOZORDER | SWP_NOSIZE);
   }
   else if (overlay_visible)
      update_overlay(x, y, w, h);
}



/* hide_overlay:
 *  Makes the overlay surface invisible on the primary surface.
 */
static void hide_overlay(void)
{
   if (overlay_visible) {
      overlay_visible = FALSE;

      IDirectDrawSurface2_UpdateOverlay(overlay_surface->id, NULL,
                                        gfx_directx_primary_surface->id, NULL,
                                        DDOVER_HIDE, NULL);
   }
}



/* paint_overlay:
 *  Handles window paint events.
 */
static void paint_overlay(RECT *rect)
{
   /* We may have lost the DirectDraw surfaces, for example
    * after the monitor has gone to low power.
    */
   if (IDirectDrawSurface2_IsLost(gfx_directx_primary_surface->id))
      switch_in_overlay();
}



/* wnd_set_windowed_coop:
 *  Sets the DirectDraw cooperative level to normal.
 */
static int wnd_set_windowed_coop(void)
{
   HRESULT hr;
   HWND allegro_wnd = win_get_window();

   hr = IDirectDraw2_SetCooperativeLevel(directdraw, allegro_wnd, DDSCL_NORMAL);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "SetCooperative level = %s (%x), hwnd = %x\n", win_err_str(hr), hr, allegro_wnd);
      return -1;
   }

   return 0;
}



/* gfx_directx_setup_driver_desc:
 *  Sets up the driver description string.
 */
static void gfx_directx_setup_driver_desc(void)
{
   char tmp[256];

   uszprintf(gfx_driver_desc, sizeof(gfx_driver_desc), 
	     uconvert_ascii("DirectDraw, in matching, %d bpp overlay", tmp),
	     _win_desktop_depth);
   
   gfx_directx_ovl.desc = gfx_driver_desc;
}



/* gfx_directx_show_video_bitmap_ovl:
 */
static int gfx_directx_show_video_bitmap_ovl(struct BITMAP *bitmap)
{
   if (gfx_directx_show_video_bitmap(bitmap) != 0) {
      return -1;
   }
   else {
      if (overlay_visible)
         update_overlay(wnd_x, wnd_y, wnd_width, wnd_height);
      return 0;
   } 
}



/* gfx_directx_request_video_bitmap_ovl:
 */
static int gfx_directx_request_video_bitmap_ovl(struct BITMAP *bitmap)
{
   if (gfx_directx_request_video_bitmap(bitmap) != 0) {
      return -1;
   }
   else {
      if (overlay_visible)
         update_overlay(wnd_x, wnd_y, wnd_width, wnd_height);
      return 0;
   } 
}



/* create_overlay:
 *  Creates the overlay surface.
 */
static int create_overlay(int w, int h, int color_depth)
{
   overlay_surface = gfx_directx_create_surface(w, h, ddpixel_format, DDRAW_SURFACE_OVERLAY);
   
   if (!overlay_surface) {
      _TRACE(PREFIX_E "Can't create overlay surface.\n");
      return -1;
   }

   return 0;
}



/* init_directx_ovl:
 *  Initializes the driver.
 */
static struct BITMAP *init_directx_ovl(int w, int h, int v_w, int v_h, int color_depth)
{
   HRESULT hr;
   DDCOLORKEY key;
   HWND allegro_wnd = win_get_window();

   /* overlay would allow scrolling on some cards, but it isn't implemented yet */
   if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported virtual resolution"));
      return NULL;
   }

   _enter_critical();

   /* init DirectX */
   if (init_directx() != 0)
      goto Error;
   if ((ddcaps.dwCaps & DDCAPS_OVERLAY) == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Overlays not supported"));
      goto Error;
   }
   if (gfx_directx_compare_color_depth(color_depth) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      goto Error;
   }
   if (wnd_call_proc(wnd_set_windowed_coop) != 0)
      goto Error;
   if (finalize_directx_init() != 0)
      goto Error;

   /* Paint window background with overlay color key. */
   overlay_brush = CreateSolidBrush(MASK_COLOR_32);
   SetClassLong(allegro_wnd, GCL_HBRBACKGROUND, (LONG) overlay_brush);

   if (adjust_window(w, h) != 0) {
      _TRACE(PREFIX_E "window size not supported.\n");
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      goto Error;
   }

   /* create surfaces */
   if (gfx_directx_create_primary() != 0)
      goto Error;

   if (create_overlay(w, h, color_depth) != 0)
      goto Error;

   /* Handle hardware limitations: according to the DirectX SDK, "these restrictions
    * can vary depending on the pixel formats of the overlay and primary surface", so
    * we handle them after creating the surfaces.
    */
   ddcaps.dwSize = sizeof(ddcaps);
   hr = IDirectDraw2_GetCaps(directdraw, &ddcaps, NULL);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't get driver caps\n");
      goto Error;
   }

   if (ddcaps.dwCaps & DDCAPS_ALIGNSIZESRC) {
      if (w%ddcaps.dwAlignSizeSrc) {
         _TRACE(PREFIX_E "Alignment requirement not met: source size %d\n", ddcaps.dwAlignSizeSrc);
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
         goto Error;
      }
   } 
   else if (ddcaps.dwCaps & DDCAPS_ALIGNSIZEDEST) {
      if (w%ddcaps.dwAlignSizeDest) {
         _TRACE(PREFIX_E "Alignment requirement not met: dest size %d\n", ddcaps.dwAlignSizeDest);
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
         goto Error;
      }
   }

   /* set color format */
   if (color_depth == 8) {
      if (gfx_directx_create_palette(overlay_surface) != 0)
	 goto Error;
   }
   else {
      if (gfx_directx_update_color_format(overlay_surface, color_depth) != 0)
         goto Error;
   }

   /* set up Allegro gfx driver */
   gfx_directx_setup_driver_desc();

   if (gfx_directx_setup_driver(&gfx_directx_ovl, w, h, color_depth) != 0)
      goto Error;

   gfx_directx_forefront_bitmap = gfx_directx_make_bitmap_from_surface(overlay_surface, w, h, BMP_ID_VIDEO);

   /* set the overlay color key */
   key.dwColorSpaceLowValue = gfx_directx_forefront_bitmap->vtable->mask_color;
   key.dwColorSpaceHighValue = gfx_directx_forefront_bitmap->vtable->mask_color;

   hr = IDirectDrawSurface2_SetColorKey(gfx_directx_primary_surface->id, DDCKEY_DESTOVERLAY, &key);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't set overlay dest color key\n");
      goto Error;
   }

   /* display the overlay surface */
   show_overlay();

   /* use hardware accelerated primitives */
   gfx_directx_enable_acceleration(&gfx_directx_ovl);

   /* use triple buffering */
   gfx_directx_enable_triple_buffering(&gfx_directx_ovl);

   /* connect to the system driver */
   win_gfx_driver = &win_gfx_driver_overlay;

   /* set default switching policy */
   set_display_switch_mode(SWITCH_PAUSE);

   /* grab input devices */
   win_grab_input();

   _exit_critical();

   return gfx_directx_forefront_bitmap;

 Error:
   _exit_critical();

   /* release the DirectDraw object */
   gfx_directx_ovl_exit(NULL);

   return NULL;
}



/* gfx_directx_ovl_exit:
 *  Shuts down the driver.
 */
static void gfx_directx_ovl_exit(struct BITMAP *bmp)
{
   HWND allegro_wnd = win_get_window();
   _enter_gfx_critical();

   if (bmp) {
      save_window_pos();
      clear_bitmap(bmp);
   }

   /* disconnect from the system driver */
   win_gfx_driver = NULL;

   /* destroy the overlay surface */
   if (overlay_surface) {
      hide_overlay();
      SetClassLong(allegro_wnd, GCL_HBRBACKGROUND, (LONG) NULL);
      DeleteObject(overlay_brush);
      gfx_directx_destroy_surface(overlay_surface);
      overlay_surface = NULL;
      gfx_directx_forefront_bitmap = NULL;
   }

   gfx_directx_exit(NULL);

   _exit_gfx_critical();
}

