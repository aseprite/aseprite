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
 *      QNX Photon windowed gfx driver.
 *
 *      By Angelo Mottola.
 *
 *      Dirty rectangles mechanism by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintqnx.h"

#ifndef ALLEGRO_QNX
   #error Something is wrong with the makefile
#endif

#include <pthread.h>


static BITMAP *qnx_ph_init_win(int, int, int, int, int);
static void qnx_ph_exit_win(BITMAP *);
static void qnx_ph_vsync_win(void);
static void qnx_ph_set_palette_win(AL_CONST struct RGB *, int, int, int);
static BITMAP *qnx_ph_create_video_bitmap_win(int width, int height);
static void qnx_ph_destroy_video_bitmap_win(BITMAP *bmp);
static int qnx_ph_show_video_bitmap_win(BITMAP *bmp);


GFX_DRIVER gfx_photon_win =
{
   GFX_PHOTON,
   empty_string, 
   empty_string,
   "Photon windowed", 
   qnx_ph_init_win,
   qnx_ph_exit_win,
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   qnx_ph_vsync_win,
   qnx_ph_set_palette_win,
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   qnx_ph_create_video_bitmap_win,
   qnx_ph_destroy_video_bitmap_win,
   qnx_ph_show_video_bitmap_win,
   NULL,                         /* AL_METHOD(int, request_video_bitmap, (BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (BITMAP *bmp, int x, int y)); */
   NULL,                         /* AL_METHOD(void, hide_mouse, (void)); */
   NULL,                         /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   NULL,                         /* AL_METHOD(int, fetch_mode_list, (void)); */
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   TRUE
};


/* global variables */
BITMAP *pseudo_screen = NULL;
void (*ph_update_window)(PhRect_t* rect) = NULL;

/* exported only for qswitch.s */
char *ph_dirty_lines = NULL;

static PdOffscreenContext_t *ph_window_context = NULL;

static pthread_cond_t vsync_cond;
static pthread_mutex_t ph_screen_lock;
static int lock_nesting = 0;

static char driver_desc[256];
static COLORCONV_BLITTER_FUNC *colorconv_blitter = NULL;
static char *pseudo_screen_addr = NULL;
static int pseudo_screen_pitch;
static int pseudo_screen_depth;
static char *window_addr = NULL;
static int window_pitch;
static int desktop_depth;
static GFX_VTABLE _special_vtable; /* special vtable for offscreen bitmap */
static int reused_screen = FALSE;

#define RENDER_DELAY (1000/70)  /* 70 Hz */
static void update_dirty_lines(void);

static void ph_acquire_win(BITMAP *bmp);
static void ph_release_win(BITMAP *bmp);

#ifdef ALLEGRO_NO_ASM
static unsigned long ph_write_line_win(BITMAP *bmp, int line);
static void ph_unwrite_line_win(BITMAP *bmp);
#else
void (*ptr_ph_acquire_win)(BITMAP *) = &ph_acquire_win;
void (*ptr_ph_release_win)(BITMAP *) = &ph_release_win;
unsigned long ph_write_line_win_asm(BITMAP *bmp, int line);
void ph_unwrite_line_win_asm(BITMAP *bmp);
#endif



/* ph_acquire_win:
 *  Bitmap locking for Photon windowed mode.
 */
static void ph_acquire_win(BITMAP *bmp)
{
   /* to prevent the drawing threads and the rendering proc
      from concurrently accessing the dirty lines array */
   pthread_mutex_lock(&ph_screen_lock);

   PgWaitHWIdle();

   lock_nesting++;
   bmp->id |= BMP_ID_LOCKED;
}

 

/* ph_release_win:
 *  Bitmap unlocking for Photon windowed mode.
 */
static void ph_release_win(BITMAP *bmp)
{
   if (lock_nesting > 0) {   
      lock_nesting--;

      if (!lock_nesting)
         bmp->id &= ~BMP_ID_LOCKED;

      pthread_mutex_unlock(&ph_screen_lock);
   }
}


