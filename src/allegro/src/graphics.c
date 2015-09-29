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
 *      Graphics mode set and bitmap creation routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"

extern void blit_end(void);   /* for LOCK_FUNCTION; defined in blit.c */



#define PREFIX_I                "al-gfx INFO: "
#define PREFIX_W                "al-gfx WARNING: "
#define PREFIX_E                "al-gfx ERROR: "



static int gfx_virgin = TRUE;          /* is the graphics system active? */

int _sub_bitmap_id_count = 1;          /* hash value for sub-bitmaps */

int _gfx_mode_set_count = 0;           /* has the graphics mode changed? */

int _screen_split_position = 0;        /* has the screen been split? */

int _safe_gfx_mode_change = 0;         /* are we getting through GFX_SAFE? */

RGB_MAP *rgb_map = NULL;               /* RGB -> palette entry conversion */

COLOR_MAP *color_map = NULL;           /* translucency/lighting table */

int _color_depth = 8;                  /* how many bits per pixel? */

int _refresh_rate_request = 0;         /* requested refresh rate */
static int current_refresh_rate = 0;   /* refresh rate set by the driver */

int _wait_for_vsync = TRUE;            /* vsync when page-flipping? */

int _color_conv = COLORCONV_TOTAL;     /* which formats to auto convert? */

static int color_conv_set = FALSE;     /* has the user set conversion mode? */

int _palette_color8[256];               /* palette -> pixel mapping */
int _palette_color15[256];
int _palette_color16[256];
int _palette_color24[256];
int _palette_color32[256];

int *palette_color = _palette_color8;

BLENDER_FUNC _blender_func15 = NULL;   /* truecolor pixel blender routines */
BLENDER_FUNC _blender_func16 = NULL;
BLENDER_FUNC _blender_func24 = NULL;
BLENDER_FUNC _blender_func32 = NULL;

BLENDER_FUNC _blender_func15x = NULL;
BLENDER_FUNC _blender_func16x = NULL;
BLENDER_FUNC _blender_func24x = NULL;

int _blender_col_15 = 0;               /* for truecolor lit sprites */
int _blender_col_16 = 0;
int _blender_col_24 = 0;
int _blender_col_32 = 0;

int _blender_alpha = 0;                /* for truecolor translucent drawing */

int _rgb_r_shift_15 = DEFAULT_RGB_R_SHIFT_15;     /* truecolor pixel format */
int _rgb_g_shift_15 = DEFAULT_RGB_G_SHIFT_15;
int _rgb_b_shift_15 = DEFAULT_RGB_B_SHIFT_15;
int _rgb_r_shift_16 = DEFAULT_RGB_R_SHIFT_16;
int _rgb_g_shift_16 = DEFAULT_RGB_G_SHIFT_16;
int _rgb_b_shift_16 = DEFAULT_RGB_B_SHIFT_16;
int _rgb_r_shift_24 = DEFAULT_RGB_R_SHIFT_24;
int _rgb_g_shift_24 = DEFAULT_RGB_G_SHIFT_24;
int _rgb_b_shift_24 = DEFAULT_RGB_B_SHIFT_24;
int _rgb_r_shift_32 = DEFAULT_RGB_R_SHIFT_32;
int _rgb_g_shift_32 = DEFAULT_RGB_G_SHIFT_32;
int _rgb_b_shift_32 = DEFAULT_RGB_B_SHIFT_32;
int _rgb_a_shift_32 = DEFAULT_RGB_A_SHIFT_32;


/* lookup table for scaling 5 bit colors up to 8 bits */
int _rgb_scale_5[32] =
{
   0,   8,   16,  24,  33,  41,  49,  57,
   66,  74,  82,  90,  99,  107, 115, 123,
   132, 140, 148, 156, 165, 173, 181, 189,
   198, 206, 214, 222, 231, 239, 247, 255
};


/* lookup table for scaling 6 bit colors up to 8 bits */
int _rgb_scale_6[64] =
{
   0,   4,   8,   12,  16,  20,  24,  28,
   32,  36,  40,  44,  48,  52,  56,  60,
   65,  69,  73,  77,  81,  85,  89,  93,
   97,  101, 105, 109, 113, 117, 121, 125,
   130, 134, 138, 142, 146, 150, 154, 158,
   162, 166, 170, 174, 178, 182, 186, 190,
   195, 199, 203, 207, 211, 215, 219, 223,
   227, 231, 235, 239, 243, 247, 251, 255
};


GFX_VTABLE _screen_vtable;             /* accelerated drivers change this */


typedef struct VRAM_BITMAP             /* list of video memory bitmaps */
{
   int x, y, w, h;
   BITMAP *bmp;
   struct VRAM_BITMAP *next_x, *next_y;
} VRAM_BITMAP;


static VRAM_BITMAP *vram_bitmap_list = NULL;


#define BMP_MAX_SIZE  46340   /* sqrt(INT_MAX) */

/* the product of these must fit in an int */
static int failed_bitmap_w = BMP_MAX_SIZE;
static int failed_bitmap_h = BMP_MAX_SIZE;



static int _set_gfx_mode(int card, int w, int h, int v_w, int v_h, int allow_config);
static int _set_gfx_mode_safe(int card, int w, int h, int v_w, int v_h);



/* lock_bitmap:
 *  Locks all the memory used by a bitmap structure.
 */
void lock_bitmap(BITMAP *bmp)
{
   LOCK_DATA(bmp, sizeof(BITMAP) + sizeof(char *) * bmp->h);

   if (bmp->dat) {
      LOCK_DATA(bmp->dat, bmp->w * bmp->h * BYTES_PER_PIXEL(bitmap_color_depth(bmp)));
   }
}



/* request_refresh_rate:
 *  Requests that the next call to set_gfx_mode() use the specified refresh
 *  rate.
 */
void request_refresh_rate(int rate)
{
   _refresh_rate_request = rate;
}



/* get_refresh_rate:
 *  Returns the refresh rate set by the most recent call to set_gfx_mode().
 */
int get_refresh_rate(void)
{
   return current_refresh_rate;
}



/* _set_current_refresh_rate:
 *  Sets the current refresh rate.
 *  (This function must be called by the gfx drivers)
 */
void _set_current_refresh_rate(int rate)
{
   /* sanity check to discard bogus values */
   if ((rate<40) || (rate>200))
      rate = 0;

   current_refresh_rate = rate;

   /* adjust retrace speed */
   _vsync_speed = rate ? BPS_TO_TIMER(rate) : BPS_TO_TIMER(70);
}



/* sort_gfx_mode_list:
 *  Callback for quick-sorting a mode-list.
 */
static int sort_gfx_mode_list(GFX_MODE *entry1, GFX_MODE *entry2)
{
   if (entry1->width > entry2->width) {
      return +1;
   }
   else if (entry1->width < entry2->width) {
      return -1;
   }
   else {
      if (entry1->height > entry2->height) {
         return +1;
      }
      else if (entry1->height < entry2->height) {
         return -1;
      }
      else {
         if (entry1->bpp > entry2->bpp) {
            return +1;
         }
         else if (entry1->bpp < entry2->bpp) {
            return -1;
         }
         else {
            return 0;
         }
      }
   }
}



