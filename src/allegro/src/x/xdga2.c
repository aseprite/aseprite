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
 *      DGA 2.0 graphics driver.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "xwin.h"

#if (defined ALLEGRO_XWINDOWS_WITH_XF86DGA2) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>


#define RESYNC()     XDGASync(_xwin.display, _xwin.screen);


static XDGADevice *dga_device = NULL;
static char _xdga2_driver_desc[256] = EMPTY_STRING;
static Colormap _dga_cmap = 0;
static int dga_event_base;
static int keyboard_got_focus = FALSE;


static int _xdga2_find_mode(int w, int h, int vw, int vh, int depth);
static void _xdga2_handle_input(void);
static BITMAP *_xdga2_gfxdrv_init(int w, int h, int vw, int vh, int color_depth);
static BITMAP *_xdga2_soft_gfxdrv_init(int w, int h, int vw, int vh, int color_depth);
static void _xdga2_gfxdrv_exit(BITMAP *bmp);
static int _xdga2_poll_scroll(void);
static int _xdga2_request_scroll(int x, int y);
static int _xdga2_request_video_bitmap(BITMAP *bmp);
static int _xdga2_scroll_screen(int x, int y);
static void _xdga2_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync);
static void _xdga2_acquire(BITMAP *bmp);
static GFX_MODE_LIST *_xdga2_fetch_mode_list(void);

#ifdef ALLEGRO_NO_ASM
uintptr_t _xdga2_write_line(BITMAP *bmp, int line);
#else
uintptr_t _xdga2_write_line_asm(BITMAP *bmp, int line);
#endif


static void (*_orig_hline) (BITMAP *bmp, int x1, int y, int x2, int color);
static void (*_orig_vline) (BITMAP *bmp, int x, int y1, int y2, int color);
static void (*_orig_rectfill) (BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
static void (*_orig_draw_sprite) (BITMAP *bmp, BITMAP *sprite, int x, int y);
static void (*_orig_masked_blit) (BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);

static void _xaccel_hline(BITMAP *bmp, int x1, int y, int x2, int color);
static void _xaccel_vline(BITMAP *bmp, int x, int y1, int y2, int color);
static void _xaccel_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color);
static void _xaccel_clear_to_color(BITMAP *bmp, int color);
static void _xaccel_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
static void _xaccel_draw_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y);
static void _xaccel_masked_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);

#define DGA_MAX_EVENTS 5


GFX_DRIVER gfx_xdga2 =
{
   GFX_XDGA2,
   empty_string,
   empty_string,
   "DGA 2.0",
   _xdga2_gfxdrv_init,
   _xdga2_gfxdrv_exit,
   _xdga2_scroll_screen,
   _xwin_vsync,
   _xdga2_set_palette_range,
   _xdga2_request_scroll,
   _xdga2_poll_scroll,
   NULL,
   NULL, NULL, NULL,
   _xdga2_request_video_bitmap,
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   NULL, NULL,
   NULL,
   _xdga2_fetch_mode_list,
   640, 480,
   TRUE,
   0, 0,
   0,
   0,
   FALSE
};



GFX_DRIVER gfx_xdga2_soft =
{
   GFX_XDGA2_SOFT,
   empty_string,
   empty_string,
   "DGA 2.0 soft",
   _xdga2_soft_gfxdrv_init,
   _xdga2_gfxdrv_exit,
   _xdga2_scroll_screen,
   _xwin_vsync,
   _xdga2_set_palette_range,
   _xdga2_request_scroll,
   _xdga2_poll_scroll,
   NULL,
   NULL, NULL, NULL,
   _xdga2_request_video_bitmap,
   NULL, NULL,
   NULL, NULL, NULL, NULL,
   NULL,
   NULL, NULL,
   NULL,
   _xdga2_fetch_mode_list,
   640, 480,
   TRUE,
   0, 0,
   0,
   0,
   FALSE
};



/* _xdga2_fetch_mode_list:
 *  Creates list of available DGA2 video modes.
 */
