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
 *      Video driver for VGA mode 13h (320x200x256).
 *
 *      By Shawn Hargreaves.
 *
 *      320x100 mode contributed by Salvador Eduardo Tropea.
 *
 *      80x80 mode contributed by Pedro Cardoso.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"

#ifdef ALLEGRO_GFX_HAS_VGA

#include "allegro/internal/aintern.h"
#include "allegro/internal/aintvga.h"

#ifdef ALLEGRO_INTERNAL_HEADER
   #include ALLEGRO_INTERNAL_HEADER
#endif

#if (!defined ALLEGRO_LINUX) || ((defined ALLEGRO_LINUX_VGA) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE)))



static BITMAP *vga_init(int w, int h, int v_w, int v_h, int color_depth);
static void vga_exit(BITMAP *b);
static int vga_scroll(int x, int y);
static GFX_MODE_LIST *vga_fetch_mode_list(void);


GFX_DRIVER gfx_vga = 
{
   GFX_VGA,
   empty_string,
   empty_string,
   "Standard VGA", 
   vga_init,
   vga_exit,
   NULL,
   _vga_vsync,
   _vga_set_palette_range,
   NULL, NULL, NULL,             /* no triple buffering */
   NULL, NULL, NULL, NULL,       /* no video bitmaps */
   NULL, NULL,                   /* no system bitmaps */
   NULL, NULL, NULL, NULL,       /* no hardware cursor */
   NULL,                         /* no drawing mode hook */
   _save_vga_mode,
   _restore_vga_mode,
   NULL,                         /* AL_METHOD(void, set_blender_mode, (int mode, int r, int g, int b, int a)); */
   vga_fetch_mode_list,
   320, 200,
   TRUE,
   0, 0,
   0x10000,
   0,
   FALSE
};



static GFX_MODE vga_gfx_modes[] = {
   { 80,  80,  8 },
   { 160, 120, 8 },
   { 256, 256, 8 },
   { 320, 100, 8 },
   { 320, 200, 8 },
   { 0,   0,   0 }
};



/* vga_init:
 *  Selects mode 13h and creates a screen bitmap for it.
 */