/* get_gfx_mode_list:
 *  Attempts to create a list of all the supported video modes for a certain
 *  GFX driver.
 */
GFX_MODE_LIST *get_gfx_mode_list(int card)
{
   _DRIVER_INFO *list_entry;
   GFX_DRIVER *drv = NULL;
   GFX_MODE_LIST *gfx_mode_list = NULL;

   ASSERT(system_driver);

   /* ask the system driver for a list of graphics hardware drivers */
   if (system_driver->gfx_drivers)
      list_entry = system_driver->gfx_drivers();
   else
      list_entry = _gfx_driver_list;

   /* find the graphics driver, and if it can fetch mode lists, do so */
   while (list_entry->driver) {
      if (list_entry->id == card) {
         drv = list_entry->driver;
         if (!drv->fetch_mode_list)
            return NULL;

         gfx_mode_list = drv->fetch_mode_list();
         if (!gfx_mode_list)
            return NULL;

         break;
      }

      list_entry++;
   }

   if (!drv)
      return NULL;

   /* sort the list and finish */
   qsort(gfx_mode_list->mode, gfx_mode_list->num_modes, sizeof(GFX_MODE),
         (int (*) (AL_CONST void *, AL_CONST void *))sort_gfx_mode_list);

   return gfx_mode_list;
}



/* destroy_gfx_mode_list:
 *  Removes the mode list created by get_gfx_mode_list() from memory.
 */
void destroy_gfx_mode_list(GFX_MODE_LIST *gfx_mode_list)
{
   if (gfx_mode_list) {
      if (gfx_mode_list->mode)
         _AL_FREE(gfx_mode_list->mode);

      _AL_FREE(gfx_mode_list);
   }
}



/* set_color_depth:
 *  Sets the pixel size (in bits) which will be used by subsequent calls to
 *  set_gfx_mode() and create_bitmap(). Valid depths are 8, 15, 16, 24 and 32.
 */
void set_color_depth(int depth)
{
   _color_depth = depth;

   switch (depth) {
      case 8:  palette_color = _palette_color8;  break;
      case 15: palette_color = _palette_color15; break;
      case 16: palette_color = _palette_color16; break;
      case 24: palette_color = _palette_color24; break;
      case 32: palette_color = _palette_color32; break;
      default: ASSERT(FALSE);
   }
}



/* get_color_depth:
 *  Returns the current color depth.
 */
int get_color_depth(void)
{
   return _color_depth;
}



/* set_color_conversion:
 *  Sets a bit mask specifying which types of color format conversions are
 *  valid when loading data from disk.
 */
void set_color_conversion(int mode)
{
   _color_conv = mode;

   color_conv_set = TRUE;
}



/* get_color_conversion:
 *  Returns the bitmask specifying which types of color format
 *  conversion are valid when loading data from disk.
 */
int get_color_conversion(void)
{
   return _color_conv;
}



/* _color_load_depth:
 *  Works out which color depth an image should be loaded as, given the
 *  current conversion mode.
 */
int _color_load_depth(int depth, int hasalpha)
{
   typedef struct CONVERSION_FLAGS
   {
      int flag;
      int in_depth;
      int out_depth;
      int hasalpha;
   } CONVERSION_FLAGS;

   static CONVERSION_FLAGS conversion_flags[] =
   {
      { COLORCONV_8_TO_15,   8,  15, FALSE },
      { COLORCONV_8_TO_16,   8,  16, FALSE },
      { COLORCONV_8_TO_24,   8,  24, FALSE },
      { COLORCONV_8_TO_32,   8,  32, FALSE },
      { COLORCONV_15_TO_8,   15, 8,  FALSE },
      { COLORCONV_15_TO_16,  15, 16, FALSE },
      { COLORCONV_15_TO_24,  15, 24, FALSE },
      { COLORCONV_15_TO_32,  15, 32, FALSE },
      { COLORCONV_16_TO_8,   16, 8,  FALSE },
      { COLORCONV_16_TO_15,  16, 15, FALSE },
      { COLORCONV_16_TO_24,  16, 24, FALSE },
      { COLORCONV_16_TO_32,  16, 32, FALSE },
      { COLORCONV_24_TO_8,   24, 8,  FALSE },
      { COLORCONV_24_TO_15,  24, 15, FALSE },
      { COLORCONV_24_TO_16,  24, 16, FALSE },
      { COLORCONV_24_TO_32,  24, 32, FALSE },
      { COLORCONV_32_TO_8,   32, 8,  FALSE },
      { COLORCONV_32_TO_15,  32, 15, FALSE },
      { COLORCONV_32_TO_16,  32, 16, FALSE },
      { COLORCONV_32_TO_24,  32, 24, FALSE },
      { COLORCONV_32A_TO_8,  32, 8 , TRUE  },
      { COLORCONV_32A_TO_15, 32, 15, TRUE  },
      { COLORCONV_32A_TO_16, 32, 16, TRUE  },
      { COLORCONV_32A_TO_24, 32, 24, TRUE  }
   };

   int i;

   ASSERT((_gfx_mode_set_count > 0) || (color_conv_set));

   if (depth == _color_depth)
      return depth;

   for (i=0; i < (int)(sizeof(conversion_flags)/sizeof(CONVERSION_FLAGS)); i++) {
      if ((conversion_flags[i].in_depth == depth) &&
          (conversion_flags[i].out_depth == _color_depth) &&
          ((conversion_flags[i].hasalpha != 0) == (hasalpha != 0))) {
         if (_color_conv & conversion_flags[i].flag)
            return _color_depth;
         else
            return depth;
      }
   }

   ASSERT(FALSE);
   return 0;
}



/* _get_vtable:
 *  Returns a pointer to the linear vtable for the specified color depth.
 */
GFX_VTABLE *_get_vtable(int color_depth)
{
   GFX_VTABLE *vt;
   int i;

   ASSERT(system_driver);

   if (system_driver->get_vtable) {
      vt = system_driver->get_vtable(color_depth);

      if (vt) {
         LOCK_DATA(vt, sizeof(GFX_VTABLE));
         LOCK_CODE(vt->draw_sprite, (long)vt->draw_sprite_end - (long)vt->draw_sprite);
         LOCK_CODE(vt->blit_from_memory, (long)vt->blit_end - (long)vt->blit_from_memory);
         return vt;
      }
   }

   for (i=0; _vtable_list[i].vtable; i++) {
      if (_vtable_list[i].color_depth == color_depth) {
         LOCK_DATA(_vtable_list[i].vtable, sizeof(GFX_VTABLE));
         LOCK_CODE(_vtable_list[i].vtable->draw_sprite, (long)_vtable_list[i].vtable->draw_sprite_end - (long)_vtable_list[i].vtable->draw_sprite);
         LOCK_CODE(_vtable_list[i].vtable->blit_from_memory, (long)_vtable_list[i].vtable->blit_end - (long)_vtable_list[i].vtable->blit_from_memory);
         return _vtable_list[i].vtable;
      }
   }

   return NULL;
}



/* shutdown_gfx:
 *  Used by allegro_exit() to return the system to text mode.
 */
static void shutdown_gfx(void)
{
   if (gfx_driver)
      set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);

   if (system_driver->restore_console_state)
      system_driver->restore_console_state();

   _remove_exit_func(shutdown_gfx);

   gfx_virgin = TRUE;
}


