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
 *      DirectDraw windowed gfx driver.
 *
 *      By Isaac Cruz.
 *
 *      General overhaul by Eric Botcazou.
 *
 *      Added resize support by David Capello.
 *
 *      See readme.txt for copyright information.
 */


#include "wddraw.h"

#define PREFIX_I                "al-wddwin INFO: "
#define PREFIX_W                "al-wddwin WARNING: "
#define PREFIX_E                "al-wddwin ERROR: "


char *_al_wd_dirty_lines = NULL;  /* used in WRITE_BANK() */
void (*_al_wd_update_window) (RECT *rect) = NULL;  /* window updater */


static void gfx_directx_set_palette_win(AL_CONST struct RGB *p, int from, int to, int vsync);
static BITMAP *gfx_directx_create_video_bitmap_win(int width, int height);
static void gfx_directx_destroy_video_bitmap_win(BITMAP *bmp);
static int gfx_directx_show_video_bitmap_win(struct BITMAP *bmp);
static BITMAP *gfx_directx_win_init(int w, int h, int v_w, int v_h, int color_depth);
static void gfx_directx_win_exit(struct BITMAP *bmp);
static BITMAP *gfx_directx_acknowledge_resize(void);

static BITMAP *_create_directx_forefront_bitmap(int w, int h, int color_depth);
static void _destroy_directx_forefront_bitmap_extras(void);


GFX_DRIVER gfx_directx_win =
{
   GFX_DIRECTX_WIN,
   empty_string,
   empty_string,
   "DirectDraw window",
   gfx_directx_win_init,
   gfx_directx_win_exit,
   NULL,                        // AL_METHOD(int, scroll, (int x, int y));
   gfx_directx_sync,
   gfx_directx_set_palette,
   NULL,                        // AL_METHOD(int, request_scroll, (int x, int y));
   NULL,                        // AL_METHOD(int, poll_scroll, (void));
   NULL,                        // AL_METHOD(void, enable_triple_buffer, (void));
   gfx_directx_create_video_bitmap_win,
   gfx_directx_destroy_video_bitmap_win,
   gfx_directx_show_video_bitmap_win,
   NULL,
   gfx_directx_create_system_bitmap,
   gfx_directx_destroy_system_bitmap,
   gfx_directx_set_mouse_sprite,
   gfx_directx_show_mouse,
   gfx_directx_hide_mouse,
   gfx_directx_move_mouse,
   NULL,                        // AL_METHOD(void, drawing_mode, (void));
   NULL,                        // AL_METHOD(void, save_video_state, (void*));
   NULL,                        // AL_METHOD(void, restore_video_state, (void*));
   NULL,                        // AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a));
   NULL,                        // AL_METHOD(int, fetch_mode_list, (void));
   gfx_directx_acknowledge_resize,
   0, 0,                        // int w, h;
   TRUE,                        // int linear;
   0,                           // long bank_size;
   0,                           // long bank_gran;
   0,                           // long vid_mem;
   0,                           // long vid_phys_base;
   TRUE                         // int windowed;
};


static void switch_in_win(void);
static void handle_window_enter_sysmode_win(void);
static void handle_window_exit_sysmode_win(void);
static void handle_window_move_win(int, int, int, int);
static void paint_win(RECT *);


static WIN_GFX_DRIVER win_gfx_driver_windowed =
{
   TRUE,
   switch_in_win,
   NULL,                        // AL_METHOD(void, switch_out, (void));
   handle_window_enter_sysmode_win,
   handle_window_exit_sysmode_win,
   handle_window_move_win,
   NULL,                        // AL_METHOD(void, iconify, (void));
   paint_win
};


static char gfx_driver_desc[256] = EMPTY_STRING;
static DDRAW_SURFACE *offscreen_surface = NULL;
static RECT working_area;
static COLORCONV_BLITTER_FUNC *colorconv_blit = NULL;
static int direct_updating_mode_enabled; /* configuration flag */
static int direct_updating_mode_on; /* live flag */
static GFX_VTABLE _special_vtable; /* special vtable for offscreen bitmap */
static int reused_offscreen_surface = FALSE;



