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
 *      MacOS X quartz fullscreen gfx driver
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
   #error something is wrong with the makefile
#endif


#define FADE_STEPS               16


static BITMAP *osx_qz_full_init(int, int, int, int, int);
static void osx_qz_full_exit(BITMAP *);
static void osx_qz_full_vsync(void);
static void osx_qz_full_set_palette(AL_CONST struct RGB *, int, int, int);
static int osx_qz_show_video_bitmap(BITMAP *);
static GFX_MODE_LIST *osx_qz_fetch_mode_list(void);


CGDirectPaletteRef osx_palette = NULL;
int osx_palette_dirty = FALSE;
int osx_screen_used;

static char driver_desc[256];
static CFDictionaryRef old_mode = NULL;
static CGrafPtr screen_port = NULL;
static CGGammaValue original_table[768];
static BITMAP *old_visible_bmp = NULL;



GFX_DRIVER gfx_quartz_full =
{
   GFX_QUARTZ_FULLSCREEN,
   empty_string,
   empty_string,
   "Quartz fullscreen",
   osx_qz_full_init,
   osx_qz_full_exit,
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   osx_qz_full_vsync,
   osx_qz_full_set_palette,
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   osx_qz_create_video_bitmap,   /* AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height)); */
   osx_qz_destroy_video_bitmap,  /* AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap)); */
   osx_qz_show_video_bitmap,     /* AL_METHOD(int, show_video_bitmap, (BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, request_video_bitmap, (BITMAP *bitmap)); */
   osx_qz_create_system_bitmap,  /* AL_METHOD(BITMAP *, create_system_bitmap, (int width, int height)); */
   osx_qz_destroy_video_bitmap,  /* AL_METHOD(void, destroy_system_bitmap, (BITMAP *bitmap)); */
   osx_mouse_set_sprite,         /* AL_METHOD(int, set_mouse_sprite, (BITMAP *sprite, int xfocus, int yfocus)); */
   osx_mouse_show,               /* AL_METHOD(int, show_mouse, (BITMAP *bmp, int x, int y)); */
   osx_mouse_hide,               /* AL_METHOD(void, hide_mouse, (void)); */
   osx_mouse_move,               /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   osx_qz_fetch_mode_list,       /* AL_METHOD(int, fetch_mode_list, (void)); */
   NULL,                         /* acknowledge_resize */
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   FALSE
};



void osx_init_fade_system(void)
{
   CGTableCount samples;

   CGGetDisplayTransferByTable(kCGDirectMainDisplay, 256, &original_table[0], &original_table[256], &original_table[512], &samples);
}



void osx_fade_screen(int fade_in, double seconds)
{
   int interval = (int)((seconds * 1000000.0) / (double)FADE_STEPS);
   int i, j;
   double factor;
   CGGammaValue table[768];

   for (i = 0; i < FADE_STEPS; i++) {
      if (fade_in)
         factor = (double)i / (double)FADE_STEPS;
      else
         factor = (double)(FADE_STEPS - i) / (double)FADE_STEPS;
      for (j = 0; j < 256; j++) {
         table[j] = original_table[j] * factor;
         table[256 + j] = original_table[256 + j] * factor;
         table[512 + j] = original_table[512 + j] * factor;
      }
      if (CGSetDisplayTransferByTable(kCGDirectMainDisplay, 256, &table[0], &table[256], &table[512]) != kCGErrorSuccess)
         break;
      usleep(interval);
   }
}



/* osx_qz_full_init:
 *  Initializes fullscreen gfx mode.
 */