#define GFX_DRIVER_FULLSCREEN_FLAG    0x01
#define GFX_DRIVER_WINDOWED_FLAG      0x02


/* gfx_driver_is_valid:
 *  Checks that the graphics driver 'drv' fulfills the condition
 *  expressed by the bitmask 'flags'.
 */
static int gfx_driver_is_valid(GFX_DRIVER *drv, int flags)
{
   if ((flags & GFX_DRIVER_FULLSCREEN_FLAG) && drv->windowed)
      return FALSE;

   if ((flags & GFX_DRIVER_WINDOWED_FLAG) && !drv->windowed)
      return FALSE;

   return TRUE;
}



/* get_gfx_driver_from_id:
 *  Retrieves a pointer to the graphics driver identified by 'card' from
 *  the list 'driver_list' or NULL if it doesn't exist.
 */
static GFX_DRIVER *get_gfx_driver_from_id(int card, _DRIVER_INFO *driver_list)
{
   int c;

   for (c=0; driver_list[c].driver; c++) {
      if (driver_list[c].id == card)
         return driver_list[c].driver;
   }

   return NULL;
}



/* init_gfx_driver:
 *  Helper function for initializing a graphics driver.
 */
static BITMAP *init_gfx_driver(GFX_DRIVER *drv, int w, int h, int v_w, int v_h)
{
   drv->name = drv->desc = get_config_text(drv->ascii_name);

   /* set gfx_driver so that it is visible when initializing the driver */
   gfx_driver = drv;

   return drv->init(w, h, v_w, v_h, _color_depth);
}



/* get_config_gfx_driver:
 *  Helper function for set_gfx_mode: it reads the gfx_card* config variables and
 *  tries to set the graphics mode if a matching driver is found. Returns TRUE if
 *  at least one matching driver was found, FALSE otherwise.
 */
static int get_config_gfx_driver(char *gfx_card, int w, int h, int v_w, int v_h, int flags, _DRIVER_INFO *driver_list)
{
   char buf[512], tmp[64];
   GFX_DRIVER *drv;
   int found = FALSE;
   int card, n;

   /* try the drivers that are listed in the config file */
   for (n=-2; n<255; n++) {
      switch (n) {

         case -2:
            /* example: gfx_card_640x480x16 = */
            uszprintf(buf, sizeof(buf), uconvert_ascii("%s_%dx%dx%d", tmp), gfx_card, w, h, _color_depth);
            break;

         case -1:
            /* example: gfx_card_24bpp = */
            uszprintf(buf, sizeof(buf), uconvert_ascii("%s_%dbpp", tmp), gfx_card, _color_depth);
            break;

         case 0:
            /* example: gfx_card = */
            uszprintf(buf, sizeof(buf), uconvert_ascii("%s", tmp), gfx_card);
            break;

         default:
            /* example: gfx_card1 = */
            uszprintf(buf, sizeof(buf), uconvert_ascii("%s%d", tmp), gfx_card, n);
            break;
      }

      card = get_config_id(uconvert_ascii("graphics", tmp), buf, 0);

      if (card) {
         drv = get_gfx_driver_from_id(card, driver_list);

         if (drv && gfx_driver_is_valid(drv, flags)) {
            found = TRUE;
            screen = init_gfx_driver(drv, w, h, v_w, v_h);

            if (screen)
               break;
         }
      }
      else {
         /* Stop searching the gfx_card#n (n>0) family at the first missing member,
          * except gfx_card1 which could have been identified with gfx_card.
          */
         if (n > 1)
            break;
      }
   }

   return found;
}



/* set_gfx_mode:
 *  Sets the graphics mode. The card should be one of the GFX_* constants
 *  from allegro.h, or GFX_AUTODETECT to accept any graphics driver. Pass
 *  GFX_TEXT to return to text mode (although allegro_exit() will usually
 *  do this for you). The w and h parameters specify the screen resolution
 *  you want, and v_w and v_h specify the minumum virtual screen size.
 *  The graphics drivers may actually create a much larger virtual screen,
 *  so you should check the values of VIRTUAL_W and VIRTUAL_H after you
 *  set the mode. If unable to select an appropriate mode, this function
 *  returns -1.
 */
int set_gfx_mode(int card, int w, int h, int v_w, int v_h)
{
   TRACE(PREFIX_I "Called set_gfx_mode(%d, %d, %d, %d, %d).\n",
         card, w, h, v_w, v_h);

   /* TODO: shouldn't this be incremented only IF successful? */
   _gfx_mode_set_count++;

   /* special bodge for the GFX_SAFE driver */
   if (card == GFX_SAFE)
      return _set_gfx_mode_safe(card, w, h, v_w, v_h);
   else
      return _set_gfx_mode(card, w, h, v_w, v_h, TRUE);
}



int acknowledge_resize(void)
{
   ASSERT(system_driver);
   ASSERT(gfx_driver);

   TRACE(PREFIX_I "acknowledge_resize init.\n");

   if (gfx_driver->acknowledge_resize) {
      /* Hide the mouse from current "screen" */
      if (screen && _mouse_installed)
         show_mouse(NULL);

      /* We've to change the global "screen" variable (even to NULL if
         the resize fails) because gfx_driver->acknowledge_resize()
         destroys the old screen pointer/video surfaces.  */
      screen = gfx_driver->acknowledge_resize();
      if (!screen) {
         TRACE(PREFIX_I "acknowledge_resize failed.\n");
         return -1;
      }

      TRACE(PREFIX_I "acknowledge_resize succeeded.\n");

      if (_al_linker_mouse)
        _al_linker_mouse->set_mouse_etc();

      return 0;
   }

   return 0;
}



/* _set_gfx_mode:
 *  Called by set_gfx_mode(). Separated to make a clear difference between
 *  the virtual GFX_SAFE driver and the rest. The allow_config parameter,
 *  if true, allows the configuration to override the graphics card/driver
 *  when using GFX_AUTODETECT.
 */
