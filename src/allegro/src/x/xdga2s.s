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
 *      DGA2 bank switching code. These routines will be called 
 *      with a line number in %eax and a pointer to the bitmap in %edx.
 *      The bank switcher should select the appropriate bank for the 
 *      line, and replace %eax with a pointer to the start of the line.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro/platform/alunixac.h"
#include "../i386/asmdefs.inc"

.text


#if (!defined ALLEGRO_NO_ASM) && (defined ALLEGRO_XWINDOWS_WITH_XF86DGA2)
#if (!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE)

FUNC (_xdga2_write_line_asm)

	/* check whether bitmap is already locked */
	testl $BMP_ID_LOCKED, BMP_ID(%edx)
	jnz Locked

	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(_xdga2_lock)
	popl %edx
	popl %eax
	popl %ecx

   Locked:
	/* get pointer to the video memory */
	movl BMP_LINE(%edx,%eax,4), %eax

	ret

#endif
#endif