/* get_working_area:
 *  Retrieves the rectangle of the working area.
 */
static void get_working_area(RECT *working_area)
{
   SystemParametersInfo(SPI_GETWORKAREA, 0, working_area, 0);
   working_area->left   += 3;  /* for the taskbar */
   working_area->top    += 3;
   working_area->right  -= 3;
   working_area->bottom -= 3;
}



/* switch_in_win:
 *  Handles window switched in.
 */
static void switch_in_win(void)
{
   gfx_directx_compare_color_depth(desktop_color_depth());

   restore_all_ddraw_surfaces();
   get_working_area(&working_area);
}



/* handle_window_enter_sysmode_win:
 *  Causes the driver to switch to indirect updating mode.
 */
static void handle_window_enter_sysmode_win(void)
{
   if (direct_updating_mode_enabled && colorconv_blit) {
      if (direct_updating_mode_on) {
         direct_updating_mode_on = FALSE;
         _TRACE(PREFIX_I "direct updating mode off\n");
      }
   }
}



/* handle_window_exit_sysmode_win:
 *  Causes the driver to switch back to direct updating mode.
 */
static void handle_window_exit_sysmode_win(void)
{
   if (direct_updating_mode_enabled && colorconv_blit) {
      if (!direct_updating_mode_on) {
         direct_updating_mode_on = TRUE;
         _TRACE(PREFIX_I "direct updating mode on\n");
      }
   }
}



/* handle_window_move_win:
 *  Makes sure alignment is kept after the window has been moved.
 */