static int _set_gfx_mode(int card, int w, int h, int v_w, int v_h, int allow_config)
{
   _DRIVER_INFO *driver_list;
   GFX_DRIVER *drv;
   char tmp1[64], tmp2[64];
   AL_CONST char *dv;
   int flags = 0;
   int c;
   ASSERT(system_driver);
   ASSERT(card != GFX_SAFE);

   /* remember the current console state */
   if (gfx_virgin) {
      TRACE(PREFIX_I "First call, remembering console state.\n");
      LOCK_FUNCTION(_stub_bank_switch);
      LOCK_FUNCTION(blit);

      if (system_driver->save_console_state)
         system_driver->save_console_state();

      _add_exit_func(shutdown_gfx, "shutdown_gfx");

      gfx_virgin = FALSE;
   }

   timer_simulate_retrace(FALSE);
   _screen_split_position = 0;

   /* close down any existing graphics driver */
   if (gfx_driver) {
      TRACE(PREFIX_I "Closing graphics driver (%p) ", gfx_driver);
      TRACE("%s.\n", gfx_driver->ascii_name);
      if (_al_linker_mouse)
         _al_linker_mouse->show_mouse(NULL);

      while (vram_bitmap_list)
         destroy_bitmap(vram_bitmap_list->bmp);

      bmp_read_line(screen, 0);
      bmp_write_line(screen, 0);
      bmp_unwrite_line(screen);

      if (gfx_driver->scroll)
         gfx_driver->scroll(0, 0);

      if (gfx_driver->exit)
         gfx_driver->exit(screen);

      destroy_bitmap(screen);

      gfx_driver = NULL;
      screen = NULL;
      gfx_capabilities = 0;
   }

   /* We probably don't want to do this because it makes
    * Allegro "forget" the color layout of previously set
    * graphics modes. But it should be retained if bitmaps
    * created in those modes are to be used in the new mode.
    */
#if 0
   /* restore default truecolor pixel format */
   _rgb_r_shift_15 = 0;
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 10;
   _rgb_r_shift_16 = 0;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 11;
   _rgb_r_shift_24 = 0;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 16;
   _rgb_r_shift_32 = 0;
   _rgb_g_shift_32 = 8;
   _rgb_b_shift_32 = 16;
   _rgb_a_shift_32 = 24;
#endif

   gfx_capabilities = 0;

   _set_current_refresh_rate(0);

   /* return to text mode? */
   if (card == GFX_TEXT) {
      TRACE(PREFIX_I "Closing, restoring original console state.\n");
      if (system_driver->restore_console_state)
         system_driver->restore_console_state();

      if (_gfx_bank) {
         _AL_FREE(_gfx_bank);
         _gfx_bank = NULL;
      }

      TRACE(PREFIX_I "Graphic mode closed.\n");
      return 0;
   }

   /* now to the interesting part: let's try to find a graphics driver */
   usetc(allegro_error, 0);

   /* ask the system driver for a list of graphics hardware drivers */
   if (system_driver->gfx_drivers)
      driver_list = system_driver->gfx_drivers();
   else
      driver_list = _gfx_driver_list;

   /* filter specific fullscreen/windowed driver requests */
   if (card == GFX_AUTODETECT_FULLSCREEN) {
      flags |= GFX_DRIVER_FULLSCREEN_FLAG;
      card = GFX_AUTODETECT;
   }
   else if (card == GFX_AUTODETECT_WINDOWED) {
      flags |= GFX_DRIVER_WINDOWED_FLAG;
      card = GFX_AUTODETECT;
   }

   if (card == GFX_AUTODETECT) {
      /* autodetect the driver */
      int found = FALSE;

      tmp1[0] = '\0';

      /* first try the config variables */
      if (allow_config) {
         /* try the gfx_card variable if GFX_AUTODETECT or GFX_AUTODETECT_FULLSCREEN was selected */
         if (!(flags & GFX_DRIVER_WINDOWED_FLAG))
            found = get_config_gfx_driver(uconvert_ascii("gfx_card", tmp1), w, h, v_w, v_h, flags, driver_list);

         /* try the gfx_cardw variable if GFX_AUTODETECT or GFX_AUTODETECT_WINDOWED was selected */
         if (!(flags & GFX_DRIVER_FULLSCREEN_FLAG) && !found)
            found = get_config_gfx_driver(uconvert_ascii("gfx_cardw", tmp1), w, h, v_w, v_h, flags, driver_list);
      }

      /* go through the list of autodetected drivers if none was previously found */
      if (!found) {
         TRACE(PREFIX_I "Autodetecting graphic driver.\n");
         for (c=0; driver_list[c].driver; c++) {
            if (driver_list[c].autodetect) {
               drv = driver_list[c].driver;

               if (gfx_driver_is_valid(drv, flags)) {
                  screen = init_gfx_driver(drv, w, h, v_w, v_h);

                  if (screen)
                     break;
               }
            }
         }
      }
      else {
         TRACE(PREFIX_I "GFX_AUTODETECT overridden through configuration:"
               " %s.\n", tmp1);
      }
   }
   else {
      /* search the list for the requested driver */
      drv = get_gfx_driver_from_id(card, driver_list);

      if (drv)
         screen = init_gfx_driver(drv, w, h, v_w, v_h);
   }

   /* gracefully handle failure */
   if (!screen) {
      gfx_driver = NULL;  /* set by init_gfx_driver() */

      if (!ugetc(allegro_error))
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unable to find a suitable graphics driver"));

      TRACE(PREFIX_E "Failed setting graphic driver %d.\n", card);
      return -1;
   }

   /* set the basic capabilities of the driver */
   if ((VIRTUAL_W > SCREEN_W) || (VIRTUAL_H > SCREEN_H)) {
      if (gfx_driver->scroll)
         gfx_capabilities |= GFX_CAN_SCROLL;

      if ((gfx_driver->request_scroll) || (gfx_driver->request_video_bitmap))
         gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
   }

   /* check whether we are instructed to disable vsync */
   dv = get_config_string(uconvert_ascii("graphics", tmp1),
                          uconvert_ascii("disable_vsync", tmp2),
                          NULL);

   if ((dv) && ((c = ugetc(dv)) != 0) && ((c == 'y') || (c == 'Y') || (c == '1')))
      _wait_for_vsync = FALSE;
   else
      _wait_for_vsync = TRUE;

   TRACE(PREFIX_I "The driver %s wait for vsync.\n",
         (_wait_for_vsync) ? "will" : "won't");

   /* Give the gfx driver an opportunity to set the drawing mode */
   if ((gfx_driver->drawing_mode) && (!_dispsw_status))
      gfx_driver->drawing_mode();

   clear_bitmap(screen);

   /* set up the default colors */
   for (c=0; c<256; c++)
      _palette_color8[c] = c;

   set_palette(default_palette);

   if (_al_linker_mouse)
      _al_linker_mouse->set_mouse_etc();

   LOCK_DATA(gfx_driver, sizeof(GFX_DRIVER));

   _register_switch_bitmap(screen, NULL);

   TRACE(PREFIX_I "set_gfx_card success for %dx%dx%d.\n",
         screen->w, screen->h, bitmap_color_depth(screen));
   return 0;
}



/* _set_gfx_mode_safe:
 *  Special wrapper used when the card parameter of set_gfx_mode()
 *  is GFX_SAFE. In this case the function tries to query the
 *  system driver for a "safe" resolution+driver it knows it will
 *  work, and set it. If the system driver cannot get a "safe"
 *  resolution+driver, it will try the given parameters.
 */