#ifdef ALLEGRO_NO_ASM

/* ph_write_line_win:
 *  Line switcher for Photon windowed mode.
 */
static unsigned long ph_write_line_win(BITMAP *bmp, int line)
{
   ph_dirty_lines[line + bmp->y_ofs] = 1;
   
   if (!(bmp->id & BMP_ID_LOCKED)) {
      ph_acquire(bmp);   
      bmp->id |= BMP_ID_AUTOLOCK;
   }

   return (unsigned long)(bmp->line[line]);
}



/* ph_unwrite_line_win:
 *  Line updater for Photon windowed mode.
 */
static void ph_unwrite_line_win(BITMAP *bmp)
{
   if (bmp->id & BMP_ID_AUTOLOCK) {
      ph_release(bmp);
      bmp->id &= ~BMP_ID_AUTOLOCK;
   }
}

#endif


/* update_window_hw:
 *  Window updater for matching color depths.
 */
static void update_window_hw(PhRect_t *rect)
{
   PhRect_t temp;
   
   if (!rect) {
      temp.ul.x = 0;
      temp.ul.y = 0;
      temp.lr.x = gfx_photon_win.w;
      temp.lr.y = gfx_photon_win.h;
      rect = &temp;
   }
   
   pthread_mutex_lock(&qnx_gfx_mutex);
   PgContextBlit(BMP_EXTRA(pseudo_screen)->context, rect, NULL, rect);
   pthread_mutex_unlock(&qnx_gfx_mutex);
}



/* update_window:
 *  Window updater for non matching color depths, performing color conversion.
 */
static void update_window(PhRect_t *rect)
{
   struct GRAPHICS_RECT src_gfx_rect, dest_gfx_rect;
   PhRect_t temp;
   
   if (!rect) {
      temp.ul.x = 0;
      temp.ul.y = 0;
      temp.lr.x = gfx_photon_win.w;
      temp.lr.y = gfx_photon_win.h;
      rect = &temp;
   }

   /* fill in source graphics rectangle description */
   src_gfx_rect.width  = rect->lr.x - rect->ul.x;
   src_gfx_rect.height = rect->lr.y - rect->ul.y;
   src_gfx_rect.pitch  = pseudo_screen_pitch;
   src_gfx_rect.data   = pseudo_screen->line[0] +
                         (rect->ul.y * pseudo_screen_pitch) +
                         (rect->ul.x * BYTES_PER_PIXEL(pseudo_screen_depth));

   /* fill in destination graphics rectangle description */
   dest_gfx_rect.pitch = window_pitch;
   dest_gfx_rect.data  = window_addr +
                         (rect->ul.y * window_pitch) +
                         (rect->ul.x * BYTES_PER_PIXEL(desktop_depth));
   
   /* function doing the hard work */
   colorconv_blitter(&src_gfx_rect, &dest_gfx_rect);

   pthread_mutex_lock(&qnx_gfx_mutex);
   PgContextBlit(ph_window_context, rect, NULL, rect);
   pthread_mutex_unlock(&qnx_gfx_mutex);
}



/* qnx_private_ph_init_win:
 *  Real screen initialization runs here.
 */
