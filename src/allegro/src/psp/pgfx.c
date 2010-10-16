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
 *      PSP gfx driver.
 *
 *      By diedel.
 *
 *      The psp_draw_to_screen() routine is based on the Basilisk II
 *      PSP refresh routines by J.F.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintpsp.h"
#include <math.h>
#include <pspdisplay.h>
#include <pspgu.h>
#include <psputils.h>

#ifndef ALLEGRO_PSP
   #error something is wrong with the makefile
#endif


#define MAX_SCR_WIDTH                  (480)
#define MAX_SCR_HEIGHT                 (272)
#define MIN_BUF_WIDTH                  (512)
#define PALETTE_PIXEL_SIZE               (4)
#define VIDEO_REFRESH_RATE              (60)
#define DISPLAY_SIZE     (MIN_BUF_WIDTH * MAX_SCR_HEIGHT * PALETTE_PIXEL_SIZE)


/* The width in pixels of the display framebuffer. */
static unsigned int framebuf_width;

/* The PSP pixel format. */
static int gu_psm_format;

/* Pointer to the texture area actually displayed onto the screen.
 * Only for the 8 bpp mode.
 */
static unsigned char *texture_start;

/* Number of vertical black pulses at the start of
 * a triple buffering operation. */
static unsigned int vcount_start = 0;

static unsigned int __attribute__((aligned(16))) list[16384];

/* The current palette in 8 bpp mode. */
static unsigned int __attribute__((aligned(16))) clut[256];


struct TEX_VERTEX
{
   unsigned short u, v;
   short x, y, z;
};


/* Software version of some blitting methods */
static void (*_orig_blit) (BITMAP * source, BITMAP * dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);


static BITMAP *psp_display_init(int w, int h, int v_w, int v_h, int color_depth);
static void gfx_psp_enable_acceleration(GFX_VTABLE *vtable);
static void gfx_psp_enable_triple_buffering(GFX_DRIVER *drv);
static void setup_gu(void);
static int psp_scroll(int x, int y);
static void psp_vsync(void);
static void psp_hw_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height);
static void psp_set_palette(AL_CONST RGB *p, int from, int to, int retracesync);
static int psp_request_scroll(int x, int y);
static int psp_poll_scroll(void);
static BITMAP *psp_create_video_bitmap(int width, int height);
static void psp_destroy_video_bitmap(BITMAP *bitmap);
static int psp_show_video_bitmap(BITMAP *bitmap);
static int psp_request_video_bitmap(BITMAP *bitmap);


GFX_DRIVER gfx_psp =
{
   GFX_PSP,
   empty_string,
   empty_string,
   "PSP gfx driver",
   psp_display_init,             /* AL_METHOD(struct BITMAP *, init, (int w, int h, int v_w, int v_h, int color_depth)); */
   NULL,                         /* AL_METHOD(void, exit, (struct BITMAP *b)); */
   psp_scroll,                   /* AL_METHOD(int, scroll, (int x, int y)); */
   psp_vsync,                    /* AL_METHOD(void, vsync, (void)); */
   psp_set_palette,              /* AL_METHOD(void, set_palette, (AL_CONST struct RGB *p, int from, int to, int retracesync)); */
   NULL,                         /* AL_METHOD(int, request_scroll, (int x, int y)); */
   NULL,                         /* AL_METHOD(int, poll_scroll, (void)); */
   NULL,                         /* AL_METHOD(void, enable_triple_buffer, (void)); */
   psp_create_video_bitmap,      /* AL_METHOD(struct BITMAP *, create_video_bitmap, (int width, int height)); */
   psp_destroy_video_bitmap,     /* AL_METHOD(void, destroy_video_bitmap, (struct BITMAP *bitmap)); */
   psp_show_video_bitmap,        /* AL_METHOD(int, show_video_bitmap, (BITMAP *bitmap)); */
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
   FALSE                         /* true if driver runs windowed */
};




/* psp_display_init:
 *  Initializes the gfx mode.
 */