static GFX_MODE_LIST *_xdga2_private_fetch_mode_list(void)
{
   XDGAMode *mode;
   int bpp, num_modes, stored_modes, i, j, already_there;
   GFX_MODE_LIST *mode_list;
   GFX_MODE *tmp;

   mode = XDGAQueryModes(_xwin.display, _xwin.screen, &num_modes);
   if (!mode)
      return NULL;

   mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!mode_list)
      goto error;
   mode_list->mode = NULL;

   stored_modes = 0;
   for (i=0; i<num_modes; i++) {
      bpp = (mode[i].depth == 24) ? mode[i].bitsPerPixel : mode[i].depth;
      already_there = FALSE;
      for (j=0; j<stored_modes; j++) {
         if ((mode_list->mode[j].width == mode[i].viewportWidth) &&
             (mode_list->mode[j].height == mode[i].viewportHeight) &&
             (mode_list->mode[j].bpp == bpp)) {
            already_there = TRUE;
            break;
         }
      }
      if (!already_there) {
	 tmp = _AL_REALLOC(mode_list->mode, sizeof(GFX_MODE) * (stored_modes + 1));
	 if (!tmp)
            goto error;
         mode_list->mode = tmp;
         mode_list->mode[stored_modes].width = mode[i].viewportWidth;
         mode_list->mode[stored_modes].height = mode[i].viewportHeight;
         mode_list->mode[stored_modes].bpp = bpp;
	 stored_modes++;
      }
   }

   tmp = _AL_REALLOC(mode_list->mode, sizeof(GFX_MODE) * (stored_modes + 1));
   if (!tmp)
      goto error;
   mode_list->mode = tmp;
   mode_list->mode[stored_modes].width = 0;
   mode_list->mode[stored_modes].height = 0;
   mode_list->mode[stored_modes].bpp = 0;
   mode_list->num_modes = stored_modes;

   XFree(mode);
   return mode_list;

   error:
   if (mode_list) {
      _AL_FREE(mode_list->mode);
      _AL_FREE(mode_list);
   }
   XFree(mode);
   return NULL;
}



static GFX_MODE_LIST *_xdga2_fetch_mode_list(void)
{
   GFX_MODE_LIST *list;
   XLOCK ();
   list = _xdga2_private_fetch_mode_list();
   XUNLOCK ();
   return list;
}



/* _xdga2_find_mode:
 *  Returns id number of specified video mode if available, 0 otherwise.
 */
static int _xdga2_find_mode(int w, int h, int vw, int vh, int depth)
{
   XDGAMode *mode;
   int num_modes;
   int bpp, i, found;

   mode = XDGAQueryModes(_xwin.display, _xwin.screen, &num_modes);
   if (!mode)
      return 0;

   /* Let's first try setting also requested refresh rate */
   for (i=0; i<num_modes; i++) {
      bpp = mode[i].depth;
      if (bpp == 24) bpp = mode[i].bitsPerPixel;
      
      if ((mode[i].viewportWidth == w) &&
          (mode[i].viewportHeight == h) &&
          (mode[i].imageWidth >= vw) &&
          (mode[i].imageHeight >= vh) &&
          (mode[i].verticalRefresh >= _refresh_rate_request) &&
          (bpp == depth)) break;
   }

   if (i == num_modes) {
      /* No modes were found, so now we don't care about refresh rate */
      for (i=0; i<num_modes; i++) {
         bpp = mode[i].depth;
         if (bpp == 24) bpp = mode[i].bitsPerPixel;
      
         if ((mode[i].viewportWidth == w) &&
            (mode[i].viewportHeight == h) &&
            (mode[i].imageWidth >= vw) &&
            (mode[i].imageHeight >= vh) &&
            (bpp == depth)) break;
      }
      if (i == num_modes) {
         /* No way out: mode not found */
         XFree(mode);
         return 0;
      }
   }
   
   found = mode[i].num;
   XFree(mode);
   return found;
}



/* _xdga2_handle_input:
 *  Handles DGA events pending from queue.
 */
