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
 *      A few macros to make my asm code (slightly :-) more readable.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_ASMDEFS_INC
#define ALLEGRO_ASMDEFS_INC


#ifndef SCAN_DEPEND
   #include "allegro/platform/alplatf.h"
#endif


#include "gen_asmdefs.inc"


/* helper macro in case we ever need to change this */
#define _align_   .balign 4, 0x90


/* readable way to access arguments passed from C code */
#define ARG1      8(%ebp)
#define ARG2      12(%ebp)
#define ARG3      16(%ebp)
#define ARG4      20(%ebp)
#define ARG5      24(%ebp)
#define ARG6      28(%ebp)
#define ARG7      32(%ebp)
#define ARG8      36(%ebp)
#define ARG9      40(%ebp)
#define ARG10     44(%ebp)


/* Bank switching macros. These should be called with a pointer to the
 * bitmap structure in %edx, and the line number you want to access in
 * %eax. Registers will be unchanged, except %eax will return a pointer 
 * to the start of the selected scanline.
 */
#define WRITE_BANK()    call *BMP_WBANK(%edx)
#define READ_BANK()     call *BMP_RBANK(%edx)

#define UNWRITE_BANK()  movl BMP_VTABLE(%edx), %eax ; call *VTABLE_UNBANK(%eax)
#define UNREAD_BANK()   movl BMP_VTABLE(%edx), %eax ; call *VTABLE_UNBANK(%eax)


/* Helper macro for looking up a position in the pattern bitmap. Passed
 * registers containing the x and y coordinates of the point, it returns
 * a 'start of pattern line' pointer in y, and an offset into this line
 * in x. It clobbers the tmp register.
 */
#define LOOKUP_PATTERN_POS(x, y, tmp)                                      ; \
   subl GLOBL(_drawing_x_anchor), x       /* adjust x */                   ; \
   andl GLOBL(_drawing_x_mask), x         /* limit range of x */           ; \
									   ; \
   subl GLOBL(_drawing_y_anchor), y       /* adjust y */                   ; \
   andl GLOBL(_drawing_y_mask), y         /* limit range of y */           ; \
									   ; \
   movl GLOBL(_drawing_pattern), tmp      /* get position in pattern */    ; \
   movl BMP_LINE(tmp, y, 4), y


/* How many stacks to allocate for the irq wrappers. This can't be in the 
 * main headers, because it is used by both C and asm code. You could 
 * probably get away with fewer of these, if you want to save memory and
 * you are feeling brave...
 */
#define IRQ_STACKS      8



#endif   /* ifndef ALLEGRO_ASMDEFS_INC */