static BITMAP *psp_display_init(int w, int h, int v_w, int v_h, int color_depth)
{
   GFX_VTABLE *vtable = _get_vtable(color_depth);
   BITMAP *psp_screen;
   uintptr_t vram_start, screen_start;
   int available_vram, framebuf_height, framebuf_size;
   int top_margin, left_margin, bytes_per_line;

   switch (color_depth) {
      case  8:
         gu_psm_format = GU_PSM_T8;
         break;

      case  15:
         gu_psm_format = GU_PSM_5551;
         break;

      case 16:
         gu_psm_format = GU_PSM_5650;
         break;

      case 32:
         gu_psm_format = GU_PSM_8888;
         break;

      default:
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported color depth"));
         return NULL;
   }

   if (w > MAX_SCR_WIDTH || h > MAX_SCR_HEIGHT) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Resolution not supported"));
      return NULL;
   }

   /* Virtual screen management. */
   if (v_w < w)
      v_w = w;
   if (v_h < h)
      v_h = h;

   if ( (gu_psm_format != GU_PSM_T8) && ((v_w > w && w < MAX_SCR_WIDTH) || (v_h > h && h < MAX_SCR_HEIGHT)) ) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Virtual screen not valid"));
      return NULL;
   }

   /* How many VRAM we need? */
   available_vram = sceGeEdramGetSize();
   vram_start =  (uintptr_t)sceGeEdramGetAddr();
   if (gu_psm_format == GU_PSM_T8) {
      /* To support the 8 bpp mode we have to set the framebuffer as
       * a palettized texture outside the display area.
       */
      available_vram -= DISPLAY_SIZE;
      vram_start += DISPLAY_SIZE;
      screen_start = vram_start;
      texture_start = (unsigned char *)screen_start;
      framebuf_width = ALIGN_TO(v_w, 16);
      framebuf_height = v_h;
   }
   else {
      framebuf_width = ALIGN_TO(v_w ,64);
      framebuf_width = MAX(MIN_BUF_WIDTH, framebuf_width);
      framebuf_height = MAX(MAX_SCR_HEIGHT, v_h);

      /* Center the screen. */
      top_margin = (MAX_SCR_HEIGHT - h) / 2;
      left_margin = (MAX_SCR_WIDTH - w) / 2;
      screen_start = vram_start + (top_margin * framebuf_width + left_margin) * BYTES_PER_PIXEL(color_depth);
      screen_start |= 0x40000000;
   }
   bytes_per_line = framebuf_width * BYTES_PER_PIXEL(color_depth);
   framebuf_size = framebuf_height * bytes_per_line;
   if ( framebuf_size > available_vram) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Virtual screen size too large"));
      return NULL;
   }

   /* Create the Allegro screen bitmap. */
   psp_screen = _make_bitmap(v_w, v_h, screen_start, &gfx_psp, color_depth, bytes_per_line);
   if (psp_screen)
      psp_screen->extra = _AL_MALLOC(sizeof(struct BMP_EXTRA_INFO));
   if (!psp_screen || !psp_screen->extra) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return NULL;
   }

   /* Used in the scrolling routines. */
   BMP_EXTRA(psp_screen)->hw_addr = vram_start;

   available_vram -= framebuf_size;
   vram_start += framebuf_size;

   BMP_EXTRA(psp_screen)->pitch = framebuf_width;

   /* Initialize the PSP video. */
   setup_gu();

   _set_current_refresh_rate(VIDEO_REFRESH_RATE);

   /* Initialize the video memory manager. */
   vmm_init(vram_start, available_vram);

   /* physical (not virtual!) screen size */
   gfx_psp.w = psp_screen->cr = w;
   gfx_psp.h = psp_screen->cb = h;

   if (gu_psm_format == GU_PSM_T8) {
      /* Special routines to manage the 8 bpp color depth. */
      vtable->do_stretch_blit = psp_do_stretch_blit8;
      memcpy(&_screen_vtable, &psp_vtable8, sizeof(GFX_VTABLE));
   }
   else {
      /* Only supported in truecolor modes. */
      gfx_psp_enable_acceleration(vtable);
      gfx_psp_enable_triple_buffering(&gfx_psp);
   }

   displayed_video_bitmap = psp_screen;

   return psp_screen;
}