static void _xdga2_handle_input(void)
{
   int i, events, events_queued;
   static XDGAEvent event[DGA_MAX_EVENTS + 1];
   XDGAEvent *cur_event;
   XKeyEvent key;
   int dx, dy, dz = 0;
   static int mouse_buttons = 0;

   if (_xwin.display == 0)
      return;

   XSync(_xwin.display, False);

   /* How much events are available in the queue.  */
   events = events_queued = XEventsQueued(_xwin.display, QueuedAlready);
   if (events <= 0)
      return;

   /* Limit amount of events we read at once.  */
   if (events > DGA_MAX_EVENTS)
      events = DGA_MAX_EVENTS;

   /* Read pending events.  */
   for (i = 0; i < events; i++)
      XNextEvent(_xwin.display, (XEvent *)&event[i]);

   /* see xwin.c */
   if (events_queued > events && event[i - 1].type == dga_event_base+KeyRelease) {
      XNextEvent(_xwin.display, (XEvent *)&event[i]);
      events++;
   }

   /* Process all events.  */
   for (i = 0; i < events; i++) {
      /* see xwin.c */
      if (event[i].type == dga_event_base+KeyRelease && (i + 1) < events) {
         if (event[i + 1].type == dga_event_base+KeyPress) {
            if (event[i].xkey.keycode == event[i + 1].xkey.keycode &&
               event[i].xkey.time == event[i + 1].xkey.time)
               continue;
         }
      }

      cur_event = &event[i];
      switch (cur_event->type - dga_event_base) {

         case KeyPress:
            XDGAKeyEventToXKeyEvent(&cur_event->xkey, &key);
	    key.type -= dga_event_base;
	    _xwin_keyboard_handler(&key, TRUE);
            break;

         case KeyRelease:
	    XDGAKeyEventToXKeyEvent(&cur_event->xkey, &key);
	    key.type -= dga_event_base;
	    _xwin_keyboard_handler(&key, TRUE);
            break;

         case ButtonPress:
            if (cur_event->xbutton.button == Button1)
               mouse_buttons |= 1;
            else if (cur_event->xbutton.button == Button3)
               mouse_buttons |= 2;
            else if (cur_event->xbutton.button == Button2)
               mouse_buttons |= 4;
            else if (cur_event->xbutton.button == Button4)
               dz = 1;
            else if (cur_event->xbutton.button == Button5)
               dz = -1;
            if (_xwin_mouse_interrupt)
               (*_xwin_mouse_interrupt)(0, 0, dz, 0, mouse_buttons);
            break;

         case ButtonRelease:
            if (cur_event->xbutton.button == Button1)
               mouse_buttons &= ~1;
            else if (cur_event->xbutton.button == Button3)
               mouse_buttons &= ~2;
            else if (cur_event->xbutton.button == Button2)
               mouse_buttons &= ~4;
            if (_xwin_mouse_interrupt)
               (*_xwin_mouse_interrupt)(0, 0, 0, 0, mouse_buttons);
            break;

         case MotionNotify:
            dx = cur_event->xmotion.dx;
            dy = cur_event->xmotion.dy;
            if (((dx != 0) || (dy != 0)) && _xwin_mouse_interrupt) {
               (*_xwin_mouse_interrupt)(dx, dy, 0, 0, mouse_buttons);
            }
            break;

         default:
            break;
      }
   }
}



/* _xdga2_display_is_local:
 *  Tests that display connection is local.
 *  (Note: this is duplicated in xwin.c).
 */
static int _xdga2_private_display_is_local(void)
{
   char *name;

   if (_xwin.display == 0)
      return 0;

   /* Get display name and test for local display.  */
   name = XDisplayName(0);

   return (((name == 0) || (name[0] == ':') || (strncmp(name, "unix:", 5) == 0)) ? 1 : 0);
}



/* _xdga2_gfxdrv_init_drv:
 *  Initializes driver and creates screen bitmap.
 */