static BITMAP *qnx_private_ph_init_win(GFX_DRIVER *drv, int w, int h, int v_w, int v_h, int color_depth)
{
   PgDisplaySettings_t settings;
   PtArg_t arg;
   PhDim_t dim;
   char tmp1[128], tmp2[128];

   if (1
#ifdef ALLEGRO_COLOR8
       && (color_depth != 8)
#endif
#ifdef ALLEGRO_COLOR16
       && (color_depth != 15)
       && (color_depth != 16)
#endif
#ifdef ALLEGRO_COLOR24
       && (color_depth != 24)
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
   
   if ((v_w != w) || (v_h != h) || (w % 4)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }

   PgGetVideoMode(&settings);
   _set_current_refresh_rate(settings.refresh);
      
   /* retrieve infos for the selected mode */
   PgGetVideoModeInfo(settings.mode, &ph_gfx_mode_info);

   dim.w = w;
   dim.h = h;
   PtSetArg(&arg, Pt_ARG_DIM, &dim, 0);
   PtSetResources(ph_window, 1, &arg);

   ph_window_context = PdCreateOffscreenContext(0, w, h, Pg_OSC_MEM_PAGE_ALIGN);
   if (!ph_window_context) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot create offscreen context"));
      return NULL;
   }

   pseudo_screen_depth = color_depth;
   desktop_depth = ph_gfx_mode_info.bits_per_pixel;

   if (color_depth == desktop_depth) {
      /* the color depths match */
      ph_update_window = update_window_hw;
      
      /* the pseudo-screen associated with the window context */
      pseudo_screen = make_photon_bitmap(ph_window_context, w, h, BMP_ID_VIDEO);
   }
   else {
      /* the color depths don't match, need color conversion */
      colorconv_blitter = _get_colorconv_blitter(color_depth, desktop_depth);
      if (!colorconv_blitter) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
         return NULL;
      }

      ph_update_window = update_window;

      /* the window context is a pre-converted offscreen buffer */
      window_addr = PdGetOffscreenContextPtr(ph_window_context);
      window_pitch = ph_window_context->pitch;
      
      /* the pseudo-screen is a memory bitmap */
      pseudo_screen_addr = _AL_MALLOC(w * h * BYTES_PER_PIXEL(color_depth));
      if (!pseudo_screen_addr) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
         return NULL;
      }
      
      pseudo_screen_pitch = w * BYTES_PER_PIXEL(color_depth);      
      pseudo_screen = _make_bitmap(w, h, (unsigned long)pseudo_screen_addr,
                                   drv, color_depth, pseudo_screen_pitch);
   }

   if (!pseudo_screen) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return NULL;
   }   

   /* the last flag serves as an end of loop delimiter */
   ph_dirty_lines = _AL_MALLOC((h+1) * sizeof(char));
   ASSERT(ph_dirty_lines);
   memset(ph_dirty_lines, 0, (h+1) * sizeof(char));
   ph_dirty_lines[h] = 1;

   setup_driver(drv, w, h, color_depth);
   setup_direct_shifts();
      
   memcpy(&_special_vtable, &_screen_vtable, sizeof(GFX_VTABLE));
   pseudo_screen->vtable = &_special_vtable;
   _special_vtable.acquire = ph_acquire_win;
   _special_vtable.release = ph_release_win;

#ifdef ALLEGRO_NO_ASM
   pseudo_screen->write_bank = ph_write_line_win;
   _special_vtable.unwrite_bank = ph_unwrite_line_win;
#else
   pseudo_screen->write_bank = ph_write_line_win_asm;
   _special_vtable.unwrite_bank = ph_unwrite_line_win_asm;
#endif

   uszprintf(driver_desc, sizeof(driver_desc), uconvert_ascii("Photon windowed, %d bpp %s", tmp1), color_depth,
             uconvert_ascii(color_depth == desktop_depth ? "in matching" : "in fast emulation", tmp2));
   drv->desc = driver_desc;

   PgFlush();
   PgWaitHWIdle();

   pthread_cond_init(&vsync_cond, NULL);
   pthread_mutex_init(&ph_screen_lock, NULL);

   install_int(update_dirty_lines, RENDER_DELAY);

   return pseudo_screen;
}



/* qnx_private_ph_exit_win:
 *  This is where the video driver is actually closed.
 */