static void handle_window_move_win(int x, int y, int w, int h)
{
   int xmod;
   RECT window_rect;
   HWND allegro_wnd = win_get_window();

   if (colorconv_blit) {
      if (((BYTES_PER_PIXEL(_win_desktop_depth) == 1) && (xmod=ABS(x)%4)) ||
          ((BYTES_PER_PIXEL(_win_desktop_depth) == 2) && (xmod=ABS(x)%2)) ||
          ((BYTES_PER_PIXEL(_win_desktop_depth) == 3) && (xmod=ABS(x)%4))) {
         GetWindowRect(allegro_wnd, &window_rect);
         SetWindowPos(allegro_wnd, 0, window_rect.left + (x > 0 ? -xmod : xmod),
                      window_rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
         _TRACE(PREFIX_I "window shifted by %d pixe%s to the %s to enforce alignment\n",
                xmod, xmod > 1 ? "ls" : "l", x > 0 ? "left" : "right");
      }
   }
   else {
      /* JG: avoid artifacts when moving fast the window on WinXP. */
      InvalidateRect(allegro_wnd, NULL, FALSE);
   }
}



/* paint_win:
 *  Handles window paint events.
 */
static void paint_win(RECT *rect)
{
   /* we may have lost the DirectDraw surfaces
    * (e.g after the monitor has gone to low power)
    */
   if (gfx_directx_primary_surface->id == 0 ||
       IDirectDrawSurface2_IsLost(gfx_directx_primary_surface->id) == DDERR_SURFACELOST) {
      switch_in_win();
   }

   /* clip the rectangle */
   rect->right = MIN(rect->right, gfx_directx_win.w);
   rect->bottom = MIN(rect->bottom, gfx_directx_win.h);

   _al_wd_update_window(rect);
}



/* update_matching_window:
 *  Updates a portion of the window when the color depths match.
 */
static void update_matching_window(RECT *rect)
{
   HWND allegro_wnd = win_get_window();
   union {
     POINT p;
     RECT r;
   } dest_rect;

   _enter_gfx_critical();

   if (!gfx_directx_forefront_bitmap) {
      _exit_gfx_critical();
      return;
   }

   if (rect)
      dest_rect.r = *rect;
   else {
      dest_rect.r.left   = 0;
      dest_rect.r.right  = gfx_directx_win.w;
      dest_rect.r.top    = 0;
      dest_rect.r.bottom = gfx_directx_win.h;
   }

   ClientToScreen(allegro_wnd, &dest_rect.p);
   ClientToScreen(allegro_wnd, &dest_rect.p + 1);

   /* Blit offscreen backbuffer to the window.
      The ID can be 0 when the primary surface is lost and we weren't
      able to restore & recreate it in gfx_directx_restore_surface().
    */
   if (gfx_directx_primary_surface->id != 0) {
      IDirectDrawSurface2_Blt(gfx_directx_primary_surface->id, &dest_rect.r,
                              offscreen_surface->id, rect, 0, NULL);
   }

   _exit_gfx_critical();
}



/* is_contained:
 *  Helper to find the relative position of two rectangles.
 */
static INLINE int is_contained(RECT *rect1, RECT *rect2)
{
   if ((rect1->left >= rect2->left) &&
       (rect1->top >= rect2->top) &&
       (rect1->right <= rect2->right) &&
       (rect1->bottom <= rect2->bottom))
      return TRUE;
   else
      return FALSE;
}



/* ddsurf_blit_ex:
 *  Extended blit function performing color conversion.
 */
static int ddsurf_blit_ex(LPDIRECTDRAWSURFACE2 dest_surf, RECT *dest_rect,
                          LPDIRECTDRAWSURFACE2 src_surf, RECT *src_rect)
{
   DDSURFACEDESC src_desc, dest_desc;
   GRAPHICS_RECT src_gfx_rect, dest_gfx_rect;
   HRESULT hr;

   src_desc.dwSize = sizeof(src_desc);
   src_desc.dwFlags = 0;
   dest_desc.dwSize = sizeof(dest_desc);
   dest_desc.dwFlags = 0;

   hr = IDirectDrawSurface2_Lock(dest_surf, dest_rect, &dest_desc,
                                 DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
   if (FAILED(hr))
      return -1;

   hr = IDirectDrawSurface2_Lock(src_surf, src_rect, &src_desc,
                                 DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL);
   if (FAILED(hr)) {
      IDirectDrawSurface2_Unlock(dest_surf, NULL);
      return -1;
   }

   /* fill in source graphics rectangle description */
   src_gfx_rect.width  = src_rect->right  - src_rect->left;
   src_gfx_rect.height = src_rect->bottom - src_rect->top;
   src_gfx_rect.pitch  = src_desc.lPitch;
   src_gfx_rect.data   = src_desc.lpSurface;

   /* fill in destination graphics rectangle description */
   dest_gfx_rect.pitch = dest_desc.lPitch;
   dest_gfx_rect.data  = dest_desc.lpSurface;

   /* function doing the hard work */
   colorconv_blit(&src_gfx_rect, &dest_gfx_rect);

   IDirectDrawSurface2_Unlock(src_surf, NULL);
   IDirectDrawSurface2_Unlock(dest_surf, NULL);

   return 0;
}



/* update_colorconv_window:
 *  Updates a portion of the window when the color depths don't match.
 */
static void update_colorconv_window(RECT *rect)
{
   RECT src_rect;
   union {
     POINT p;
     RECT r;
   } dest_rect;
   HDC src_dc, dest_dc;
   HRESULT hr;
   int direct;
   HWND allegro_wnd = win_get_window();

   _enter_gfx_critical();

   if (!gfx_directx_forefront_bitmap) {
      _exit_gfx_critical();
      return;
   }

   if (rect) {
#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      src_rect.left   = rect->left & 0xfffffffc;
      src_rect.right  = (rect->right+3) & 0xfffffffc;
#else
      src_rect.left   = rect->left;
      src_rect.right  = rect->right;
#endif
      src_rect.top    = rect->top;
      src_rect.bottom = rect->bottom;
   }
   else {
      src_rect.left   = 0;
      src_rect.right  = gfx_directx_win.w;
      src_rect.top    = 0;
      src_rect.bottom = gfx_directx_win.h;
   }

   dest_rect.r = src_rect;
   ClientToScreen(allegro_wnd, &dest_rect.p);
   ClientToScreen(allegro_wnd, &dest_rect.p + 1);

   direct = (direct_updating_mode_on &&
             is_contained(&dest_rect.r, &working_area) &&
             GetForegroundWindow() == allegro_wnd);

   if (direct) {
      /* blit directly to the primary surface without clipping */
      if (gfx_directx_primary_surface->id != 0) {
         ddsurf_blit_ex(gfx_directx_primary_surface->id, &dest_rect.r,
            offscreen_surface->id, &src_rect);
      }
   }
   else {
      /* blit to the window using GDI */
      hr = IDirectDrawSurface2_GetDC(offscreen_surface->id, &src_dc);
      if (FAILED(hr))
         goto End;

      dest_dc = GetDC(allegro_wnd);
      if (!dest_dc) {
         IDirectDrawSurface2_ReleaseDC(offscreen_surface->id, src_dc);
         goto End;
      }

      BitBlt(dest_dc,
             src_rect.left,
             src_rect.top,
             src_rect.right - src_rect.left,
             src_rect.bottom - src_rect.top,
             src_dc,
             src_rect.left,
             src_rect.top,
             SRCCOPY);

      ReleaseDC(allegro_wnd, dest_dc);
      IDirectDrawSurface2_ReleaseDC(offscreen_surface->id, src_dc);
   }

 End:
   _exit_gfx_critical();
}



/* gfx_directx_create_video_bitmap_win:
 */
static BITMAP *gfx_directx_create_video_bitmap_win(int width, int height)
{
   DDRAW_SURFACE *surf;
   BITMAP *bmp;

   /* try to detect page flipping and triple buffering patterns */
   if ((width == gfx_directx_forefront_bitmap->w) && (height == gfx_directx_forefront_bitmap->h) && (!reused_offscreen_surface)) {
      /* recycle the offscreen surface as a video bitmap */
      bmp = gfx_directx_make_bitmap_from_surface(offscreen_surface, width, height, BMP_ID_VIDEO);
      if (bmp) {
         bmp->vtable = &_special_vtable;
         bmp->write_bank = gfx_directx_write_bank_win;
         offscreen_surface->parent_bmp = bmp;
         reused_offscreen_surface = TRUE;
      }
      return bmp;
   }

   /* create the DirectDraw surface */
   if (colorconv_blit) {
      surf = gfx_directx_create_surface(width, height, ddpixel_format, DDRAW_SURFACE_SYSTEM);
   }
   else {
      surf = gfx_directx_create_surface(width, height, NULL, DDRAW_SURFACE_VIDEO);
      if (!surf)
         surf = gfx_directx_create_surface(width, height, NULL, DDRAW_SURFACE_SYSTEM);
   }

   if (!surf)
      return NULL;

   /* create the bitmap that wraps up the surface */
   bmp = gfx_directx_make_bitmap_from_surface(surf, width, height, BMP_ID_VIDEO);
   if (!bmp) {
      gfx_directx_destroy_surface(surf);
      return NULL;
   }

   surf->parent_bmp = bmp;

   return bmp;
}



/* gfx_directx_destroy_video_bitmap_win:
 */
static void gfx_directx_destroy_video_bitmap_win(BITMAP *bmp)
{
   DDRAW_SURFACE *surf;

   surf = DDRAW_SURFACE_OF(bmp);

   if (surf == offscreen_surface) {
      reused_offscreen_surface = FALSE;
   }
   else {
      /* destroy the surface */
      gfx_directx_destroy_surface(surf);
   }

   _AL_FREE(bmp);
}



/* gfx_directx_show_video_bitmap_win:
 *  Makes the specified video bitmap visible.
 */
static int gfx_directx_show_video_bitmap_win(BITMAP *bmp)
{
   DDRAW_SURFACE *surf;
   BITMAP *former_visible_bmp;

   /* guard against show_video_bitmap(screen); */
   surf = DDRAW_SURFACE_OF(bmp);
   if (surf == offscreen_surface)
      return 0;

   former_visible_bmp = offscreen_surface->parent_bmp;

   /* manually flip the offscreen surface */
   gfx_directx_forefront_bitmap->extra = surf;
   offscreen_surface = surf;

   /* restore regular methods for video bitmaps */
   former_visible_bmp->vtable->release = gfx_directx_unlock;
   former_visible_bmp->vtable->unwrite_bank = gfx_directx_unwrite_bank;
   former_visible_bmp->write_bank = gfx_directx_write_bank;

   /* set special methods for the forefront bitmap */
   bmp->vtable->release = gfx_directx_unlock_win;
   bmp->vtable->unwrite_bank = gfx_directx_unwrite_bank_win;
   bmp->write_bank = gfx_directx_write_bank_win;

   /* display the new contents */
   if (_wait_for_vsync)
      gfx_directx_sync();
   _al_wd_update_window(NULL);

   return 0;
}



/* gfx_directx_set_palette_win_8:
 *  Updates the palette for color conversion from 8-bit to 8-bit.
 */
static void gfx_directx_set_palette_win_8(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   unsigned char *cmap;
   int n;

   cmap = _get_colorconv_map();

   for (n = from; n <= to; n++)
      cmap[n] = _win_desktop_rgb_map.data[p[n].r>>1][p[n].g>>1][p[n].b>>1];

   _al_wd_update_window(NULL);
}



/* gfx_directx_set_palette_win:
 *  Updates the palette for color conversion from 8-bit to hi/true color.
 */
static void gfx_directx_set_palette_win(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   /* for the normal updating mode */
   gfx_directx_set_palette(p, from, to, FALSE);

   /* for the direct updating mode */
   _set_colorconv_palette(p, from, to);

   _al_wd_update_window(NULL);
}



/* wnd_set_windowed_coop:
 *  Sets the DirectDraw cooperative level.
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



/* verify_color_depth:
 *  Compares the requested color depth with the desktop color depth.
 */
static int verify_color_depth (int color_depth)
{
   AL_CONST char *ddu;
   char tmp1[64], tmp2[128];
   int i;

   if ((gfx_directx_compare_color_depth(color_depth) == 0) && (color_depth != 8)) {
      /* the color depths match */
      _al_wd_update_window = update_matching_window;
   }
   else {
      /* disallow cross-conversion between 15-bit and 16-bit colors */
      if ((BYTES_PER_PIXEL(color_depth) == 2) && (BYTES_PER_PIXEL(_win_desktop_depth) == 2))
         return -1;

      /* the color depths don't match, need color conversion */
      colorconv_blit = _get_colorconv_blitter(color_depth, _win_desktop_depth);

      if (!colorconv_blit)
         return -1;

      _al_wd_update_window = update_colorconv_window;

      /* read direct updating configuration variable */
      ddu = get_config_string(uconvert_ascii("graphics", tmp1),
                              uconvert_ascii("disable_direct_updating", tmp2),
                              NULL);

      if ((ddu) && ((i = ugetc(ddu)) != 0) && ((i == 'y') || (i == 'Y') || (i == '1')))
         direct_updating_mode_enabled = FALSE;
      else
         direct_updating_mode_enabled = TRUE;

      direct_updating_mode_on = direct_updating_mode_enabled;
   }

   return 0;
}



/* create_offscreen:
 *  Creates the offscreen surface.
 */
static int create_offscreen(int w, int h, int color_depth)
{
   if (colorconv_blit) {
      /* create offscreen surface in system memory */
      offscreen_surface = gfx_directx_create_surface(w, h, ddpixel_format, DDRAW_SURFACE_SYSTEM);
   }
   else {
      /* create offscreen surface in video memory */
      offscreen_surface = gfx_directx_create_surface(w, h, NULL, DDRAW_SURFACE_VIDEO);

      if (!offscreen_surface) {
         _TRACE(PREFIX_E "Can't create offscreen surface in video memory.\n");

         /* create offscreen surface in system memory */
         offscreen_surface = gfx_directx_create_surface(w, h, NULL, DDRAW_SURFACE_SYSTEM);
      }
   }

   if (!offscreen_surface) {
      _TRACE(PREFIX_E "Can't create offscreen surface.\n");
      return -1;
   }

   return 0;
}


/* gfx_directx_setup_driver_desc:
 *  Sets the driver description string.
 */
static void gfx_directx_setup_driver_desc(void)
{
   char tmp1[128], tmp2[128];

   uszprintf(gfx_driver_desc, sizeof(gfx_driver_desc),
             uconvert_ascii("DirectDraw, in %s, %d bpp window", tmp1),
             uconvert_ascii((colorconv_blit ? "color conversion" : "matching"), tmp2),
             _win_desktop_depth);

   gfx_directx_win.desc = gfx_driver_desc;
}



/* gfx_directx_win_init:
 *  Initializes the driver.
 */
static struct BITMAP *gfx_directx_win_init(int w, int h, int v_w, int v_h, int color_depth)
{
   HRESULT hr;
   HWND allegro_wnd = win_get_window();

   /* flipping is impossible in windowed mode */
   if ((v_w != w && v_w != 0) || (v_h != h && v_h != 0)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported virtual resolution"));
      return NULL;
   }

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   if (w%4)
      return NULL;
#endif

   _enter_critical();

   /* init DirectX */
   if (init_directx() != 0)
      goto Error;
   if (verify_color_depth(color_depth)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      goto Error;
   }
   if (wnd_call_proc(wnd_set_windowed_coop) != 0)
      goto Error;
   if (finalize_directx_init() != 0)
      goto Error;

   if (adjust_window(w, h) != 0) {
      _TRACE(PREFIX_E "window size not supported.\n");
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      goto Error;
   }

   /* get the working area */
   get_working_area(&working_area);

   /* create primary surface */
   if (gfx_directx_create_primary() != 0)
      goto Error;

   /* create clipper */
   if (gfx_directx_create_clipper(allegro_wnd) != 0)
      goto Error;

   hr = IDirectDrawSurface_SetClipper(gfx_directx_primary_surface->id, ddclipper);
   if (FAILED(hr))
      goto Error;

   /* create forefront bitmap */
   if (!_create_directx_forefront_bitmap(w, h, color_depth))
      goto Error;

   /* connect to the system driver */
   win_gfx_driver = &win_gfx_driver_windowed;

   /* set default switching policy */
   set_display_switch_mode(SWITCH_PAUSE);

   _exit_critical();

   return gfx_directx_forefront_bitmap;

 Error:
   _exit_critical();

   /* release the DirectDraw object */
   gfx_directx_win_exit(NULL);

   return NULL;
}



/* gfx_directx_win_exit:
 *  Shuts down the driver.
 */
static void gfx_directx_win_exit(struct BITMAP *bmp)
{
   _enter_gfx_critical();

   if (bmp) {
      save_window_pos();
      clear_bitmap(bmp);
   }

   /* disconnect from the system driver */
   win_gfx_driver = NULL;

   _destroy_directx_forefront_bitmap_extras();

   /* release the color conversion blitter */
   if (colorconv_blit) {
      _release_colorconv_blitter(colorconv_blit);
      colorconv_blit = NULL;
   }

   gfx_directx_exit(NULL);

   _exit_gfx_critical();
}



static BITMAP *gfx_directx_acknowledge_resize(void)
{
   HWND allegro_wnd = win_get_window();
   int color_depth = (screen ? bitmap_color_depth(screen):
                               desktop_color_depth());
   int w, h;
   RECT rc;
   BITMAP *new_screen;
   BITMAP* tmp = NULL;

   GetClientRect(allegro_wnd, &rc);
   w = rc.right;
   h = rc.bottom;
   if (w % 4)
      w -= (w % 4);

   /* Copy current content in screen */
   if (screen)
      tmp = create_bitmap_ex(color_depth, w, h);

   _enter_gfx_critical();

   if (tmp) {
      clear_bitmap(tmp);
      blit(gfx_directx_forefront_bitmap, tmp, 0, 0, 0, 0, w, h);
   }

   /* Destroy old screen */
   destroy_bitmap(gfx_directx_forefront_bitmap);
   _destroy_directx_forefront_bitmap_extras();

   /* Re-create the screen */
   new_screen = _create_directx_forefront_bitmap(w, h, color_depth);
   if (!new_screen) {
      if (tmp)
         destroy_bitmap(tmp);

      _exit_gfx_critical();
      return NULL;
   }

   /* Restore content in the new screen */
   clear_bitmap(new_screen);
   if (tmp) {
     blit(tmp, new_screen, 0, 0, 0, 0, w, h);
     destroy_bitmap(tmp);
   }

   _exit_gfx_critical();

   return new_screen;
}



static BITMAP *_create_directx_forefront_bitmap(int w, int h, int color_depth)
{
   unsigned char *cmap;
   int i;

   /* create offscreen backbuffer */
   if (create_offscreen(w, h, color_depth) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Windowed mode not supported"));
      goto Error;
   }

   /* setup color management */
   if (_win_desktop_depth == 8) {
      build_desktop_rgb_map();

      if (color_depth == 8) {
         gfx_directx_win.set_palette = gfx_directx_set_palette_win_8;
      }
      else {
         cmap = _get_colorconv_map();

         for (i=0; i<4096; i++)
            cmap[i] = _win_desktop_rgb_map.data[((i&0xF00)>>7) | ((i&0x800)>>11)]
                                          [((i&0x0F0)>>3) | ((i&0x080)>>7)]
                                          [((i&0x00F)<<1) | ((i&0x008)>>3)];

         if (gfx_directx_update_color_format(offscreen_surface, color_depth) != 0)
            goto Error;
      }
   }
   else {
      if (color_depth == 8) {
         gfx_directx_win.set_palette = gfx_directx_set_palette_win;

         if (gfx_directx_create_palette(offscreen_surface) != 0)
            goto Error;

         /* init the core library color conversion functions */
         if (gfx_directx_update_color_format(gfx_directx_primary_surface, _win_desktop_depth) != 0)
            goto Error;
      }
      else {
         if (gfx_directx_update_color_format(offscreen_surface, color_depth) != 0)
            goto Error;
      }
   }

   /* setup Allegro gfx driver */
   gfx_directx_setup_driver_desc();
   if (gfx_directx_setup_driver(&gfx_directx_win, w, h, color_depth) != 0)
      goto Error;

   gfx_directx_forefront_bitmap = gfx_directx_make_bitmap_from_surface(offscreen_surface, w, h, BMP_ID_VIDEO);

   gfx_directx_enable_acceleration(&gfx_directx_win);
   memcpy(&_special_vtable, &_screen_vtable, sizeof (GFX_VTABLE));
   _special_vtable.release = gfx_directx_unlock_win;
   _special_vtable.unwrite_bank = gfx_directx_unwrite_bank_win;
   gfx_directx_forefront_bitmap->vtable = &_special_vtable;
   gfx_directx_forefront_bitmap->write_bank = gfx_directx_write_bank_win;

   /* the last flag serves as end of loop delimiter */
   _al_wd_dirty_lines = _AL_MALLOC_ATOMIC((h+1) * sizeof(char));
   ASSERT(_al_wd_dirty_lines);
   memset(_al_wd_dirty_lines, 0, (h+1) * sizeof(char));

   return gfx_directx_forefront_bitmap;

Error:
   return NULL;
}



static void _destroy_directx_forefront_bitmap_extras(void)
{
   /* destroy dirty lines array */
   if (_al_wd_dirty_lines) {
      _AL_FREE(_al_wd_dirty_lines);
      _al_wd_dirty_lines = NULL;
   }

   /* destroy the offscreen backbuffer */
   if (offscreen_surface) {
      gfx_directx_destroy_surface(offscreen_surface);
      offscreen_surface = NULL;
      reused_offscreen_surface = FALSE;
      gfx_directx_forefront_bitmap = NULL;
   }
}
