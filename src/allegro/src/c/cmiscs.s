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
 *      Empty bank switch routines for DOS version.  Should be used together
 *      with C versions of GFX_VTABLE routines and ASM calling convention.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"

.text


/* empty bank switch routine for the standard VGA mode and memory bitmaps */
FUNC(_stub_bank_switch)
   movl BMP_LINE(%edx, %eax, 4), %eax
   ret

FUNC(_stub_unbank_switch)
   ret

FUNC(_stub_bank_switch_end)
   ret