static BITMAP *_xdga2_private_gfxdrv_init_drv(GFX_DRIVER *drv, int w, int h, int vw, int vh, int depth, int accel)
{
   int dga_error_base, dga_major_version, dga_minor_version;
   int mode, mask, red_shift = 0, green_shift = 0, blue_shift = 0;
   long input_mask;
   char tmp1[128], tmp2[128];
   BITMAP *bmp;

   /* This is just to test if the system driver has been installed properly */
   if (_xwin.window == None)
      return NULL;

   /* Test that display is local.  */
   if (!_xdga2_private_display_is_local()) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("This driver needs local display"));
      return NULL;
   }

   /* Choose convenient size.  */
   if ((w == 0) && (h == 0)) {
      w = 640;
      h = 480;
   }

   if ((w < 80) || (h < 80) || (w > 4096) || (h > 4096)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported screen size"));
      return NULL;
   }

   if (vw < w)
      vw = w;
   if (vh < h)
      vh = h;

   if (1
#ifdef ALLEGRO_COLOR8
       && (depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (depth != 15)
       && (depth != 16)
#endif
#ifdef ALLEGRO_COLOR24
       && (depth != 24)
#endif
#ifdef ALLEGRO_COLOR32
       && (depth != 32)
#endif
       ) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return NULL;
   }

   /* Checks presence of DGA extension */
   if (!XDGAQueryExtension(_xwin.display, &dga_event_base, &dga_error_base) ||
       !XDGAQueryVersion(_xwin.display, &dga_major_version, &dga_minor_version)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("DGA extension is not supported"));
      return NULL;
   }

   /* Works only with DGA 2.0 or newer */
   if (dga_major_version < 2) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("DGA 2.0 or newer is required"));
      return NULL;
   }

   /* Attempts to access the framebuffer */
   if (!XDGAOpenFramebuffer(_xwin.display, _xwin.screen)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not open framebuffer"));
      return NULL;
   }

   /* Finds suitable video mode number */
   mode = _xdga2_find_mode(w, h, vw, vh, depth);
   if (!mode) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }

   /* Sets DGA video mode */
   dga_device = XDGASetMode(_xwin.display, _xwin.screen, mode);
   if (dga_device == NULL) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not switch to DGA mode"));
      return NULL;
   }
   _xwin.in_dga_mode = 2;
   _set_current_refresh_rate(dga_device->mode.verticalRefresh);
   set_display_switch_mode(SWITCH_NONE);

   /* Installs DGA color map */
   if (_dga_cmap) {
      XFreeColormap(_xwin.display, _dga_cmap);
      _dga_cmap = 0;
   }
   if ((dga_device->mode.visualClass == PseudoColor)
       || (dga_device->mode.visualClass == GrayScale)
       || (dga_device->mode.visualClass == DirectColor))
      _dga_cmap = XDGACreateColormap(_xwin.display, _xwin.screen, dga_device, AllocAll);
   else
      _dga_cmap = XDGACreateColormap(_xwin.display, _xwin.screen, dga_device, AllocNone);
   XDGAInstallColormap(_xwin.display, _xwin.screen, _dga_cmap);

   /* Sets up direct color shifts */
   if (depth != 8) {
      for (mask = dga_device->mode.redMask, red_shift = 0; (mask & 1) == 0;
         mask >>= 1, red_shift++);
      for (mask = dga_device->mode.greenMask, green_shift = 0; (mask & 1) == 0;
         mask >>= 1, green_shift++);
      for (mask = dga_device->mode.blueMask, blue_shift = 0; (mask & 1) == 0;
         mask >>= 1, blue_shift++);
   }
   switch (depth) {

      case 15:
         _rgb_r_shift_15 = red_shift;
         _rgb_g_shift_15 = green_shift;
         _rgb_b_shift_15 = blue_shift;
         break;

      case 16:
         _rgb_r_shift_16 = red_shift;
         _rgb_g_shift_16 = green_shift;
         _rgb_b_shift_16 = blue_shift;
         break;

      case 24:
         _rgb_r_shift_24 = red_shift;
         _rgb_g_shift_24 = green_shift;
         _rgb_b_shift_24 = blue_shift;
         break;

      case 32:
         _rgb_r_shift_32 = red_shift;
         _rgb_g_shift_32 = green_shift;
         _rgb_b_shift_32 = blue_shift;
         break;
   }

   /* Enables input */
   XSync(_xwin.display, True);
   input_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask
              | ButtonReleaseMask | PointerMotionMask;
   XDGASelectInput(_xwin.display, _xwin.screen, input_mask);
   if (_xwin_keyboard_focused) {
      (*_xwin_keyboard_focused)(FALSE, 0);
      keyboard_got_focus = TRUE;
   }
   _mouse_on = TRUE;

   /* Creates screen bitmap */
   drv->linear = TRUE;
   bmp = _make_bitmap(dga_device->mode.imageWidth, dga_device->mode.imageHeight,
         (uintptr_t)dga_device->data, drv, depth,
         dga_device->mode.bytesPerScanline);
   if (!bmp) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return NULL;
   }
   drv->w = bmp->cr = w;
   drv->h = bmp->cb = h;
   drv->vid_mem = dga_device->mode.imageWidth * dga_device->mode.imageHeight
                * BYTES_PER_PIXEL(depth);

   if (accel) {
      /* Hardware acceleration has been requested */
      
      /* Updates line switcher to accommodate framebuffer synchronization */
#ifdef ALLEGRO_NO_ASM
      bmp->write_bank = _xdga2_write_line;
      bmp->read_bank = _xdga2_write_line;
#else
      bmp->write_bank = _xdga2_write_line_asm;
      bmp->read_bank = _xdga2_write_line_asm;
#endif

      _screen_vtable.acquire = _xdga2_acquire;

      /* Checks for hardware acceleration support */
      if (dga_device->mode.flags & XDGASolidFillRect) {
         /* XDGAFillRectangle is available */
         _orig_hline = _screen_vtable.hline;
         _orig_vline = _screen_vtable.vline;
         _orig_rectfill = _screen_vtable.rectfill;
         _screen_vtable.hline = _xaccel_hline;
         _screen_vtable.vline = _xaccel_vline;
         _screen_vtable.rectfill = _xaccel_rectfill;
         _screen_vtable.clear_to_color = _xaccel_clear_to_color;
         gfx_capabilities |= (GFX_HW_HLINE | GFX_HW_FILL);
      }
      if (dga_device->mode.flags & XDGABlitRect) {
         /* XDGACopyArea is available */
         _screen_vtable.blit_to_self = _xaccel_blit_to_self;
         _screen_vtable.blit_to_self_forward = _xaccel_blit_to_self;
         _screen_vtable.blit_to_self_backward = _xaccel_blit_to_self;
         gfx_capabilities |= GFX_HW_VRAM_BLIT;
      }
      if (dga_device->mode.flags & XDGABlitTransRect) {
         /* XDGACopyTransparentArea is available */
         _orig_draw_sprite = _screen_vtable.draw_sprite;
         _orig_masked_blit = _screen_vtable.masked_blit;
         _screen_vtable.masked_blit = _xaccel_masked_blit;
         _screen_vtable.draw_sprite = _xaccel_draw_sprite;
         if (_screen_vtable.color_depth == 8)
            _screen_vtable.draw_256_sprite = _xaccel_draw_sprite;
         gfx_capabilities |= GFX_HW_VRAM_BLIT_MASKED;
      }

      RESYNC();
   }

   /* Checks for triple buffering */
   if (dga_device->mode.viewportFlags & XDGAFlipRetrace)
      gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;

   /* Sets up driver description */
   uszprintf(_xdga2_driver_desc, sizeof(_xdga2_driver_desc),
             uconvert_ascii("X-Windows DGA 2.0 graphics%s", tmp1),
             uconvert_ascii(accel ? (gfx_capabilities ? " (accelerated)" : "") : " (software only)", tmp2));
   drv->desc = _xdga2_driver_desc;

   return bmp;
}

