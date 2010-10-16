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


#ifndef AINTVGA_H
#define AINTVGA_H

#ifndef ALLEGRO_H
   #error must include allegro.h first
#endif

#ifdef ALLEGRO_GFX_HAS_VGA

#ifdef __cplusplus
   extern "C" {
#endif


AL_VAR(int, _crtc);

AL_FUNC(void, _vga_regs_init, (void));
AL_FUNC(void, _vga_vsync, (void));
AL_FUNC(void, _vga_set_palette_range, (AL_CONST PALETTE p, int from, int to, int do_sync));
AL_FUNC(void, _set_vga_virtual_width, (int old_width, int new_width));
AL_FUNC(uintptr_t, _set_vga_mode, (int modenum));
AL_FUNC(void, _unset_vga_mode, (void));
AL_FUNC(void, _save_vga_mode, (void));
AL_FUNC(void, _restore_vga_mode, (void));


/* reads the current value of a VGA hardware register */
AL_INLINE(int, _read_vga_register, (int port, int idx),
{
   if (port==0x3C0)
      inportb(_crtc+6); 

   outportb(port, idx);
   return inportb(port+1);
})


/* writes to a VGA hardware register */
AL_INLINE(void, _write_vga_register, (int port, int idx, int v),
{
   if (port==0x3C0) {
      inportb(_crtc+6);
      outportb(port, idx);
      outportb(port, v);
   }
   else {
      outportb(port, idx);
      outportb(port+1, v);
   }
})


/* alters specific bits of a VGA hardware register */
AL_INLINE(void, _alter_vga_register, (int port, int idx, int mask, int v),
{
   int temp;
   temp = _read_vga_register(port, idx);
   temp &= (~mask);
   temp |= (v & mask);
   _write_vga_register(port, idx, temp);
})


/* waits until the VGA isn't in a horizontal blank */
AL_INLINE(void, _vsync_out_h, (void),
{
   do {
   } while (inportb(0x3DA) & 1);
})


/* waits until the VGA isn't in a vertical blank */
AL_INLINE(void, _vsync_out_v, (void),
{
   do {
   } while (inportb(0x3DA) & 8);
})


/* waits until the VGA is in a vertical blank */
AL_INLINE(void, _vsync_in, (void),
{
   if (_timer_use_retrace) {
      int t = retrace_count; 

      do {
      } while (t == retrace_count);
   }
   else {
      do {
      } while (!(inportb(0x3DA) & 8));
   }
})


/* modifies the VGA pelpan register */
AL_INLINE(void, _write_hpp, (int value),
{
   if (_timer_use_retrace) {
      _retrace_hpp_value = value;

      do {
      } while (_retrace_hpp_value == value);
   }
   else {
      do {
      } while (!(inportb(0x3DA) & 8));

      _write_vga_register(0x3C0, 0x33, value);
   }
})



#ifdef __cplusplus
   }
#endif

#endif          /* ifdef ALLEGRO_GFX_HAS_VGA */

#endif          /* ifndef AINTVGA_H */