static BITMAP *vga_init(int w, int h, int v_w, int v_h, int color_depth)
{
   unsigned long addr;
   BITMAP *b;
   int c;

   /* check it is a valid resolution */
   if (color_depth != 8) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("VGA only supports 8 bit color"));
      return NULL;
   }

   /* for setting GFX_SAFE */
   if ((!w) && (!h)) {
      w = 320;
      h = 200;
   }

   if ((w == 320) && (h == 200) && (v_w <= 320) && (v_h <= 200)) {
      /* standard mode 13h */
      addr = _set_vga_mode(0x13);
      if (!addr)
	 return NULL;

      b = _make_bitmap(320, 200, addr, &gfx_vga, 8, 320);
      if (!b)
	 return NULL;

      gfx_vga.scroll = NULL;
      gfx_vga.w = 320;
      gfx_vga.h = 200;
   }
   else if ((w == 320) && (h == 100) && (v_w <= 320) && (v_h <= 200)) {
      /* tweaked 320x100 mode */
      addr = _set_vga_mode(0x13);
      if (!addr)
	 return NULL;

      b = _make_bitmap(320, 200, addr, &gfx_vga, 8, 320);
      if (!b)
	 return NULL;

      outportb(0x3D4, 9);
      outportb(0x3D5, inportb(0x3D5) | 0x80);

      gfx_vga.scroll = vga_scroll;
      gfx_vga.w = 320;
      gfx_vga.h = 100;
   }
   else if ((w == 256) && (h == 256) && (v_w <= 256) && (v_h <= 256)) {
      /* tweaked 256x256 mode */
      addr = _set_vga_mode(0x13);
      if (!addr)
	 return NULL;

      b = _make_bitmap(256, 256, addr, &gfx_vga, 8, 256);
      if (!b)
	 return NULL;

      outportb(0x3D4, 0x11);
      c = inportb(0x3D5) & 0x7F;
      outportb(0x3D4, (c << 8) | 0x11);
      outportb(0x3D5, c);

      outportb(0x3C2, 0xE3);
      outportw(0x3D4, 0x5F00);
      outportw(0x3D4, 0x3F01);
      outportw(0x3D4, 0x4002);
      outportw(0x3D4, 0x8203);
      outportw(0x3D4, 0x4A04);
      outportw(0x3D4, 0x9A05);
      outportw(0x3D4, 0x2306);
      outportw(0x3D4, 0xB207);
      outportw(0x3D4, 0x0008);
      outportw(0x3D4, 0x6109);
      outportw(0x3D4, 0x0A10);
      outportw(0x3D4, 0xAC11);
      outportw(0x3D4, 0xFF12);
      outportw(0x3D4, 0x2013);
      outportw(0x3D4, 0x4014);
      outportw(0x3D4, 0x0715);
      outportw(0x3D4, 0x1A16);
      outportw(0x3D4, 0xA317);
      outportw(0x3C4, 0x0101);
      outportw(0x3C4, 0x0E04);
      outportw(0x3CE, 0x4005);
      outportw(0x3CE, 0x0506);

      inportb(0x3DA);
      outportb(0x3C0, 0x30);
      outportb(0x3C0, 0x41);

      inportb(0x3DA);
      outportb(0x3C0, 0x33);
      outportb(0x3C0, 0x00);

      outportb(0x3C6, 0xFF);

      gfx_vga.scroll = NULL;
      gfx_vga.w = 256;
      gfx_vga.h = 256;
   }
   else if ((w == 160) && (h == 120) && (v_w <= 160) && (v_h <= 409)) {
      /* tweaked 160x120 mode */
      addr = _set_vga_mode(0x0D);
      if (!addr)
	 return NULL;

      b = _make_bitmap(160, 409, addr, &gfx_vga, 8, 160);
      if (!b)
	 return NULL;

      outportb(0x3D4, 0x11);
      outportb(0x3D5, inportb(0x3D5)&0x7F);
      outportb(0x3D4, 0x04);
      outportb(0x3D5, inportb(0x3D5)+1);
      outportb(0x3D4, 0x11);
      outportb(0x3D5, inportb(0x3D5)|0x80);

      outportb(0x3C2, (inportb(0x3CC)&~0xC0)|0x80);

      outportb(0x3D4, 0x11);
      outportb(0x3D5, inportb(0x3D5)&0x7F);

      outportb(0x3D4, 0x06);
      outportb(0x3D5, 0x0B);

      outportb(0x3D4, 0x07);
      outportb(0x3D5, 0x3E);

      outportb(0x3D4, 0x10);
      outportb(0x3D5, 0xEA);

      outportb(0x3D4, 0x11);
      outportb(0x3D5, 0x8C);

      outportb(0x3D4, 0x12);
      outportb(0x3D5, 0xDF);

      outportb(0x3D4, 0x15);
      outportb(0x3D5, 0xE7);

      outportb(0x3D4, 0x16);
      outportb(0x3D5, 0x04);

      outportb(0x3D4, 0x11);
      outportb(0x3D5, inportb(0x3D5)|0x80);

      outportb(0x3CE, 0x05);
      outportb(0x3CF, (inportb(0x3CF)&0x60)|0x40);

      inportb(0x3DA);
      outportb(0x3C0, 0x30);
      outportb(0x3C0, inportb(0x3C1)|0x40);

      for (c=0; c<16; c++) {
	 outportb(0x3C0, c);
	 outportb(0x3C0, c);
      }
      outportb(0x3C0, 0x20);

      outportb(0x3C8, 0x00);

      outportb(0x3C4, 0x04);
      outportb(0x3C5, (inportb(0x3C5)&0xF7)|8);
      outportb(0x3D4, 0x14);
      outportb(0x3D5, (inportb(0x3D5)&~0x40)|64);
      outportb(0x3D4, 0x017);
      outportb(0x3D5, (inportb(0x3D5)&~0x40)|64);

      outportb(0x3D4, 0x09);
      outportb(0x3D5, (inportb(0x3D5)&0x60)|3);

      gfx_vga.scroll = vga_scroll;
      gfx_vga.w = 160;
      gfx_vga.h = 120;
   }
   else if ((w == 80) && (h == 80) && (v_w <= 80) && (v_h <= 819)) {
      /* tweaked 80x80 mode */
      addr = _set_vga_mode(0x13);
      if (!addr)
	 return NULL;

      b = _make_bitmap(80, 819, addr, &gfx_vga, 8, 80);
      if (!b)
	 return NULL;

      outportw(0x3C4, 0x0604);
      outportw(0x3C4, 0x0F02);

      outportw(0x3D4, 0x0014);
      outportw(0x3D4, 0xE317);
      outportw(0x3D4, 0xE317);
      outportw(0x3D4, 0x0409);

      gfx_vga.scroll = vga_scroll;
      gfx_vga.w = 80;
      gfx_vga.h = 80;
   }
   else {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not a valid VGA resolution"));
      return NULL;
   }

   #ifdef ALLEGRO_LINUX

      b->vtable->acquire = __al_linux_acquire_bitmap;
      b->vtable->release = __al_linux_release_bitmap;

   #endif

   return b;
}



/* vga_exit:
 *  Unsets the video mode.
 */
static void vga_exit(BITMAP *b)
{
   _unset_vga_mode();
}



/* vga_scroll:
 *  Hardware scrolling routine for standard VGA modes.
 */
static int vga_scroll(int x, int y)
{
   long a = (x + (y * VIRTUAL_W));

   if (gfx_vga.w > 80)
      a /= 4;

   DISABLE();

   if (_wait_for_vsync)
      _vsync_out_h();

   /* write to VGA address registers */
   _write_vga_register(_crtc, 0x0D, a & 0xFF);
   _write_vga_register(_crtc, 0x0C, (a>>8) & 0xFF);

   ENABLE();

   if (_wait_for_vsync)
      _vsync_in();

   return 0;
}



/* vga_fetch_mode_list:
 *  Creates a list of of currently implemented VGA modes.
 */
static GFX_MODE_LIST *vga_fetch_mode_list(void)
{
   GFX_MODE_LIST *mode_list;

   mode_list = _AL_MALLOC(sizeof(GFX_MODE_LIST));
   if (!mode_list) return NULL;
   
   mode_list->mode = _AL_MALLOC(sizeof(vga_gfx_modes));
   if (!mode_list->mode) return NULL;
   
   memcpy(mode_list->mode, vga_gfx_modes, sizeof(vga_gfx_modes));
   mode_list->num_modes = sizeof(vga_gfx_modes) / sizeof(GFX_MODE) - 1;

   return mode_list;
}



#ifdef ALLEGRO_MODULE

extern void _module_init_modex(int);  /* from modex.c */

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   _module_init_modex(system_driver);
   
   if (system_driver == SYSTEM_LINUX)
      _unix_register_gfx_driver(GFX_VGA, &gfx_vga, TRUE, TRUE);
}

#endif      /* ifdef ALLEGRO_MODULE */



#endif      /* (!defined ALLEGRO_LINUX) || ((defined ALLEGRO_LINUX_VGA) && ...) */
#endif      /* ifdef ALLEGRO_GFX_HAS_VGA */
