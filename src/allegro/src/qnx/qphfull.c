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
 *      QNX Photon fullscreen gfx drivers.
 *
 *      By Angelo Mottola.
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


static BITMAP *qnx_ph_init_accel(int, int, int, int, int);
static BITMAP *qnx_ph_init_soft(int, int, int, int, int);
static BITMAP *qnx_ph_init_safe(int, int, int, int, int);
static GFX_MODE_LIST *qnx_ph_fetch_mode_list_accel(void);
static GFX_MODE_LIST *qnx_ph_fetch_mode_list_soft(void);
static GFX_MODE_LIST *qnx_ph_fetch_mode_list_safe(void);
static void qnx_ph_exit(BITMAP *);
static void qnx_ph_vsync(void);
static void qnx_ph_set_palette(AL_CONST struct RGB *, int, int, int);

/* requested capabilities for the drivers */
#define CAPS_ACCEL  (PgVM_MODE_CAP1_LINEAR | PgVM_MODE_CAP1_OFFSCREEN | \
                     PgVM_MODE_CAP1_DOUBLE_BUFFER | PgVM_MODE_CAP1_2D_ACCEL)
#define CAPS_SOFT   (PgVM_MODE_CAP1_LINEAR | PgVM_MODE_CAP1_OFFSCREEN | \
                     PgVM_MODE_CAP1_DOUBLE_BUFFER)
#define CAPS_SAFE   (PgVM_MODE_CAP1_LINEAR)


GFX_DRIVER gfx_photon_accel =
{
   GFX_PHOTON_ACCEL,
   empty_string, 
   empty_string,
   "Photon accelerated",
   qnx_ph_init_accel,
   qnx_ph_exit,
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   qnx_ph_vsync,
   qnx_ph_set_palette,
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   qnx_ph_create_video_bitmap,
   qnx_ph_destroy_video_bitmap,
   qnx_ph_show_video_bitmap,
   qnx_ph_request_video_bitmap,
   NULL,                         /* AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y)); */
   NULL,                         /* AL_METHOD(void, hide_mouse, (void)); */
   NULL,                         /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   qnx_ph_fetch_mode_list_accel,
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   FALSE
};


GFX_DRIVER gfx_photon_soft =
{
   GFX_PHOTON_SOFT,
   empty_string, 
   empty_string,
   "Photon soft",
   qnx_ph_init_soft,
   qnx_ph_exit,
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   qnx_ph_vsync,
   qnx_ph_set_palette,
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   qnx_ph_create_video_bitmap,
   qnx_ph_destroy_video_bitmap,
   qnx_ph_show_video_bitmap,
   qnx_ph_request_video_bitmap,
   NULL,                         /* AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y)); */
   NULL,                         /* AL_METHOD(void, hide_mouse, (void)); */
   NULL,                         /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   qnx_ph_fetch_mode_list_soft,
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   FALSE
};


GFX_DRIVER gfx_photon_safe =
{
   GFX_PHOTON_SAFE,
   empty_string, 
   empty_string,
   "Photon safe",
   qnx_ph_init_safe,
   qnx_ph_exit,
   NULL,                         /* AL_METHOD(int, scroll, (int x, int y)); */
   qnx_ph_vsync,
   qnx_ph_set_palette,
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, show_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, request_video_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(struct BITMAP *, create_system_bitmap, (int width, int height)); */
   NULL,                         /* AL_METHOD(void, destroy_system_bitmap, (struct BITMAP *bitmap)); */
   NULL,                         /* AL_METHOD(int, set_mouse_sprite, (struct BITMAP *sprite, int xfocus, int yfocus)); */
   NULL,                         /* AL_METHOD(int, show_mouse, (struct BITMAP *bmp, int x, int y)); */
   NULL,                         /* AL_METHOD(void, hide_mouse, (void)); */
   NULL,                         /* AL_METHOD(void, move_mouse, (int x, int y)); */
   NULL,                         /* AL_METHOD(void, drawing_mode, (void)); */
   NULL,                         /* AL_METHOD(void, save_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, restore_video_state, (void)); */
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   qnx_ph_fetch_mode_list_safe,
   0, 0,                         /* physical (not virtual!) screen size */
   TRUE,                         /* true if video memory is linear */
   0,                            /* bank size, in bytes */
   0,                            /* bank granularity, in bytes */
   0,                            /* video memory size, in bytes */
   0,                            /* physical address of video memory */
   FALSE
};


static PdDirectContext_t *ph_direct_context = NULL;
static PdOffscreenContext_t *ph_screen_context = NULL;

static char driver_desc[256];
static PgDisplaySettings_t original_settings;



/* find_video_mode:
 *  Scans the list of available video modes and returns the index of a
 *  suitable mode, -1 on failure.
 */
