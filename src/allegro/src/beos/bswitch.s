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
 *      BeOS gfx line switchers.
 *
 *      By Jason Wilkins.
 *
 *      Windowed mode support added by Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "src/i386/asmdefs.inc"



.text


/* _be_gfx_bwindowscreen_read_write_bank_asm:
 *   eax = line number
 *   edx = bitmap
 */
FUNC(_be_gfx_bwindowscreen_read_write_bank_asm)

   /* check whether bitmap is already locked */
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz Locked

   pushl %ecx
   pushl %eax
   pushl %edx
   call *GLOBL(_be_sync_func)
   popl %edx
   popl %eax
   popl %ecx

   /* set lock and autolock flags */
   orl $(BMP_ID_LOCKED | BMP_ID_AUTOLOCK), BMP_ID(%edx)

 Locked:
   /* get pointer to the video memory */
   movl BMP_LINE(%edx,%eax,4), %eax

   ret



/* _be_gfx_bwindowscreen_unwrite_bank_asm:
 *   edx = bitmap
 */
FUNC(_be_gfx_bwindowscreen_unwrite_bank_asm)

   /* only unlock bitmaps that were autolocked */
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz No_unlock

   /* clear lock and autolock flags */
   andl $~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK), BMP_ID(%edx)

 No_unlock:
   ret



/* _be_gfx_bwindow_read_write_bank_asm:
 *   eax = line number
 *   edx = bitmap
 */
FUNC(_be_gfx_bwindow_read_write_bank_asm)

   /* clobber the line */
   pushl %ecx
   pushl %eax
   movl GLOBL(_be_dirty_lines), %ecx
   addl BMP_YOFFSET(%edx), %eax
   movl $1, (%ecx, %eax, 4)  /* _be_dirty_lines[line] = 1; (line has changed) */
   popl %eax
   popl %ecx

   /* check whether bitmap is already locked */
   testl $BMP_ID_LOCKED, BMP_ID(%edx)
   jnz Locked_win

   /* set lock and autolock flags */
   orl $(BMP_ID_LOCKED | BMP_ID_AUTOLOCK), BMP_ID(%edx)

 Locked_win:
   /* get pointer to the video memory */
   movl BMP_LINE(%edx,%eax,4), %eax

   ret



/* _be_gfx_bwindow_unwrite_bank_asm:
 *   edx = bitmap
 */
FUNC(_be_gfx_bwindow_unwrite_bank_asm)

   /* only unlock bitmaps that were autolocked */
   testl $BMP_ID_AUTOLOCK, BMP_ID(%edx)
   jz No_unlock_win
   
   pushl %ecx
   pushl %eax
   pushl %edx
   pushl GLOBL(_be_window_lock)
   call GLOBL(release_sem)
   popl %edx  /* throw away GLOBL(_be_window_lock) */
   popl %edx
   popl %eax
   popl %ecx

   andl $~(BMP_ID_LOCKED | BMP_ID_AUTOLOCK), BMP_ID(%edx)

  No_unlock_win:
   ret 