static BITMAP *private_osx_qz_full_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   CFDictionaryRef mode = NULL;
   boolean_t match = FALSE;
   int bpp, refresh_rate;
   char tmp1[128];

   if (1
#ifdef ALLEGRO_COLOR8
       && (color_depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (color_depth != 15)
#endif
#ifdef ALLEGRO_COLOR32
       && (color_depth != 32)
#endif
       ) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
      return NULL;
   }

   if ((w == 0) && (h == 0)) {
      w = 320;
      h = 200;
   }

   if (v_w < w) v_w = w;
   if (v_h < h) v_h = h;

   if ((v_w != w) || (v_h != h)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }

   bpp = color_depth == 15 ? 16 : color_depth;
   if (_refresh_rate_request > 0)
      mode = CGDisplayBestModeForParametersAndRefreshRate(kCGDirectMainDisplay, bpp, w, h,
         (double)_refresh_rate_request, &match);
   if (!match)
      mode = CGDisplayBestModeForParameters(kCGDirectMainDisplay, bpp, w, h, &match);
   if (!match) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }

   osx_init_fade_system();
   old_mode = CGDisplayCurrentMode(kCGDirectMainDisplay);
   osx_fade_screen(FALSE, 0.2);
   if (CGDisplayCapture(kCGDirectMainDisplay) != kCGErrorSuccess) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot capture main display"));
      return NULL;
   }
   if (CGDisplaySwitchToMode(kCGDirectMainDisplay, mode) != kCGErrorSuccess) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot switch main display mode"));
      return NULL;
   }
   HideMenuBar();
   CGDisplayRestoreColorSyncSettings();

   CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayRefreshRate), kCFNumberSInt32Type, &refresh_rate);
   _set_current_refresh_rate(refresh_rate);

   if (CGDisplayCanSetPalette(kCGDirectMainDisplay))
      osx_palette = CGPaletteCreateDefaultColorPalette();

   bmp = _make_bitmap(w, h, (unsigned long)CGDisplayBaseAddress(kCGDirectMainDisplay),
                      &gfx_quartz_full, color_depth, CGDisplayBytesPerRow(kCGDirectMainDisplay));
   if (bmp)
      bmp->extra = calloc(1, sizeof(struct BMP_EXTRA_INFO));
   if ((!bmp) || (!bmp->extra)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return NULL;
   }

   screen_port = CreateNewPortForCGDisplayID((UInt32)kCGDirectMainDisplay);
   if (!screen_port) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot create DirectDisplay port"));
      return NULL;
   }
   BMP_EXTRA(bmp)->port = screen_port;

   setup_direct_shifts();

   gfx_quartz_full.w = w;
   gfx_quartz_full.h = h;
   gfx_quartz_full.vid_mem = w * h * BYTES_PER_PIXEL(color_depth);

   _screen_vtable.created_sub_bitmap = osx_qz_created_sub_bitmap;
   if (color_depth != 8)
      _screen_vtable.blit_to_self = osx_qz_blit_to_self;

   uszprintf(driver_desc, sizeof(driver_desc), uconvert_ascii("CoreGraphics DirectDisplay access, %d bpp", tmp1),
             color_depth);
   gfx_quartz_full.desc = driver_desc;

   osx_keyboard_focused(FALSE, 0);
   clear_keybuf();
   osx_gfx_mode = OSX_GFX_FULL;
   osx_skip_mouse_move = TRUE;
   osx_screen_used = FALSE;

   if (color_depth != 8) {
      gfx_quartz_full.set_mouse_sprite = osx_mouse_set_sprite;
      gfx_quartz_full.show_mouse = osx_mouse_show;
      gfx_quartz_full.hide_mouse = osx_mouse_hide;
      gfx_quartz_full.move_mouse = osx_mouse_move;
   }
   else {
      /* 8 bit modes have problems handling hardware cursor so we disable it */
      gfx_quartz_full.set_mouse_sprite = NULL;
      gfx_quartz_full.show_mouse = NULL;
      gfx_quartz_full.hide_mouse = NULL;
      gfx_quartz_full.move_mouse = NULL;
      CGDisplayHideCursor(kCGDirectMainDisplay);
   }

   old_visible_bmp = bmp;

   return bmp;
}

static BITMAP *osx_qz_full_init(int w, int h, int v_w, int v_h, int color_depth)
{
   BITMAP *bmp;
   _unix_lock_mutex(osx_event_mutex);
   bmp = private_osx_qz_full_init(w, h, v_w, v_h, color_depth);
   _unix_unlock_mutex(osx_event_mutex);
   if (!bmp)
      osx_qz_full_exit(bmp);
   return bmp;
}



/* osx_qz_full_exit:
 *  Shuts down fullscreen gfx mode.
 */
static void osx_qz_full_exit(BITMAP *bmp)
{
   _unix_lock_mutex(osx_event_mutex);

   if ((bmp) && (bmp->extra)) {
      if (BMP_EXTRA(bmp)->port)
         DisposeGWorld(BMP_EXTRA(bmp)->port);
      free(bmp->extra);
   }

   if (osx_palette) {
      CGPaletteRelease(osx_palette);
      osx_palette = NULL;
   }

   if (old_mode) {
      osx_fade_screen(FALSE, 0.1);
      CGDisplaySwitchToMode(kCGDirectMainDisplay, old_mode);
      CGDisplayRelease(kCGDirectMainDisplay);
      ShowMenuBar();
      if (bitmap_color_depth(bmp) == 8)
         CGDisplayShowCursor(kCGDirectMainDisplay);
      osx_fade_screen(TRUE, 0.2);
      CGDisplayRestoreColorSyncSettings();
      old_mode = NULL;
   }

   osx_gfx_mode = OSX_GFX_NONE;

   _unix_unlock_mutex(osx_event_mutex);
}



/* osx_qz_full_vsync:
 *  Quartz video vertical synchronization routine for fullscreen mode.
 */
static void osx_qz_full_vsync(void)
{
   CGDisplayWaitForBeamPositionOutsideLines(kCGDirectMainDisplay, 0, gfx_quartz_full.h / 2);
   CGDisplayWaitForBeamPositionOutsideLines(kCGDirectMainDisplay, gfx_quartz_full.h / 2, gfx_quartz_full.h - 1);
}