static int find_video_mode(int w, int h, int depth, unsigned int flags)
{
   PgVideoModes_t mode_list;
   PgVideoModeInfo_t mode_info;
   int i;
   
   if (PgGetVideoModeList(&mode_list))
      return -1;
      
   for (i=0; i<mode_list.num_modes; i++) {
      if ((!PgGetVideoModeInfo(mode_list.modes[i], &mode_info)) &&
          ((mode_info.mode_capabilities1 & flags) == flags) &&
          (mode_info.width == w) &&
          (mode_info.height == h) &&
          (mode_info.bits_per_pixel == depth)) {
         return mode_list.modes[i];
      }
   }
   
   return -1;
}



/* qnx_ph_fetch_mode_list:
 *  Generates a list of valid video modes.
 *  Returns the mode list on success or NULL on failure.
 */
static GFX_MODE_LIST *qnx_ph_fetch_mode_list(unsigned int flags)
{
   GFX_MODE_LIST *mode_list;
   PgVideoModes_t ph_mode_list;
   PgVideoModeInfo_t ph_mode_info;
   int i, count = 0;

   if (PgGetVideoModeList(&ph_mode_list))
      return NULL;

   mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!mode_list)
      return NULL;

   mode_list->mode = _AL_MALLOC(sizeof(GFX_MODE) * (ph_mode_list.num_modes + 1));
   if (!mode_list->mode) {
       _AL_FREE(mode_list);
       return NULL;
   }
   
   for (i=0; i<ph_mode_list.num_modes; i++) {
      if ((!PgGetVideoModeInfo(ph_mode_list.modes[i], &ph_mode_info)) &&
          ((ph_mode_info.mode_capabilities1 & flags) == flags)) {
         mode_list->mode[count].width = ph_mode_info.width;
         mode_list->mode[count].height = ph_mode_info.height;
         mode_list->mode[count].bpp = ph_mode_info.bits_per_pixel;
         count++;
      }
   }
   
   mode_list->mode[count].width = 0;
   mode_list->mode[count].height = 0;
   mode_list->mode[count].bpp = 0;

   mode_list->num_modes = count;

   return mode_list;
}
   

   
/* qnx_private_ph_init:
 *  Real screen initialization runs here.
 */
static BITMAP *qnx_private_ph_init(GFX_DRIVER *drv, int w, int h, int v_w, int v_h,
                                   int color_depth, int flags)
{
   static int first = 1;
   PhDim_t dim;
   PtArg_t arg;
   PgHWCaps_t caps;
   PgDisplaySettings_t settings;
   int mode_num;
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
      w = 640;
      h = 480;
   }
   
   if (v_w < w) v_w = w;
   if (v_h < h) v_h = h;
   
   if ((v_w != w) || (v_h != h)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }

   dim.w = w;
   dim.h = h;
   PtSetArg(&arg, Pt_ARG_DIM, &dim, 0);
   PtSetResources(ph_window, 1, &arg);
   
   if (first) {
      PgGetVideoMode(&original_settings);
      first = 0;
   }
   
   mode_num = find_video_mode(w, h, color_depth, flags);
   if (mode_num == -1) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }
   
   /* retrieve infos for the selected mode */
   PgGetVideoModeInfo(mode_num, &ph_gfx_mode_info);
   
   /* set the selected mode */
   settings.mode = mode_num;
   settings.refresh = _refresh_rate_request;
   settings.flags = 0;
   
   if (PgSetVideoMode(&settings)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }
   
   PgGetVideoMode(&settings);
   _set_current_refresh_rate(settings.refresh);
   
   if (!(ph_direct_context = PdCreateDirectContext())) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot create direct context"));
      PgSetVideoMode(&original_settings);
      return NULL;
   }
   
   if (!PdDirectStart(ph_direct_context)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot start direct context"));
      return NULL;
   }
   
   if (!(ph_screen_context = PdCreateOffscreenContext(0, 0, 0, Pg_OSC_MAIN_DISPLAY | Pg_OSC_MEM_PAGE_ALIGN))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot access the framebuffer"));
      return NULL;
   }
   
   ph_frontbuffer = make_photon_bitmap(ph_screen_context, w, h, BMP_ID_VIDEO);
   if (!ph_frontbuffer) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot access the framebuffer"));
      return NULL;
   }

   setup_driver(drv, w, h, color_depth);
   setup_direct_shifts();
   
   PgGetGraphicsHWCaps(&caps);
   uszprintf(driver_desc, sizeof(driver_desc), uconvert_ascii("Photon direct (%s)", tmp1),
             uconvert(caps.chip_name, U_UTF8, tmp2, U_CURRENT, sizeof(tmp2)));
   drv->desc = driver_desc;

   _mouse_on = TRUE;

   PgFlush();
   PgWaitHWIdle();

   return ph_frontbuffer;
}



