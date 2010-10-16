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
 *      SVGAlib bank switching code. These routines will be called 
 *      with a line number in %eax and a pointer to the bitmap in %edx.
 *      The bank switcher should select the appropriate bank for the 
 *      line, and replace %eax with a pointer to the start of the line.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro/platform/alunixac.h"
#include "../i386/asmdefs.inc"


.text


#if (!defined ALLEGRO_NO_ASM) && (defined ALLEGRO_LINUX_SVGALIB)
#if (!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE)


FUNC(_svgalib_read_line_asm)
	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(_svgalib_read_line)
	popl %edx
	popl %ecx  /* preserve %eax */
	popl %ecx
	ret


FUNC(_svgalib_write_line_asm)
	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(_svgalib_write_line)
	popl %edx
	popl %ecx  /* preserve %eax */
	popl %ecx
	ret


FUNC(_svgalib_unwrite_line_asm)
	pushl %ecx
	pushl %eax
	pushl %edx
	call GLOBL(_svgalib_unwrite_line)
	popl %edx
	popl %eax
	popl %ecx
	ret


#endif
#endif