static int _set_gfx_mode_safe(int card, int w, int h, int v_w, int v_h)
{
   char buf[ALLEGRO_ERROR_SIZE], tmp1[64];
   struct GFX_MODE mode;
   int ret, driver;

   ASSERT(card == GFX_SAFE);
   ASSERT(system_driver);
   TRACE(PREFIX_I "Trying to set a safe graphics mode.\n");

   if (system_driver->get_gfx_safe_mode) {
      ustrzcpy(buf, sizeof(buf), allegro_error);

      /* retrieve the safe graphics mode */
      system_driver->get_gfx_safe_mode(&driver, &mode);
      TRACE(PREFIX_I "The system driver suggests %dx%dx%d\n",
            mode.width, mode.height, mode.bpp);

      /* try using the specified resolution but current depth */
      if (_set_gfx_mode(driver, w, h, 0, 0, TRUE) == 0)
         return 0;

      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, buf);

      /* finally use the safe settings */
      set_color_depth(mode.bpp);
      if (_set_gfx_mode(driver, mode.width, mode.height, 0, 0, TRUE) == 0)
         return 0;

      ASSERT(FALSE);  /* the safe graphics mode must always work */
   }
   else {
      TRACE(PREFIX_W "The system driver was unable to get a safe mode, "
            "I'll try with the specified parameters...\n");
      /* no safe graphics mode, try hard-coded autodetected modes with
       * custom settings */
      _safe_gfx_mode_change = 1;

      ret = _set_gfx_mode(GFX_AUTODETECT, w, h, 0, 0, TRUE);

      _safe_gfx_mode_change = 0;

      if (ret == 0)
         return 0;
   }

   /* failing to set GFX_SAFE is a fatal error */
   TRACE(PREFIX_E "Bad bad, not even GFX_SAFE works?\n");
   _set_gfx_mode(GFX_TEXT, 0, 0, 0, 0, TRUE);
   allegro_message(uconvert_ascii("%s\n", tmp1),
                   get_config_text("Fatal error: unable to set GFX_SAFE"));
   return -1;
}



/* _sort_out_virtual_width:
 *  Decides how wide the virtual screen really needs to be. That is more
 *  complicated than it sounds, because the Allegro graphics primitives
 *  require that each scanline be contained within a single bank. That
 *  causes problems on cards that don't have overlapping banks, unless the
 *  bank size is a multiple of the virtual width. So we may need to adjust
 *  the width just to keep things running smoothly...
 */
void _sort_out_virtual_width(int *width, GFX_DRIVER *driver)
{
   int w = *width;

   /* hah! I love VBE 2.0... */
   if (driver->linear)
      return;

   /* if banks can overlap, we are ok... */
   if (driver->bank_size > driver->bank_gran)
      return;

   /* damn, have to increase the virtual width */
   while (((driver->bank_size / w) * w) != driver->bank_size) {
      w++;
      if (w > driver->bank_size)
         break; /* oh shit */
   }

   *width = w;
}



/* _make_bitmap:
 *  Helper function for creating the screen bitmap. Sets up a bitmap
 *  structure for addressing video memory at addr, and fills the bank
 *  switching table using bank size/granularity information from the
 *  specified graphics driver.
 */
BITMAP *_make_bitmap(int w, int h, uintptr_t addr, GFX_DRIVER *driver, int color_depth, int bpl)
{
   GFX_VTABLE *vtable = _get_vtable(color_depth);
   int i, bank, size;
   BITMAP *b;

   if (!vtable)
      return NULL;

   size = sizeof(BITMAP) + sizeof(char *) * h;

   b = (BITMAP *)_AL_MALLOC(size);
   if (!b)
      return NULL;

   _gfx_bank = _AL_REALLOC(_gfx_bank, h * sizeof(int));
   if (!_gfx_bank) {
      _AL_FREE(b);
      return NULL;
   }

   LOCK_DATA(b, size);
   LOCK_DATA(_gfx_bank, h * sizeof(int));

   b->w = b->cr = w;
   b->h = b->cb = h;
   b->clip = TRUE;
   b->cl = b->ct = 0;
   b->vtable = &_screen_vtable;
   b->write_bank = b->read_bank = _stub_bank_switch;
   b->dat = NULL;
   b->id = BMP_ID_VIDEO;
   b->extra = NULL;
   b->x_ofs = 0;
   b->y_ofs = 0;
   b->seg = _video_ds();

   memcpy(&_screen_vtable, vtable, sizeof(GFX_VTABLE));
   LOCK_DATA(&_screen_vtable, sizeof(GFX_VTABLE));

   _last_bank_1 = _last_bank_2 = -1;

   driver->vid_phys_base = addr;

   b->line[0] = (unsigned char *)addr;
   _gfx_bank[0] = 0;

   if (driver->linear) {
      for (i=1; i<h; i++) {
         b->line[i] = b->line[i-1] + bpl;
         _gfx_bank[i] = 0;
      }
   }
   else {
      bank = 0;

      for (i=1; i<h; i++) {
         b->line[i] = b->line[i-1] + bpl;
         if (b->line[i]+bpl-1 >= (unsigned char *)addr + driver->bank_size) {
            while (b->line[i] >= (unsigned char *)addr + driver->bank_gran) {
               b->line[i] -= driver->bank_gran;
               bank++;
            }
         }
         _gfx_bank[i] = bank;
      }
   }

   return b;
}



/* create_bitmap_ex
 *  Creates a new memory bitmap in the specified color_depth
 */
BITMAP *create_bitmap_ex(int color_depth, int width, int height)
{
   GFX_VTABLE *vtable;
   BITMAP *bitmap;
   int nr_pointers;
   int padding;
   int i;

   ASSERT(width >= 0);
   ASSERT(height > 0);
   ASSERT(system_driver);

   if (system_driver->create_bitmap)
      return system_driver->create_bitmap(color_depth, width, height);

   vtable = _get_vtable(color_depth);
   if (!vtable)
      return NULL;

   /* We need at least two pointers when drawing, otherwise we get crashes with
    * Electric Fence.  We think some of the assembly code assumes a second line
    * pointer is always available.
    */
   nr_pointers = MAX(2, height);
   bitmap = _AL_MALLOC(sizeof(BITMAP) + (sizeof(char *) * nr_pointers));
   if (!bitmap)
      return NULL;

   /* This avoids a crash for assembler code accessing the last pixel, as it
    * read 4 bytes instead of 3.
    */
   padding = (color_depth == 24) ? 1 : 0;

   bitmap->dat = _AL_MALLOC_ATOMIC(width * height * BYTES_PER_PIXEL(color_depth) + padding);
   if (!bitmap->dat) {
      _AL_FREE(bitmap);
      return NULL;
   }

   bitmap->w = bitmap->cr = width;
   bitmap->h = bitmap->cb = height;
   bitmap->clip = TRUE;
   bitmap->cl = bitmap->ct = 0;
   bitmap->vtable = vtable;
   bitmap->write_bank = bitmap->read_bank = _stub_bank_switch;
   bitmap->id = 0;
   bitmap->extra = NULL;
   bitmap->x_ofs = 0;
   bitmap->y_ofs = 0;
   bitmap->seg = _default_ds();

   if (height > 0) {
      bitmap->line[0] = bitmap->dat;
      for (i=1; i<height; i++)
         bitmap->line[i] = bitmap->line[i-1] + width * BYTES_PER_PIXEL(color_depth);
   }

   if (system_driver->created_bitmap)
      system_driver->created_bitmap(bitmap);

   return bitmap;
}



/* create_bitmap:
 *  Creates a new memory bitmap.
 */
BITMAP *create_bitmap(int width, int height)
{
   ASSERT(width >= 0);
   ASSERT(height > 0);
   return create_bitmap_ex(_color_depth, width, height);
}



/* create_sub_bitmap:
 *  Creates a sub bitmap, ie. a bitmap sharing drawing memory with a
 *  pre-existing bitmap, but possibly with different clipping settings.
 *  Usually will be smaller, and positioned at some arbitrary point.
 *
 *  Mark Wodrich is the owner of the brain responsible this hugely useful
 *  and beautiful function.
 */