static BITMAP *_xdga2_gfxdrv_init_drv(GFX_DRIVER *drv, int w, int h, int vw, int vh, int depth, int accel)
{
   BITMAP *bmp;
   XLOCK();
   bmp = _xdga2_private_gfxdrv_init_drv(drv, w, h, vw, vh, depth, accel);
   XUNLOCK();
   if (!bmp)
      _xdga2_gfxdrv_exit(bmp);
   else
      _xwin_input_handler = _xdga2_handle_input;
   return bmp;
}



/* _xdga2_gfxdrv_init:
 *  Creates screen bitmap.
 */
static BITMAP *_xdga2_gfxdrv_init(int w, int h, int vw, int vh, int color_depth)
{
   return _xdga2_gfxdrv_init_drv(&gfx_xdga2, w, h, vw, vh, color_depth, TRUE);
}



/* _xdga2_soft_gfxdrv_init:
 *  Creates screen bitmap (software only mode).
 */
static BITMAP *_xdga2_soft_gfxdrv_init(int w, int h, int vw, int vh, int color_depth)
{
   return _xdga2_gfxdrv_init_drv(&gfx_xdga2_soft, w, h, vw, vh, color_depth, FALSE);
}



/* _xdga2_gfxdrv_exit:
 *  Shuts down gfx driver.
 */
