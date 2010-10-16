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
 *      Helpers for accessing the VGA hardware registers.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifdef ALLEGRO_GFX_HAS_VGA

#include "allegro/internal/aintern.h"
#include "allegro/internal/aintvga.h"



int _crtc = 0x3D4;



/* _vga_regs_init:
 *  Initialiases the VGA register access functions.
 */
void _vga_regs_init(void)
{
   LOCK_VARIABLE(_crtc);

   if (inportb(0x3CC) & 1) 
      _crtc = 0x3D4;
   else 
      _crtc = 0x3B4;
}



/* _vga_vsync:
 *  Waits for the start of a vertical retrace.
 */
void _vga_vsync(void)
{
   _vsync_out_v();
   _vsync_in();
}



/* _set_vga_virtual_width:
 *  Used by various graphics drivers to adjust the virtual width of
 *  the screen, using VGA register 0x3D4 index 0x13.
 */
void _set_vga_virtual_width(int old_width, int new_width)
{
   int width;

   if (old_width != new_width) {
      width = _read_vga_register(_crtc, 0x13);
      _write_vga_register(_crtc, 0x13, (width * new_width) / old_width);
   }
}



/* _vga_set_palette_range:
 *  Sets part of the VGA palette.
 */
void _vga_set_palette_range(AL_CONST PALETTE p, int from, int to, int vsync)
{
   int i;

   if (vsync)
      _vga_vsync();

   outportb(0x3C8, from);

   for (i=from; i<=to; i++) {
      outportb(0x3C9, p[i].r);
      outportb(0x3C9, p[i].g);
      outportb(0x3C9, p[i].b);
   }
}


#endif /* ALLEGRO_GFX_HAS_VGA */