BITMAP *create_sub_bitmap(BITMAP *parent, int x, int y, int width, int height)
{
   BITMAP *bitmap;
   int nr_pointers;
   int i;

   ASSERT(parent);
   ASSERT((x >= 0) && (y >= 0) && (x < parent->w) && (y < parent->h));
   ASSERT((width > 0) && (height > 0));
   ASSERT(system_driver);

   if (x+width > parent->w)
      width = parent->w-x;

   if (y+height > parent->h)
      height = parent->h-y;

   if (parent->vtable->create_sub_bitmap)
      return parent->vtable->create_sub_bitmap(parent, x, y, width, height);

   if (system_driver->create_sub_bitmap)
      return system_driver->create_sub_bitmap(parent, x, y, width, height);

   /* get memory for structure and line pointers */
   /* (see create_bitmap for the reason we need at least two) */
   nr_pointers = MAX(2, height);
   bitmap = _AL_MALLOC(sizeof(BITMAP) + (sizeof(char *) * nr_pointers));
   if (!bitmap)
      return NULL;

   acquire_bitmap(parent);

   bitmap->w = bitmap->cr = width;
   bitmap->h = bitmap->cb = height;
   bitmap->clip = TRUE;
   bitmap->cl = bitmap->ct = 0;
   bitmap->vtable = parent->vtable;
   bitmap->write_bank = parent->write_bank;
   bitmap->read_bank = parent->read_bank;
   bitmap->dat = NULL;
   bitmap->extra = NULL;
   bitmap->x_ofs = x + parent->x_ofs;
   bitmap->y_ofs = y + parent->y_ofs;
   bitmap->seg = parent->seg;

   /* All bitmaps are created with zero ID's. When a sub-bitmap is created,
    * a unique ID is needed to identify the relationship when blitting from
    * one to the other. This is obtained from the global variable
    * _sub_bitmap_id_count, which provides a sequence of integers (yes I
    * know it will wrap eventually, but not for a long time :-) If the
    * parent already has an ID the sub-bitmap adopts it, otherwise a new
    * ID is given to both the parent and the child.
    */
   if (!(parent->id & BMP_ID_MASK)) {
      parent->id |= _sub_bitmap_id_count;
      _sub_bitmap_id_count = (_sub_bitmap_id_count+1) & BMP_ID_MASK;
   }

   bitmap->id = parent->id | BMP_ID_SUB;
   bitmap->id &= ~BMP_ID_LOCKED;

   if (is_planar_bitmap(bitmap))
      x /= 4;

   x *= BYTES_PER_PIXEL(bitmap_color_depth(bitmap));

   /* setup line pointers: each line points to a line in the parent bitmap */
   for (i=0; i<height; i++)
      bitmap->line[i] = parent->line[y+i] + x;

   if (bitmap->vtable->set_clip)
      bitmap->vtable->set_clip(bitmap);

   if (parent->vtable->created_sub_bitmap)
      parent->vtable->created_sub_bitmap(bitmap, parent);

   if (system_driver->created_sub_bitmap)
      system_driver->created_sub_bitmap(bitmap, parent);

   if (parent->id & BMP_ID_VIDEO)
      _register_switch_bitmap(bitmap, parent);

   release_bitmap(parent);

   return bitmap;
}



/* add_vram_block:
 *  Creates a video memory bitmap out of the specified region
 *  of the larger screen surface, returning a pointer to it.
 */
static BITMAP *add_vram_block(int x, int y, int w, int h)
{
   VRAM_BITMAP *b, *new_b;
   VRAM_BITMAP **last_p;

   new_b = _AL_MALLOC(sizeof(VRAM_BITMAP));
   if (!new_b)
      return NULL;

   new_b->x = x;
   new_b->y = y;
   new_b->w = w;
   new_b->h = h;

   new_b->bmp = create_sub_bitmap(screen, x, y, w, h);
   if (!new_b->bmp) {
      _AL_FREE(new_b);
      return NULL;
   }

   /* find sorted y-position */
   last_p = &vram_bitmap_list;
   for (b = vram_bitmap_list; b && (b->y < new_b->y); b = b->next_y)
      last_p = &b->next_y;

   /* insert */
   *last_p = new_b;
   new_b->next_y = b;

   return new_b->bmp;
}



/* create_video_bitmap:
 *  Attempts to make a bitmap object for accessing offscreen video memory.
 *
 *  The algorithm is similar to algorithms for drawing polygons. Think of
 *  a horizontal stripe whose height is equal to that of the new bitmap.
 *  It is initially aligned to the top of video memory and then it moves
 *  downwards, stopping each time its top coincides with the bottom of a
 *  video bitmap. For each stop, create a list of video bitmaps intersecting
 *  the stripe, sorted by the left coordinate, from left to right, in the
 *  next_x linked list. We look through this list and stop as soon as the
 *  gap between the rightmost right edge seen so far and the current left
 *  edge is big enough for the new video bitmap. In that case, we are done.
 *  If we don't find such a gap, we move the stripe further down.
 *
 *  To make it efficient to find new bitmaps intersecting the stripe, the
 *  list of all bitmaps is always sorted by top, from top to bottom, in
 *  the next_y linked list. The list of bitmaps intersecting the stripe is
 *  merely updated, not recalculated from scratch, when we move the stripe.
 *  So every bitmap gets bubbled to its correct sorted x-position at most
 *  once. Bubbling a bitmap takes at most as many steps as the number of
 *  video bitmaps. So the algorithm behaves at most quadratically with
 *  regard to the number of video bitmaps. I think that a linear behaviour
 *  is more typical in practical applications (this happens, for instance,
 *  if you allocate many bitmaps of the same height).
 *
 *  There's one more optimization: if a call fails, we cache the size of the
 *  requested bitmap, so that next time we can detect very quickly that this
 *  size is too large (this needs to be maintained when destroying bitmaps).
 */
