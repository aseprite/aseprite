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
 *      QNX asm line switcher routines.
 *
 *      By Angelo Mottola.
 *
 *      Window updating system derived from Win32 version by Eric Botcazou.
 *
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"

.text


#ifndef ALLEGRO_NO_ASM

/* ph_write_line_asm:
 *  edx = bitmap
 *  eax = line
 */
FUNC (ph_write_line_asm)

   /* check whether bitmap is already locked */
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz Locked

   /* set lock and autolock flags */
   orl $(BMP_ID_LOCKED | BMP_ID_AUTOLOCK), BMP_ID(%edx)

   pushl %ecx
   pushl %eax
   pushl %edx
   call GLOBL(PgWaitHWIdle)
   popl %edx
   popl %eax
   popl %ecx

 Locked:
   /* get pointer to the video memory */
   movl BMP_LINE(%edx,%eax,4), %eax

   ret



/* ph_unwrite_line_asm:
 *  edx = bmp
 */
FUNC (ph_unwrite_line_asm)

   /* only unlock bitmaps that were autolocked */
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz No_unlock

   /* clear lock and autolock flags */
   andl $~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK), BMP_ID(%edx)

 No_unlock:
   ret



/* ph_write_line_win_asm:
 *  edx = bitmap
 *  eax = line
 */
FUNC (ph_write_line_win_asm)
   pushl %ecx

   /* clobber the line */
   movl GLOBL(ph_dirty_lines), %ecx
   addl BMP_YOFFSET(%edx), %ecx
   movb $1, (%ecx,%eax)   /* ph_dirty_lines[line] = 1; (line has changed) */

   /* check whether bitmap is already locked */
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz Locked_win

   /* lock the surface */
   pushl %eax
   pushl %edx
   call *GLOBL(ptr_ph_acquire_win) 
   popl %edx
   popl %eax

   /* set the autolock flag */
   orl $BMP_ID_AUTOLOCK, BMP_ID(%edx)

 Locked_win:
   popl %ecx

   /* get pointer to the video memory */
   movl BMP_LINE(%edx,%eax,4), %eax

   ret



/* ph_unwrite_line_win_asm:
 *  edx = bmp
 */
FUNC (ph_unwrite_line_win_asm)

   /* only unlock bitmaps that were autolocked */
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz No_unlock_win

   /* unlock surface */
   pushl %ecx
   pushl %eax
   pushl %edx
   call *GLOBL(ptr_ph_release_win)
   popl %edx
   popl %eax
   popl %ecx

   /* clear the autolock flag */
   andl $~BMP_ID_AUTOLOCK, BMP_ID(%edx)

 No_unlock_win:
   ret

#endif