static void _xdga2_gfxdrv_exit(BITMAP *bmp)
{
   XLOCK();
   
   if (_xwin.in_dga_mode) {
      _xwin_input_handler = 0;
       
      XDGACloseFramebuffer(_xwin.display, _xwin.screen);
      XDGASetMode(_xwin.display, _xwin.screen, 0);
      _xwin.in_dga_mode = 0;

      if (_dga_cmap) {
         XFreeColormap(_xwin.display, _dga_cmap);
         _dga_cmap = 0;
      }

      XInstallColormap(_xwin.display, _xwin.colormap);

      set_display_switch_mode(SWITCH_BACKGROUND);
   }
   
   XUNLOCK();
}



/* _xdga2_poll_scroll:
 *  Returns true if there are pending scrolling requests left.
 */
static int _xdga2_poll_scroll(void)
{
   int result;

   XLOCK();
   result = XDGAGetViewportStatus(_xwin.display, _xwin.screen);
   XUNLOCK();
   return result;
}



/* _xdga2_request_scroll:
 *  Starts a screen scroll but doesn't wait for the retrace.
 */
static int _xdga2_request_scroll(int x, int y)
{
   XLOCK();
   
   if (x < 0) x = 0;
   else if (x > dga_device->mode.maxViewportX)
      x = dga_device->mode.maxViewportX;
   if (y < 0) y = 0;
   else if (y > dga_device->mode.maxViewportY)
      y = dga_device->mode.maxViewportY;

   XDGASetViewport(_xwin.display, _xwin.screen, x, y, XDGAFlipRetrace);

   XUNLOCK();
   
   return 0;
}



/* _xdga2_request_video_bitmap:
 *  Page flips to display specified bitmap, but doesn't wait for retrace.
 */
static int _xdga2_request_video_bitmap(BITMAP *bmp)
{
   XLOCK();
   XDGASetViewport(_xwin.display, _xwin.screen, bmp->x_ofs, bmp->y_ofs, XDGAFlipRetrace);
   XUNLOCK();
   return 0;
}



/* _xdga2_scroll_screen:
 *  Scrolls DGA viewport.
 */
static int _xdga2_scroll_screen(int x, int y)
{
   if (x < 0) x = 0;
   else if (x > dga_device->mode.maxViewportX)
      x = dga_device->mode.maxViewportX;
   if (y < 0) y = 0;
   else if (y > dga_device->mode.maxViewportY)
      y = dga_device->mode.maxViewportY;
   if ((_xwin.scroll_x == x) && (_xwin.scroll_y == y))
      return 0;

   XLOCK();

   _xwin.scroll_x = x;
   _xwin.scroll_y = y;

   if (_wait_for_vsync)
      while (XDGAGetViewportStatus(_xwin.display, _xwin.screen))
         ;

   XDGASetViewport(_xwin.display, _xwin.screen, x, y, XDGAFlipRetrace);

   XUNLOCK();
   
   return 0;
}



/* _xdga2_set_palette_range:
 *  Sets palette entries.
 */
static void _xdga2_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   int i;
   static XColor color[256];

   XLOCK();
   
   if (vsync) {
      XSync(_xwin.display, False);
   }

   if (dga_device->mode.depth == 8) {
      for (i = from; i <= to; i++) {
         color[i].flags = DoRed | DoGreen | DoBlue;
         color[i].pixel = i;
         color[i].red = ((p[i].r & 0x3F) * 65535L) / 0x3F;
         color[i].green = ((p[i].g & 0x3F) * 65535L) / 0x3F;
         color[i].blue = ((p[i].b & 0x3F) * 65535L) / 0x3F;
      }
      XStoreColors(_xwin.display, _dga_cmap, color + from, to - from + 1);
      XSync(_xwin.display, False);
   }

   XUNLOCK();
}