BITMAP *create_video_bitmap(int width, int height)
{
   VRAM_BITMAP *active_list, *b, *vram_bitmap;
   VRAM_BITMAP **last_p;
   BITMAP *bmp;
   int x = 0, y = 0;

   ASSERT(width >= 0);
   ASSERT(height > 0);

   if (_dispsw_status)
      return NULL;

   /* let the driver handle the request if it can */
   if (gfx_driver->create_video_bitmap) {
      bmp = gfx_driver->create_video_bitmap(width, height);
      if (!bmp)
         return NULL;

      b = _AL_MALLOC(sizeof(VRAM_BITMAP));
      b->x = -1;
      b->y = -1;
      b->w = 0;
      b->h = 0;
      b->bmp = bmp;
      b->next_y = vram_bitmap_list;
      vram_bitmap_list = b;

      return bmp;
   }

   /* check bad args */
   if ((width > VIRTUAL_W) || (height > VIRTUAL_H) ||
       (width < 0) || (height < 0))
      return NULL;

   /* check cached bitmap size */
   if ((width >= failed_bitmap_w) && (height >= failed_bitmap_h))
      return NULL;

   vram_bitmap = vram_bitmap_list;
   active_list = NULL;
   y = 0;

   while (TRUE) {
      /* Move the blocks from the VRAM_BITMAP_LIST that intersect the
       * stripe to the ACTIVE_LIST: look through the VRAM_BITMAP_LIST
       * following the next_y link, grow the ACTIVE_LIST following
       * the next_x link and sorting by x-position.
       * (this loop runs in amortized quadratic time)
       */
      while (vram_bitmap && (vram_bitmap->y < y+height)) {
         /* find sorted x-position */
         last_p = &active_list;
         for (b = active_list; b && (vram_bitmap->x > b->x); b = b->next_x)
            last_p = &b->next_x;

         /* insert */
         *last_p = vram_bitmap;
         vram_bitmap->next_x = b;

         /* next video bitmap */
         vram_bitmap = vram_bitmap->next_y;
      }

      x = 0;

      /* Look for holes to put our bitmap in.
       * (this loop runs in quadratic time)
       */
      for (b = active_list; b; b = b->next_x) {
         if (x+width <= b->x)  /* hole is big enough? */
            return add_vram_block(x, y, width, height);

         /* Move search x-position to the right edge of the
          * skipped bitmap if we are not already past it.
          * And check there is enough room before continuing.
          */
         if (x < b->x + b->w) {
            x = (b->x + b->w + 15) & ~15;
            if (x+width > VIRTUAL_W)
               break;
         }
      }

      /* If the whole ACTIVE_LIST was scanned, there is a big
       * enough hole to the right of the rightmost bitmap.
       */
      if (b == NULL)
         return add_vram_block(x, y, width, height);

      /* Move search y-position to the topmost bottom edge
       * of the bitmaps intersecting the stripe.
       * (this loop runs in quadratic time)
       */
      y = active_list->y + active_list->h;
      for (b = active_list->next_x; b; b = b->next_x) {
         if (y > b->y + b->h)
            y = b->y + b->h;
      }

      if (y+height > VIRTUAL_H) {  /* too close to bottom? */
         /* Before failing, cache this bitmap size so that later calls can
          * learn from it. Use area as a measure to sort the bitmap sizes.
          */
         if (width * height < failed_bitmap_w * failed_bitmap_h) {
            failed_bitmap_w = width;
            failed_bitmap_h = height;
         }

         return NULL;
      }

      /* Remove the blocks that don't intersect the new stripe from ACTIVE_LIST.
       * (this loop runs in quadratic time)
       */
      last_p = &active_list;
      for (b = active_list; b; b = b->next_x) {
         if (y >= b->y + b->h)
            *last_p = b->next_x;
         else
            last_p = &b->next_x;
      }
   }
}



/* create_system_bitmap:
 *  Attempts to make a system-specific (eg. DirectX surface) bitmap object.
 */
BITMAP *create_system_bitmap(int width, int height)
{
   BITMAP *bmp;

   ASSERT(width >= 0);
   ASSERT(height > 0);
   ASSERT(gfx_driver != NULL);

   if (gfx_driver->create_system_bitmap)
      return gfx_driver->create_system_bitmap(width, height);

   bmp = create_bitmap(width, height);
   bmp->id |= BMP_ID_SYSTEM;

   return bmp;
}



/* destroy_bitmap:
 *  Destroys a memory bitmap.
 */
void destroy_bitmap(BITMAP *bitmap)
{
   VRAM_BITMAP *prev, *pos;

   if (bitmap) {
      if (is_video_bitmap(bitmap)) {
         /* special case for getting rid of video memory bitmaps */
         ASSERT(!_dispsw_status);

         prev = NULL;
         pos = vram_bitmap_list;

         while (pos) {
            if (pos->bmp == bitmap) {
               if (prev)
                  prev->next_y = pos->next_y;
               else
                  vram_bitmap_list = pos->next_y;

               if (pos->x < 0) {
                  /* the driver is in charge of this object */
                  gfx_driver->destroy_video_bitmap(bitmap);
                  _AL_FREE(pos);
                  return;
               }

               /* Update cached bitmap size using worst case scenario:
                * the bitmap lies between two holes whose size is the cached
                * size on each axis respectively.
                */
               failed_bitmap_w = failed_bitmap_w * 2 + ((bitmap->w + 15) & ~15);
               if (failed_bitmap_w > BMP_MAX_SIZE)
                  failed_bitmap_w = BMP_MAX_SIZE;

               failed_bitmap_h = failed_bitmap_h * 2 + bitmap->h;
               if (failed_bitmap_h > BMP_MAX_SIZE)
                  failed_bitmap_h = BMP_MAX_SIZE;

               _AL_FREE(pos);
               break;
            }

            prev = pos;
            pos = pos->next_y;
         }

         _unregister_switch_bitmap(bitmap);
      }
      else if (is_system_bitmap(bitmap)) {
         /* special case for getting rid of system memory bitmaps */
         ASSERT(gfx_driver != NULL);

         if (gfx_driver->destroy_system_bitmap) {
            gfx_driver->destroy_system_bitmap(bitmap);
            return;
         }
      }

      /* normal memory or sub-bitmap destruction */
      if (system_driver->destroy_bitmap) {
         if (system_driver->destroy_bitmap(bitmap))
            return;
      }

      if (bitmap->dat)
         _AL_FREE(bitmap->dat);

      _AL_FREE(bitmap);
   }
}



/* set_clip_rect:
 *  Sets the two opposite corners of the clipping rectangle to be used when
 *  drawing to the bitmap. Nothing will be drawn to positions outside of this
 *  rectangle. When a new bitmap is created the clipping rectangle will be
 *  set to the full area of the bitmap.
 */
void set_clip_rect(BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
   ASSERT(bitmap);

   /* internal clipping is inclusive-exclusive */
   x2++;
   y2++;

   bitmap->cl = CLAMP(0, x1, bitmap->w-1);
   bitmap->ct = CLAMP(0, y1, bitmap->h-1);
   bitmap->cr = CLAMP(0, x2, bitmap->w);
   bitmap->cb = CLAMP(0, y2, bitmap->h);

   if (bitmap->vtable->set_clip)
      bitmap->vtable->set_clip(bitmap);
}



/* add_clip_rect:
 *  Makes the new clipping rectangle the intersection between the given
 *  rectangle and the current one.
 */
void add_clip_rect(BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
   int cx1, cy1, cx2, cy2;

   ASSERT(bitmap);

   get_clip_rect(bitmap, &cx1, &cy1, &cx2, &cy2);

   x1 = MAX(x1, cx1);
   y1 = MAX(y1, cy1);
   x2 = MIN(x2, cx2);
   y2 = MIN(y2, cy2);

   set_clip_rect(bitmap, x1, y1, x2, y2);
}



/* set_clip:
 *  Sets the two opposite corners of the clipping rectangle to be used when
 *  drawing to the bitmap. Nothing will be drawn to positions outside of this
 *  rectangle. When a new bitmap is created the clipping rectangle will be
 *  set to the full area of the bitmap. If x1, y1, x2 and y2 are all zero
 *  clipping will be turned off, which will slightly speed up drawing
 *  operations but will allow memory to be corrupted if you attempt to draw
 *  off the edge of the bitmap.
 */