/* gfx_psp_enable_acceleration:
 *  Installs and activates some routines to accelerate Allegro.
 */
static void gfx_psp_enable_acceleration(GFX_VTABLE *vtable)
{
   /* Keep the original blitting methods */
   _orig_blit = vtable->blit_to_self;

   /* Accelerated blits. */
   vtable->blit_from_memory = psp_hw_blit;
   vtable->blit_to_memory = psp_hw_blit;
   vtable->blit_from_system = psp_hw_blit;
   vtable->blit_to_system = psp_hw_blit;
   vtable->blit_to_self = psp_hw_blit;

   _screen_vtable.blit_from_memory = psp_hw_blit;
   _screen_vtable.blit_to_memory = psp_hw_blit;
   _screen_vtable.blit_from_system = psp_hw_blit;
   _screen_vtable.blit_to_system = psp_hw_blit;
   _screen_vtable.blit_to_self = psp_hw_blit;

   /* Supporting routines. */
   system_driver->create_bitmap = psp_create_bitmap;
   system_driver->destroy_bitmap = psp_destroy_bitmap;

   gfx_capabilities |= (GFX_HW_VRAM_BLIT | GFX_HW_MEM_BLIT);
}



/* gfx_psp_enable_triple_buffering:
 *  Installs the triple buffering capability.
 */
static void gfx_psp_enable_triple_buffering(GFX_DRIVER *drv)
{
   drv->request_scroll = psp_request_scroll;
   drv->poll_scroll = psp_poll_scroll;
   drv->request_video_bitmap = psp_request_video_bitmap;

   gfx_capabilities |= GFX_CAN_TRIPLE_BUFFER;

}



/* setup_gu:
 *  Initializes the PSP graphics hardware.
 */
static void setup_gu(void)
{
   sceGuInit();
   sceGuStart(GU_DIRECT, list);
   if (gu_psm_format == GU_PSM_T8) {
      sceGuDrawBuffer(GU_PSM_8888, 0, MIN_BUF_WIDTH);
      sceGuDispBuffer(MAX_SCR_WIDTH, MAX_SCR_HEIGHT, 0, MIN_BUF_WIDTH);
   }
   else {
      sceGuDrawBuffer(gu_psm_format, 0, framebuf_width);
      sceGuDispBuffer(MAX_SCR_WIDTH, MAX_SCR_HEIGHT, 0, framebuf_width);
   }
   sceGuScissor(0, 0, MAX_SCR_WIDTH, MAX_SCR_HEIGHT);
   sceGuEnable(GU_SCISSOR_TEST);
   sceGuEnable(GU_TEXTURE_2D);
   sceGuClear(GU_COLOR_BUFFER_BIT);
   sceGuFinish();
   sceGuSync(0,0);
   sceDisplayWaitVblankStart();
   sceGuDisplay(GU_TRUE);
}



/* psp_scroll:
 *  PSP scrolling routine.
 *  This handle horizontal scrolling in 8 pixel increments under 15 or 16
 *  bpp modes and 4 pixel increments under 32 bpp mode.
 *  The 8 bpp mode has no restrictions.
 */
static int psp_scroll(int x, int y)
{
   uintptr_t new_addr;

   if (_wait_for_vsync)
      sceDisplayWaitVblankStart();

   if (gu_psm_format == GU_PSM_T8) {
      /* We position the texture data pointer at the requested coordinates
       * and the screen display is updated.
       */
      displayed_video_bitmap = screen;
      texture_start = screen->line[y] + x;
      psp_draw_to_screen();
   }
   else {
      /* Truecolor pixel formats. */
      new_addr =  BMP_EXTRA(screen)->hw_addr + (y * framebuf_width + x) * BYTES_PER_PIXEL(bitmap_color_depth(screen));
      sceDisplaySetFrameBuf((void *)new_addr, framebuf_width, gu_psm_format, PSP_DISPLAY_SETBUF_IMMEDIATE);
   }

   return 0;
}



/* psp_vsync:
 *  Waits for a retrace.
 */
void psp_vsync(void)
{
   sceDisplayWaitVblankStart();
}



/* psp_hw_blit:
 *  PSP accelerated blitting routine.
 */