/* _xdga2_lock:
 *  Synchronizes with framebuffer.
 */
void _xdga2_lock(BITMAP *bmp)
{
   XLOCK();
   RESYNC();
   XUNLOCK();
   bmp->id |= BMP_ID_LOCKED;
}



/* _xdga2_acquire:
 *  Video bitmap acquire function; synchronizes with framebuffer if needed.
 */
static void _xdga2_acquire(BITMAP *bmp)
{
   if (!(bmp->id & BMP_ID_LOCKED))
      _xdga2_lock(bmp);
}


#ifdef ALLEGRO_NO_ASM

/* _xdga2_write_line:
 *  Returns new line and synchronizes framebuffer if needed.
 */
uintptr_t _xdga2_write_line(BITMAP *bmp, int line)
{
   if (!(bmp->id & BMP_ID_LOCKED))
      _xdga2_lock(bmp);

   return (uintptr_t)(bmp->line[line]);
}

#endif


/* _xaccel_hline:
 *  Accelerated hline.
 */
static void _xaccel_hline(BITMAP *bmp, int x1, int y, int x2, int color)
{
   int tmp;
   
   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_hline(bmp, x1, y, x2, color);
      return;
   }

   if (x1 > x2) {
      tmp = x1;
      x1 = x2;
      x2 = tmp;
   }

   if (bmp->clip) {
      if ((y < bmp->ct) || (y >= bmp->cb))
         return;

      if (x1 < bmp->cl)
         x1 = bmp->cl;

      if (x2 >= bmp->cr)
         x2 = bmp->cr-1;

      if (x2 < x1)
         return;
   }

   x1 += bmp->x_ofs;
   y += bmp->y_ofs;
   x2 += bmp->x_ofs;

   XLOCK();
   XDGAFillRectangle(_xwin.display, _xwin.screen, x1, y, (x2 - x1) + 1, 1, color);
   XUNLOCK();
   bmp->id &= ~BMP_ID_LOCKED;
}



/* _xaccel_vline:
 *  Accelerated vline.
 */
static void _xaccel_vline(BITMAP *bmp, int x, int y1, int y2, int color)
{
   int tmp;
   
   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_vline(bmp, x, y1, y2, color);
      return;
   }

   if (y1 > y2) {
      tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   if (bmp->clip) {
      if ((x < bmp->cl) || (x >= bmp->cr))
         return;

      if (y1 < bmp->ct)
         y1 = bmp->ct;

      if (y2 >= bmp->cb)
         y2 = bmp->cb-1;

      if (y2 < y1)
         return;
   }

   x += bmp->x_ofs;
   y1 += bmp->y_ofs;
   y2 += bmp->y_ofs;

   XLOCK();
   XDGAFillRectangle(_xwin.display, _xwin.screen, x, y1, 1, (y2 - y1) + 1, color);
   XUNLOCK();
   bmp->id &= ~BMP_ID_LOCKED;
}



/* _xaccel_rectfill:
 *  Accelerated rectfill.
 */
static void _xaccel_rectfill(BITMAP *bmp, int x1, int y1, int x2, int y2, int color)
{
   int tmp;

   if (_drawing_mode != DRAW_MODE_SOLID) {
      _orig_rectfill(bmp, x1, y1, x2, y2, color);
      return;
   }

   if (x2 < x1) {
      tmp = x1;
      x1 = x2;
      x2 = tmp;
   }

   if (y2 < y1) {
      tmp = y1;
      y1 = y2;
      y2 = tmp;
   }

   if (bmp->clip) {
      if (x1 < bmp->cl)
         x1 = bmp->cl;

      if (x2 >= bmp->cr)
         x2 = bmp->cr-1;

      if (x2 < x1)
         return;

      if (y1 < bmp->ct)
         y1 = bmp->ct;

      if (y2 >= bmp->cb)
         y2 = bmp->cb-1;

      if (y2 < y1)
         return;
   }

   x1 += bmp->x_ofs;
   y1 += bmp->y_ofs;
   x2 += bmp->x_ofs;
   y2 += bmp->y_ofs;

   XLOCK();
   XDGAFillRectangle(_xwin.display, _xwin.screen, x1, y1, (x2 - x1) + 1, (y2 - y1) + 1, color);
   XUNLOCK();
   bmp->id &= ~BMP_ID_LOCKED;
}