void set_clip(BITMAP *bitmap, int x1, int y1, int x2, int y2)
{
   int t;

   ASSERT(bitmap);

   if ((!x1) && (!y1) && (!x2) && (!y2)) {
      set_clip_rect(bitmap, 0, 0, bitmap->w-1, bitmap->h-1);
      set_clip_state(bitmap, FALSE);
      return;
   }

   if (x2 < x1) {
      t = x1;
      x1 = x2;
      x2 = t;
   }

   if (y2 < y1) {
      t = y1;
      y1 = y2;
      y2 = t;
   }

   set_clip_rect(bitmap, x1, y1, x2, y2);
   set_clip_state(bitmap, TRUE);
}



/* scroll_screen:
 *  Attempts to scroll the hardware screen, returning 0 on success.
 *  Check the VIRTUAL_W and VIRTUAL_H values to see how far the screen
 *  can be scrolled. Note that a lot of VESA drivers can only handle
 *  horizontal scrolling in four pixel increments.
 */
int scroll_screen(int x, int y)
{
   int ret = 0;
   int h;

   /* can driver handle hardware scrolling? */
   if ((!gfx_driver->scroll) || (_dispsw_status))
      return -1;

   /* clip x */
   if (x < 0) {
      x = 0;
      ret = -1;
   }
   else if (x > (VIRTUAL_W - SCREEN_W)) {
      x = VIRTUAL_W - SCREEN_W;
      ret = -1;
   }

   /* clip y */
   if (y < 0) {
      y = 0;
      ret = -1;
   }
   else {
      h = (_screen_split_position > 0) ? _screen_split_position : SCREEN_H;
      if (y > (VIRTUAL_H - h)) {
         y = VIRTUAL_H - h;
         ret = -1;
      }
   }

   /* scroll! */
   if (gfx_driver->scroll(x, y) != 0)
      ret = -1;

   return ret;
}



/* request_scroll:
 *  Attempts to initiate a triple buffered hardware scroll, which will
 *  take place during the next retrace. Returns 0 on success.
 */
int request_scroll(int x, int y)
{
   int ret = 0;
   int h;

   /* can driver handle triple buffering? */
   if ((!gfx_driver->request_scroll) || (_dispsw_status)) {
      scroll_screen(x, y);
      return -1;
   }

   /* clip x */
   if (x < 0) {
      x = 0;
      ret = -1;
   }
   else if (x > (VIRTUAL_W - SCREEN_W)) {
      x = VIRTUAL_W - SCREEN_W;
      ret = -1;
   }

   /* clip y */
   if (y < 0) {
      y = 0;
      ret = -1;
   }
   else {
      h = (_screen_split_position > 0) ? _screen_split_position : SCREEN_H;
      if (y > (VIRTUAL_H - h)) {
         y = VIRTUAL_H - h;
         ret = -1;
      }
   }

   /* scroll! */
   if (gfx_driver->request_scroll(x, y) != 0)
      ret = -1;

   return ret;
}



/* poll_scroll:
 *  Checks whether a requested triple buffer flip has actually taken place.
 */
int poll_scroll(void)
{
   if ((!gfx_driver->poll_scroll) || (_dispsw_status))
      return FALSE;

   return gfx_driver->poll_scroll();
}



/* show_video_bitmap:
 *  Page flipping function: swaps to display the specified video memory
 *  bitmap object (this must be the same size as the physical screen).
 */
int show_video_bitmap(BITMAP *bitmap)
{
   if ((!is_video_bitmap(bitmap)) ||
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H) ||
       (_dispsw_status))
      return -1;

   if (gfx_driver->show_video_bitmap)
      return gfx_driver->show_video_bitmap(bitmap);

   return scroll_screen(bitmap->x_ofs, bitmap->y_ofs);
}



/* request_video_bitmap:
 *  Triple buffering function: triggers a swap to display the specified
 *  video memory bitmap object, which will take place on the next retrace.
 */
int request_video_bitmap(BITMAP *bitmap)
{
   if ((!is_video_bitmap(bitmap)) ||
       (bitmap->w != SCREEN_W) || (bitmap->h != SCREEN_H) ||
       (_dispsw_status))
      return -1;

   if (gfx_driver->request_video_bitmap)
      return gfx_driver->request_video_bitmap(bitmap);

   return request_scroll(bitmap->x_ofs, bitmap->y_ofs);
}



/* enable_triple_buffer:
 *  Asks a driver to turn on triple buffering mode, if it is capable
 *  of that.
 */
int enable_triple_buffer(void)
{
   if (gfx_capabilities & GFX_CAN_TRIPLE_BUFFER)
      return 0;

   if (_dispsw_status)
      return -1;

   if (gfx_driver->enable_triple_buffer) {
      gfx_driver->enable_triple_buffer();

      if ((gfx_driver->request_scroll) || (gfx_driver->request_video_bitmap)) {
         gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;
         return 0;
      }
   }

   return -1;
}



/* get_gfx_mode_type:
 *  Evaluates the type of the graphics driver card
 *  and tells you whether it is a windowed, fullscreen,
 *  definitely windowed or fullscreen, and/or a magic driver.
 *  See gfx.h for the GFX_TYPE_* defines.
 */
int get_gfx_mode_type(int graphics_card)
{
   int gfx_type = GFX_TYPE_UNKNOWN;
   _DRIVER_INFO *gfx_driver_info;
   GFX_DRIVER *gfx_driver_entry;

   ASSERT(system_driver);

   /* Ask the system driver for a list of graphics hardware drivers */
   if (system_driver->gfx_drivers) {
      gfx_driver_info = system_driver->gfx_drivers();
   }
   else {
      gfx_driver_info = _gfx_driver_list;
   }

   ASSERT(gfx_driver_info);

   while (gfx_driver_info->driver) {
      if (gfx_driver_info->id == graphics_card) {
         gfx_driver_entry = (GFX_DRIVER*)gfx_driver_info->driver;
         if (gfx_driver_entry->windowed) {
            gfx_type |= (GFX_TYPE_WINDOWED   | GFX_TYPE_DEFINITE);
         }
         else {
            gfx_type |= (GFX_TYPE_FULLSCREEN | GFX_TYPE_DEFINITE);
         }
         break;
      }
      gfx_driver_info++;
   }

   switch (graphics_card) {
      case GFX_AUTODETECT:
         gfx_type |= GFX_TYPE_MAGIC;
         break;
      case GFX_AUTODETECT_FULLSCREEN:
         gfx_type |= (GFX_TYPE_MAGIC | GFX_TYPE_FULLSCREEN | GFX_TYPE_DEFINITE);
         break;
      case GFX_AUTODETECT_WINDOWED:
         gfx_type |= (GFX_TYPE_MAGIC | GFX_TYPE_WINDOWED   | GFX_TYPE_DEFINITE);
         break;
      case GFX_SAFE:
         gfx_type |= GFX_TYPE_MAGIC;
         break;
      case GFX_TEXT:
         gfx_type |= GFX_TYPE_MAGIC;
         break;
   }
   return gfx_type;
}



/* get_gfx_mode:
 *  Tells you which graphics mode is currently set.
 *  Useful for determining the actual driver that
 *  GFX_AUTODETECT* or GFX_SAFE set successfully.
 */
int get_gfx_mode(void)
{
   if (!gfx_driver)
      return GFX_NONE;

   return gfx_driver->id;
}
