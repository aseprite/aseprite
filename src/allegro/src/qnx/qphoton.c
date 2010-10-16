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
 *      QNX Photon helper routines.
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


/* Photon globals */
int ph_gfx_mode = PH_GFX_NONE;
PgVideoModeInfo_t ph_gfx_mode_info;
PgColor_t ph_palette[256];



/* setup_direct_shifts
 *  Setups direct color shifts.
 */
void setup_direct_shifts(void)
{
   _rgb_r_shift_15 = 10;
   _rgb_g_shift_15 = 5;
   _rgb_b_shift_15 = 0;

   _rgb_r_shift_16 = 11;
   _rgb_g_shift_16 = 5;
   _rgb_b_shift_16 = 0;

   _rgb_r_shift_24 = 16;
   _rgb_g_shift_24 = 8;
   _rgb_b_shift_24 = 0;

   _rgb_a_shift_32 = 24;
   _rgb_r_shift_32 = 16; 
   _rgb_g_shift_32 = 8; 
   _rgb_b_shift_32 = 0;

   /* set the source color key */   
   PgSetChroma(MASK_COLOR_32, Pg_CHROMA_SRC_MATCH | Pg_CHROMA_NODRAW);
}



/* setup_driver:
 *  Helper function for initializing the gfx driver.
 */
void setup_driver(GFX_DRIVER *drv, int w, int h, int color_depth)
{
   PgHWCaps_t caps;

   /* setup the driver structure */
   drv->w = w;
   drv->h = h;
   drv->linear = TRUE;

   if (PgGetGraphicsHWCaps(&caps))
      drv->vid_mem = w * h * BYTES_PER_PIXEL(color_depth);
   else
      drv->vid_mem = caps.total_video_ram;

   /* modify the vtable to work with video memory */
   memcpy(&_screen_vtable, _get_vtable(color_depth), sizeof(GFX_VTABLE));

   _screen_vtable.acquire = ph_acquire;
   _screen_vtable.release = ph_release;

#ifdef ALLEGRO_NO_ASM
   _screen_vtable.unwrite_bank = ph_unwrite_line;
#else
   _screen_vtable.unwrite_bank = ph_unwrite_line_asm;
#endif

   _screen_vtable.created_sub_bitmap = qnx_ph_created_sub_bitmap;
}