/* _xaccel_clear_to_color:
 *  Accelerated clear_to_color.
 */
static void _xaccel_clear_to_color(BITMAP *bmp, int color)
{
   int x1, y1, x2, y2;

   x1 = bmp->cl + bmp->x_ofs;
   y1 = bmp->ct + bmp->y_ofs;
   x2 = bmp->cr + bmp->x_ofs;
   y2 = bmp->cb + bmp->y_ofs;
   
   XLOCK();
   XDGAFillRectangle(_xwin.display, _xwin.screen, x1, y1, x2 - x1, y2 - y1, color);
   XUNLOCK();
   bmp->id &= ~BMP_ID_LOCKED;
}



/* _xaccel_blit_to_self:
 *  Accelerated vram -> vram blit.
 */
static void _xaccel_blit_to_self(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   source_x += source->x_ofs;
   source_y += source->y_ofs;
   dest_x += dest->x_ofs;
   dest_y += dest->y_ofs;

   XLOCK();
   XDGACopyArea(_xwin.display, _xwin.screen, source_x, source_y, width, height, dest_x, dest_y);
   XUNLOCK();
   dest->id &= ~BMP_ID_LOCKED;
}



/* _xaccel_draw_sprite:
 *  Accelerated draw_sprite.
 */
static void _xaccel_draw_sprite(BITMAP *bmp, BITMAP *sprite, int x, int y)
{
   int sx, sy, w, h;

   if (is_video_bitmap(sprite)) {
      sx = 0;
      sy = 0;
      w = sprite->w;
      h = sprite->h;

      if (bmp->clip) {
         if (x < bmp->cl) {
            sx += bmp->cl - x;
            w -= bmp->cl - x;
            x = bmp->cl;
         }

         if (y < bmp->ct) {
            sy += bmp->ct - y;
            h -= bmp->ct - y;
            y = bmp->ct;
         }

         if (x + w > bmp->cr)
            w = bmp->cr - x;

         if (w <= 0)
            return;

         if (y + h > bmp->cb)
            h = bmp->cb - y;

         if (h <= 0)
            return;
      }

      sx += sprite->x_ofs;
      sy += sprite->y_ofs;
      x += bmp->x_ofs;
      y += bmp->y_ofs;

      XLOCK();
      XDGACopyTransparentArea(_xwin.display, _xwin.screen, sx, sy, w, h, x, y, sprite->vtable->mask_color);
      XUNLOCK();
      bmp->id &= ~BMP_ID_LOCKED;
   }
   else
      _orig_draw_sprite(bmp, sprite, x, y);
}



/* _xaccel_masked_blit:
 *  Accelerated masked_blit.
 */
static void _xaccel_masked_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   if (is_video_bitmap(source)) {
      source_x += source->x_ofs;
      source_y += source->y_ofs;
      dest_x += dest->x_ofs;
      dest_y += dest->y_ofs;

      XLOCK();
      XDGACopyTransparentArea(_xwin.display, _xwin.screen, source_x, source_y, width, height, dest_x, dest_y, source->vtable->mask_color);
      XUNLOCK();
      dest->id &= ~BMP_ID_LOCKED;
   }
   else
      _orig_masked_blit(source, dest, source_x, source_y, dest_x, dest_y, width, height);
}



#ifdef ALLEGRO_MODULE

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   if (system_driver != SYSTEM_XWINDOWS) return;
   _unix_register_gfx_driver(GFX_XDGA2_SOFT, &gfx_xdga2_soft, FALSE, FALSE);
   _unix_register_gfx_driver(GFX_XDGA2,      &gfx_xdga2,      FALSE, FALSE);
}

#endif


#endif