static void qnx_private_ph_exit_win(BITMAP *bmp)
{
   PhDim_t dim;
   
   if (bmp)
      clear_bitmap(bmp);
   
   remove_int(update_dirty_lines);
   
   PgSetPalette(ph_palette, 0, 0, -1, 0, 0);
   PgFlush();
   
   ph_gfx_mode = PH_GFX_NONE;
   
   if (ph_window_context) {
      PhDCRelease(ph_window_context);
      ph_window_context = NULL;
   }
   
   if (pseudo_screen_addr) {
      _AL_FREE(pseudo_screen_addr);
      pseudo_screen_addr = NULL;
   }
   
   if (ph_dirty_lines) {
      _AL_FREE(ph_dirty_lines);
      ph_dirty_lines = NULL;
   }
   
   if (colorconv_blitter) {
      _release_colorconv_blitter(colorconv_blitter);
      colorconv_blitter = NULL;
   }
   
   pthread_mutex_destroy(&ph_screen_lock);
   pthread_cond_destroy(&vsync_cond);
   
   dim.w = 1;
   dim.h = 1;
   PtSetResource(ph_window, Pt_ARG_DIM, &dim, 0);
}



/* qnx_ph_init_win:
 *  Initializes normal windowed Photon gfx driver.
 */
static BITMAP *qnx_ph_init_win(int w, int h, int v_w, int v_h, int color_depth)
{
   struct BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_event_mutex);
   
   ph_gfx_mode = PH_GFX_WINDOW;
   
   bmp = qnx_private_ph_init_win(&gfx_photon_win, w, h, v_w, v_h, color_depth);
   if (!bmp)
      qnx_private_ph_exit_win(NULL);
      
   pthread_mutex_unlock(&qnx_event_mutex);

   return bmp;
}



/* qnx_ph_exit_win:
 *  Shuts down photon windowed gfx driver.
 */
static void qnx_ph_exit_win(BITMAP *bmp)
{
   pthread_mutex_lock(&qnx_event_mutex);
   qnx_private_ph_exit_win(bmp);
   pthread_mutex_unlock(&qnx_event_mutex);
}



/* update_dirty_lines:
 *  Dirty line updater routine.
 */
static void update_dirty_lines(void)
{
   PhRect_t rect;

   /* to prevent the drawing threads and the rendering proc
      from concurrently accessing the dirty lines array */
   pthread_mutex_lock(&ph_screen_lock);

   PgWaitHWIdle();
   
   /* pseudo dirty rectangles mechanism:
    *  at most only one PgFlush() call is performed for each frame.
    */

   rect.ul.x = 0;
   rect.lr.x = gfx_photon_win.w;

   /* find the first dirty line */
   rect.ul.y = 0;

   while (!ph_dirty_lines[rect.ul.y])
      rect.ul.y++;

   if (rect.ul.y < gfx_photon_win.h) {
      /* find the last dirty line */
      rect.lr.y = gfx_photon_win.h;

      while (!ph_dirty_lines[rect.lr.y-1])
         rect.lr.y--;

      ph_update_window(&rect);
      
      /* clean up the dirty lines */
      while (rect.ul.y < rect.lr.y)
         ph_dirty_lines[rect.ul.y++] = 0;
   }
   
   pthread_mutex_unlock(&ph_screen_lock);
   
   /* simulate vertical retrace */
   pthread_cond_broadcast(&vsync_cond);
   
   PgFlush();
}



/* qnx_ph_vsync_win:
 *  Waits for vertical retrace.
 */
static void qnx_ph_vsync_win(void)
{
   pthread_mutex_lock(&ph_screen_lock);
   pthread_cond_wait(&vsync_cond, &ph_screen_lock);
   pthread_mutex_unlock(&ph_screen_lock);
}



/* qnx_ph_set_palette_win:
 *  Sets hardware palette.
 */
static void qnx_ph_set_palette_win(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int i;
   
   for (i=from; i<=to; i++)
      ph_palette[i] = (p[i].r << 18) | (p[i].g << 10) | (p[i].b << 2);
      
   if (desktop_depth == 8) {
      if (vsync)
         PgWaitVSync();
   
      PgSetPalette(ph_palette, 0, from, to - from + 1,
                   Pg_PALSET_HARDLOCKED | Pg_PALSET_FORCE_EXPOSE, 0);
      PgFlush();
   }
   else {
      pthread_mutex_lock(&ph_screen_lock);
   
      _set_colorconv_palette(p, from, to);
         
      /* invalidate the whole screen */      
      ph_dirty_lines[0] = ph_dirty_lines[gfx_photon_win.h-1] = 1;
      
      pthread_mutex_unlock(&ph_screen_lock);
   }
}