/* qnx_private_ph_exit:
 *  This is where the video driver is actually closed.
 */
static void qnx_private_ph_exit(BITMAP *bmp)
{
   PhDim_t dim;

   if (bmp)
      clear_bitmap(bmp);
   
   ph_gfx_mode = PH_GFX_NONE;
   
   if (ph_screen_context) {
      PhDCRelease(ph_screen_context);
      ph_screen_context = NULL;
   }

   if (bmp)
      _AL_FREE(bmp->extra);

   if (ph_direct_context) {
      PdReleaseDirectContext(ph_direct_context);
      ph_direct_context = NULL;
      PgFlush();
      PgSetVideoMode(&original_settings);
   }
   
   dim.w = 1;
   dim.h = 1;
   PtSetResource(ph_window, Pt_ARG_DIM, &dim, 0);
}



/* qnx_ph_init_accel:
 *  Initializes the accelerated fullscreen Photon gfx driver.
 */
static BITMAP *qnx_ph_init_accel(int w, int h, int v_w, int v_h, int color_depth)
{
   struct BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_event_mutex);
   
   ph_gfx_mode = PH_GFX_DIRECT;
   
   bmp = qnx_private_ph_init(&gfx_photon_accel, w, h, v_w, v_h, color_depth, CAPS_ACCEL);
   if (bmp) {
      enable_acceleration(&gfx_photon_accel);
      enable_triple_buffering(&gfx_photon_accel);
   }
   else {
      qnx_private_ph_exit(NULL);
   }

   pthread_mutex_unlock(&qnx_event_mutex);

   return bmp;
}



/* qnx_ph_init_soft:
 *  Initializes the soft fullscreen Photon gfx driver.
 */
static BITMAP *qnx_ph_init_soft(int w, int h, int v_w, int v_h, int color_depth)
{
   struct BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_event_mutex);
   
   ph_gfx_mode = PH_GFX_DIRECT;
   
   bmp = qnx_private_ph_init(&gfx_photon_soft, w, h, v_w, v_h, color_depth, CAPS_SOFT);
   if (bmp) {
      enable_triple_buffering(&gfx_photon_soft);
   }
   else {
      qnx_private_ph_exit(NULL);
   }

   pthread_mutex_unlock(&qnx_event_mutex);

   return bmp;
}



/* qnx_ph_init_safe:
 *  Initializes the safe fullscreen Photon gfx driver.
 */
static BITMAP *qnx_ph_init_safe(int w, int h, int v_w, int v_h, int color_depth)
{
   struct BITMAP *bmp;
   
   pthread_mutex_lock(&qnx_event_mutex);
   
   ph_gfx_mode = PH_GFX_DIRECT;
   
   bmp = qnx_private_ph_init(&gfx_photon_safe, w, h, v_w, v_h, color_depth, CAPS_SAFE);
   if (!bmp)
      qnx_private_ph_exit(NULL);

   pthread_mutex_unlock(&qnx_event_mutex);

   return bmp;
}



/* qnx_ph_exit:
 *  Shuts down photon direct driver.
 */
static void qnx_ph_exit(BITMAP *bmp)
{
   pthread_mutex_lock(&qnx_event_mutex);
   qnx_private_ph_exit(bmp);
   pthread_mutex_unlock(&qnx_event_mutex);
}



/* qnx_ph_fetch_mode_list_accel:
 */
static GFX_MODE_LIST *qnx_ph_fetch_mode_list_accel(void)
{
   return qnx_ph_fetch_mode_list(CAPS_ACCEL);
}



/* qnx_ph_fetch_mode_list_soft:
 */
static GFX_MODE_LIST *qnx_ph_fetch_mode_list_soft(void)
{
   return qnx_ph_fetch_mode_list(CAPS_SOFT);
}



/* qnx_ph_fetch_mode_list_safe:
 */
static GFX_MODE_LIST *qnx_ph_fetch_mode_list_safe(void)
{
   return qnx_ph_fetch_mode_list(CAPS_SAFE);
}



/* qnx_ph_vsync:
 *  Waits for vertical retrace.
 */
static void qnx_ph_vsync(void)
{
   PgWaitVSync();
}



/* qnx_ph_set_palette:
 *  Sets hardware palette.
 */
static void qnx_ph_set_palette(AL_CONST struct RGB *p, int from, int to, int vsync)
{
   int i;
   
   for (i=from; i<=to; i++)
      ph_palette[i] = (p[i].r << 18) | (p[i].g << 10) | (p[i].b << 2);
      
   if (vsync)
      PgWaitVSync();

   PgSetPalette(ph_palette, 0, from, to - from + 1, Pg_PALSET_HARDLOCKED, 0);
      
   PgFlush();
}