static void psp_hw_blit(BITMAP *source, BITMAP *dest, int source_x, int source_y, int dest_x, int dest_y, int width, int height)
{
   void *source_ptr, *dest_ptr;

   /* sceGuCopyImage() can transfer blocks of size <=1024x1024.
    * Also the pitch of the source and destination buffers is limited
    * to 2048 pixels.
    */
   if (width > 1024 || height > 1024 || BMP_EXTRA(source)->pitch > 2048 || BMP_EXTRA(dest)->pitch > 2048)
      return _orig_blit(source, dest, source_x, source_y, dest_x, dest_y, width, height);

   /* We position the pixel data pointers at the requested coordinates
    * and we align it to 16 bytes. The x,y coordinates are modified accordly.
    */
   source_ptr = source->line[source_y] + source_x * BYTES_PER_PIXEL(bitmap_color_depth(source));
   source_x = ((unsigned int)source_ptr & 0xF) / BYTES_PER_PIXEL(bitmap_color_depth(source));
   source_y = 0;
   source_ptr = (void *)((unsigned int)source_ptr & ~0xF);

   dest_ptr = dest->line[dest_y] + dest_x * BYTES_PER_PIXEL(bitmap_color_depth(dest));
   dest_x = ((unsigned int)dest_ptr & 0xF) / BYTES_PER_PIXEL(bitmap_color_depth(dest));
   dest_y = 0;
   dest_ptr = (void *)((unsigned int)dest_ptr & ~0xF);

   /* The interesting part. */
   sceKernelDcacheWritebackAll();
   sceGuStart(GU_DIRECT,list);
   sceGuCopyImage(gu_psm_format, source_x, source_y, width, height, BMP_EXTRA(source)->pitch, source_ptr, dest_x, dest_y, BMP_EXTRA(dest)->pitch, dest_ptr);
   sceGuFinish();
   sceGuSync(0,0);
}



/* psp_draw_to_screen:
 *  ONLY in 8 bpp video mode.
 *  Draws the texture representing the screen to the PSP display
 *  maximizing the use of the texture-cache.
 */
void psp_draw_to_screen()
{
   unsigned short texture_x;
   unsigned char *texture_ptr;
   short dest_x, dest_y;
   int slice_w = 64;
   int num_slices = ceil(SCREEN_W/(float)(slice_w));
   int i;
   struct TEX_VERTEX *vertices;

   /* We align the texture data pointer to 16 bytes.
    * The texture u coordinate is modified accordly.
    */
   texture_x = (unsigned int)texture_start & 0xF;
   texture_ptr = (unsigned char *)((unsigned int)texture_start & ~0xF);

   /* Blit the texture at the center of the screen. */
   dest_x = (MAX_SCR_WIDTH - SCREEN_W) / 2;
   dest_y = (MAX_SCR_HEIGHT - SCREEN_H) / 2;

   sceKernelDcacheWritebackAll();

   sceGuStart(GU_DIRECT,list);
   sceGuTexMode(GU_PSM_T8,0,0,0);
   sceGuTexImage(0, 512, 512, framebuf_width, texture_ptr);
   sceGuTexFunc(GU_TFX_REPLACE, GU_TCC_RGB);
   sceGuTexFilter(GU_LINEAR, GU_LINEAR);

   for (i=0; i < num_slices; i++) {
      vertices = (struct TEX_VERTEX*)sceGuGetMemory(2 * sizeof(struct TEX_VERTEX));

      vertices[0].u = texture_x + i * slice_w;
      vertices[0].v = 0;
      vertices[1].u = vertices[0].u + slice_w;
      if (vertices[1].u > texture_x + SCREEN_W)
         vertices[1].u = texture_x + SCREEN_W;
      vertices[1].v = vertices[0].v + SCREEN_H;

      vertices[0].x =  dest_x + i * slice_w;
      vertices[0].y = dest_y;
      vertices[0].z = 0;
      vertices[1].x = vertices[0].x + slice_w;
      if (vertices[1].x > dest_x + SCREEN_W)
         vertices[1].x = dest_x + SCREEN_W;
      vertices[1].y = vertices[0].y + SCREEN_H;
      vertices[1].z = 0;

      sceGuDrawArray(GU_SPRITES,GU_TEXTURE_16BIT|GU_VERTEX_16BIT|GU_TRANSFORM_2D, 2,0,vertices);
   }

   sceGuFinish();
   sceGuSync(0,0);
}