/* qnx_ph_create_video_bitmap_win:
 */
static BITMAP *qnx_ph_create_video_bitmap_win(int width, int height)
{
   PdOffscreenContext_t *context;
   struct BITMAP *bmp;

   if ((width == screen->w) && (height == screen->h)) {
      if (!reused_screen) {
         reused_screen = TRUE;
         return screen;
      }
   }

   if (colorconv_blitter) {
      /* If we are in color conversion mode, we fall back to
       * memory bitmaps because we don't have system bitmaps.
       */
      bmp = create_bitmap(width, height);
      if (!bmp)
         return NULL;
      
      /* emulate the video bitmap framework */
      bmp->id |= BMP_ID_VIDEO;
      bmp->vtable = &_screen_vtable;
      
   #ifdef ALLEGRO_NO_ASM
      bmp->write_bank = ph_write_line;
      bmp->read_bank = ph_write_line;
   #else
      bmp->write_bank = ph_write_line_asm;
      bmp->read_bank = ph_write_line_asm;
   #endif
   
      return bmp;
   }
   else {
      context = PdCreateOffscreenContext(0, width, height, Pg_OSC_MEM_PAGE_ALIGN);
      if (!context)
         return NULL;
         
      return make_photon_bitmap(context, width, height, BMP_ID_VIDEO);
   }
}



/* qnx_ph_destroy_video_bitmap_win:
 */
static void qnx_ph_destroy_video_bitmap_win(BITMAP *bmp)
{
   if (bmp == screen) {
      reused_screen = FALSE;
      return;
   }

   if (bmp == pseudo_screen) {
      /* in this case, 'bmp' points to the visible contents
       * but 'screen' doesn't, so we first invert that
       */
      qnx_ph_show_video_bitmap_win(screen);
   }

   if (colorconv_blitter) {
      bmp->id &= ~BMP_ID_VIDEO;
      destroy_bitmap(bmp);
   }
   else {
      PhDCRelease(BMP_EXTRA(bmp)->context);
      destroy_photon_bitmap(bmp);
   }
}



/* qnx_ph_show_video_bitmap_win:
 */
static int qnx_ph_show_video_bitmap_win(BITMAP *bmp)
{
   pthread_mutex_lock(&ph_screen_lock);

   /* turn the pseudo_screen into a normal video normal */
   pseudo_screen->vtable->acquire = ph_acquire;
   pseudo_screen->vtable->release = ph_release;
#ifdef ALLEGRO_NO_ASM
   pseudo_screen->write_bank = ph_write_line;
   pseudo_screen->vtable->unwrite_bank = ph_unwrite_line;
#else
   pseudo_screen->write_bank = ph_write_line_asm;
   pseudo_screen->vtable->unwrite_bank = ph_unwrite_line_asm;
#endif

   /* show the video bitmap */
   pseudo_screen = bmp;

   /* turn the video bitmap into the pseudo_screen */
   pseudo_screen->vtable->acquire = ph_acquire_win;
   pseudo_screen->vtable->release = ph_release_win;
#ifdef ALLEGRO_NO_ASM
   pseudo_screen->write_bank = ph_write_line_win;
   pseudo_screen->vtable->unwrite_bank = ph_unwrite_line_win;
#else
   pseudo_screen->write_bank = ph_write_line_win_asm;
   pseudo_screen->vtable->unwrite_bank = ph_unwrite_line_win_asm;
#endif
     
   /* invalidate the whole screen */
   ph_dirty_lines[0] = ph_dirty_lines[gfx_photon_win.h-1] = 1;
   
   pthread_mutex_unlock(&ph_screen_lock);
   
   return 0;
}