/* osx_qz_full_set_palette:
 *  Sets palette for quartz fullscreen.
 */
static void osx_qz_full_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int i;
   CGDeviceColor color;

   if (!CGDisplayCanSetPalette(kCGDirectMainDisplay))
      return;

   _unix_lock_mutex(osx_event_mutex);

   for (i = from; i <= to; i++) {
      color.red = ((float)p[i].r / 63.0);
      color.green = ((float)p[i].g / 63.0);
      color.blue = ((float)p[i].b / 63.0);
      CGPaletteSetColorAtIndex(osx_palette, color, i);
   }
   osx_palette_dirty = TRUE;

   _unix_unlock_mutex(osx_event_mutex);
}



/* osx_qz_show_video_bitmap:
 *  Displays a video bitmap on the screen by copying it using CopyBits
 *  (hw accelerated copy).
 */
static int osx_qz_show_video_bitmap(BITMAP *bmp)
{
   Rect rect;
   unsigned char *addr;
   int i;

   if ((bmp->w != gfx_quartz_full.w) || (bmp->h != gfx_quartz_full.h))
      return -1;

   SetRect(&rect, 0, 0, bmp->w, bmp->h);
   while (!QDDone(screen_port));
   while (!QDDone(BMP_EXTRA(bmp)->port));
   LockPortBits(screen_port);
   LockPortBits(BMP_EXTRA(bmp)->port);
   if (_wait_for_vsync)
      osx_qz_full_vsync();
   CopyBits(GetPortBitMapForCopyBits(BMP_EXTRA(bmp)->port),
            GetPortBitMapForCopyBits(screen_port),
            &rect, &rect, srcCopy, NULL);
   UnlockPortBits(screen_port);
   UnlockPortBits(BMP_EXTRA(bmp)->port);

   /* Since we're copying instead of page flipping, we have to make sure
    * the page now in the back does not have line pointers pointing to
    * the real screen, so we swap them with the page now in the front.
    */
   for (i = 0; i < bmp->h; i++) {
      addr = bmp->line[i];
      bmp->line[i] = old_visible_bmp->line[i];
      old_visible_bmp->line[i] = addr;
   }
   old_visible_bmp = bmp;

   return 0;
}



/* osx_qz_fetch_mode_list:
 *  Creates a list of available fullscreen video modes.
 */
static GFX_MODE_LIST *osx_qz_fetch_mode_list(void)
{
   GFX_MODE_LIST *gfx_mode_list = NULL;
   GFX_MODE *gfx_mode;
   CFArrayRef modes_list;
   CFDictionaryRef mode;
   int i, j, num_modes;
   int width, height, bpp;
   int already_stored;

   modes_list = CGDisplayAvailableModes(kCGDirectMainDisplay);
   if (!modes_list)
      return NULL;
   num_modes = CFArrayGetCount(modes_list);
   gfx_mode_list = (GFX_MODE_LIST *)malloc(sizeof(GFX_MODE_LIST));
   if (!gfx_mode_list)
      return NULL;
   gfx_mode_list->mode = NULL;
   gfx_mode_list->num_modes = 0;

   for (i = 0; i < num_modes; i++) {
      mode = CFArrayGetValueAtIndex(modes_list, i);

      CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayWidth), kCFNumberSInt32Type, &width);
      CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayHeight), kCFNumberSInt32Type, &height);
      CFNumberGetValue(CFDictionaryGetValue(mode, kCGDisplayBitsPerPixel), kCFNumberSInt32Type, &bpp);

      if (bpp == 16)
         bpp = 15;
      already_stored = FALSE;
      for (j = 0; j < gfx_mode_list->num_modes; j++) {
         if ((gfx_mode_list->mode[j].width == width) &&
             (gfx_mode_list->mode[j].height == height) &&
             (gfx_mode_list->mode[j].bpp == bpp)) {
            already_stored = TRUE;
            break;
         }
      }
      if (!already_stored) {
         gfx_mode_list->num_modes++;
         gfx_mode = (GFX_MODE *)realloc(gfx_mode_list->mode, sizeof(GFX_MODE) * (gfx_mode_list->num_modes + 1));
         if (!gfx_mode) {
            free(gfx_mode_list->mode);
            free(gfx_mode_list);
            return NULL;
         }
         gfx_mode_list->mode = gfx_mode;
         gfx_mode = &gfx_mode_list->mode[gfx_mode_list->num_modes - 1];
         gfx_mode->width = width;
         gfx_mode->height = height;
         gfx_mode->bpp = bpp;
      }
   }

   if (gfx_mode_list->mode) {
      gfx_mode_list->mode[gfx_mode_list->num_modes].width = 0;
      gfx_mode_list->mode[gfx_mode_list->num_modes].height = 0;
      gfx_mode_list->mode[gfx_mode_list->num_modes].bpp = 0;
   }
   else {
      free(gfx_mode_list);
      return NULL;
   }
   return gfx_mode_list;
}