/* psp_set_palette:
 *  Sets the hardware palette for the 8 bpp video mode.
 */
static void psp_set_palette(AL_CONST RGB *p, int from, int to, int retracesync)
{
   int c;

   /* Update the palette. */
   for (c=from; c<=to; c++) {
      clut[c] = makecol32(_rgb_scale_6[p[c].r], _rgb_scale_6[p[c].g], _rgb_scale_6[p[c].b]);
   }
   sceKernelDcacheWritebackAll();

   if (retracesync)
      sceDisplayWaitVblankStart();

   sceGuStart(GU_DIRECT,list);
   sceGuClutMode(GU_PSM_8888,0,0xff,0);   /* 32-bit palette */
   sceGuClutLoad((256/8),clut);           /* upload 32 blocks of 8 entries (256) */
   sceGuFinish();
   sceGuSync(0,0);

   psp_draw_to_screen();
}



/* psp_request_scroll:
 *  Attempts to initiate a triple buffered hardware scroll, which will
 *  take place during the next retrace. Returns 0 on success.
 */ 
static int psp_request_scroll(int x, int y)
{
   uintptr_t new_addr;
   
   new_addr =  BMP_EXTRA(screen)->hw_addr + (y * framebuf_width + x) * BYTES_PER_PIXEL(bitmap_color_depth(screen));

   vcount_start = sceDisplayGetVcount();
   sceDisplaySetFrameBuf((void *)new_addr, framebuf_width, gu_psm_format, PSP_DISPLAY_SETBUF_NEXTFRAME);

   return 0;
}



/* psp_poll_scroll:
 *  This function is used for triple buffering. It checks the status of
 *  a hardware scroll previously initiated by the request_scroll() or by
 *  the request_video_bitmap() routines. 
 */
static int psp_poll_scroll(void)
{
   int ret;

   /* A new vertical blank pulse has arrived? */
   if (sceDisplayGetVcount() - vcount_start > 0) {
      vcount_start = 0;
      ret = 0;
   }
   else
      ret = -1;

   return ret;
}



/* psp_create_video_bitmap:
 *  Attempts to make a bitmap object for accessing offscreen video memory.
 */
static BITMAP *psp_create_video_bitmap(int width, int height)
{
   uintptr_t vram_addr, bitmap_offset = 0;
   GFX_VTABLE *vtable;
   BITMAP *bitmap;
   int pitch, used_height = height;
   int i, size, top_margin, left_margin;

   if (_color_depth == 8) {
      if (width >= SCREEN_W && height >= SCREEN_H)
         /* This video bitmap or the derived sub bitmaps can be used for
          * page flipping. The pitch is calculated so that sceGuTexImage()
          * works correctly in that case.
          */
         pitch = ALIGN_TO(width, 16);
      else
         /* A regular 8 bpp video bitmap. */
         pitch = width;
   }
   else {
      if (width == SCREEN_W && height == SCREEN_H) {
         /* Special code for video bitmaps created under truecolor modes that
          * can be used with the show_video_bitmap() and request_video_bitmap()
          * functions for page flipping and triple buffering.
          */
         pitch = MIN_BUF_WIDTH;
         used_height = MAX_SCR_HEIGHT;

         /* Center the bitmap. */
         top_margin = (MAX_SCR_HEIGHT - height) / 2;
         left_margin = (MAX_SCR_WIDTH - width) / 2;
         bitmap_offset = (top_margin * MIN_BUF_WIDTH + left_margin) * BYTES_PER_PIXEL(_color_depth);
      }
      else
         /* A regular truecolor video bitmap.
          * Its width must be multiple of 8 pixels in order to 
          * blit properly using sceGuCopyImage().
          */
         pitch = ALIGN_TO(width, 8);
   }

   /* Allocate video memory for the pixel data. */
   size = pitch * used_height * BYTES_PER_PIXEL(_color_depth);
   vram_addr = vmm_alloc_mem(size);
   if (!vram_addr)
      return NULL;

   /* Clean it. */
   for (i=0; i<size ;i++)
      *((unsigned char *)(vram_addr+i)) = 0;

   /* Create the Allegro bitmap. */
   if (_color_depth == 8)
      vtable = &psp_vtable8;
   else
      vtable = _get_vtable(_color_depth);
   bitmap = _AL_MALLOC(sizeof(BITMAP) + (sizeof(char *) * height));
   if (!vtable || !bitmap) {
      vmm_free_mem(vram_addr, size);
      return NULL;
   }
   bitmap->w = bitmap->cr = width;
   bitmap->h = bitmap->cb = height;
   bitmap->clip = TRUE;
   bitmap->cl = bitmap->ct = 0;
   bitmap->vtable = vtable;
   bitmap->write_bank = bitmap->read_bank = _stub_bank_switch;
   bitmap->id = BMP_ID_VIDEO;
   bitmap->x_ofs = 0;
   bitmap->y_ofs = 0;
   bitmap->seg = _video_ds();

   if (height > 0) {
      bitmap->line[0] = (unsigned char *)(vram_addr + bitmap_offset);
      for (i=1; i<height; i++)
         bitmap->line[i] = bitmap->line[i-1] + pitch * BYTES_PER_PIXEL(_color_depth);
   }

   /* Setup info structure to store additional information. */
   bitmap->extra = _AL_MALLOC(sizeof(struct BMP_EXTRA_INFO));
   if (!bitmap->extra) {
      vmm_free_mem(vram_addr, size);
      _AL_FREE(bitmap);
      return NULL;
   }
   BMP_EXTRA(bitmap)->pitch = pitch;
   BMP_EXTRA(bitmap)->size = size;
   BMP_EXTRA(bitmap)->hw_addr = vram_addr;

   return bitmap;
}



/* psp_destroy_video_bitmap:
 *  Restores the video ram used for the video bitmap and the system ram
 *  used for the bitmap management structures.
 */
static void psp_destroy_video_bitmap(BITMAP *bitmap)
{
   if (!is_sub_bitmap(bitmap))
      vmm_free_mem(BMP_EXTRA(bitmap)->hw_addr, BMP_EXTRA(bitmap)->size);

   _AL_FREE(bitmap->extra);
   _AL_FREE(bitmap);
}



/* psp_show_video_bitmap:
 *  Page flipping function: swaps to display the specified video memory 
 *  bitmap object (this must be the same size as the physical screen).
 */
int psp_show_video_bitmap(BITMAP *bitmap)
{
   if (_wait_for_vsync)
      sceDisplayWaitVblankStart();

   if (gu_psm_format == GU_PSM_T8) {
      /* We position the texture data pointer at the new bitmap
       * and the screen display is updated.
       */
      displayed_video_bitmap = bitmap;
      texture_start = bitmap->line[0];
      psp_draw_to_screen();
   }
   else {
      /* Truecolor pixel formats. */
      
      /* No sub bitmaps are allowed. */
      if (is_sub_bitmap(bitmap))
         return -1;

      sceDisplaySetFrameBuf((void *)BMP_EXTRA(bitmap)->hw_addr, BMP_EXTRA(bitmap)->pitch, gu_psm_format, PSP_DISPLAY_SETBUF_IMMEDIATE);
   }

   return 0;
}



/* psp_request_video_bitmap:
 *  Triple buffering function: triggers a swap to display the specified 
 *  video memory bitmap object, which will take place on the next retrace.
 */
static int psp_request_video_bitmap(BITMAP *bitmap)
{
   /* No sub bitmaps are allowed. */
   if (is_sub_bitmap(bitmap))
      return -1;

   vcount_start = sceDisplayGetVcount();
   sceDisplaySetFrameBuf((void *)BMP_EXTRA(bitmap)->hw_addr, BMP_EXTRA(bitmap)->pitch, gu_psm_format, PSP_DISPLAY_SETBUF_NEXTFRAME);

   return 0;
}
