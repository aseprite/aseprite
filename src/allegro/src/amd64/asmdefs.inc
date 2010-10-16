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

#if defined ALLEGRO_UNIX
  #define FUNC(name)            .globl name ; _align_ ; .type name,@function ; name:
#else
  #define FUNC(name)            .globl name ; _align_ ; name:
#endif


/* helper macro in case we ever need to change this */
#define _align_   .balign 4, 0x90


/* readable way to access the nth argument passed from C code
 *    i is the number of 32 bit arguments passed before and including this one,
 *    l is the number of 64 bit arguments passed before and including this one
 */
#define ARG(i,l)  (4*(i+1) + 8*l)(%rbp)

/* integer arguments */
#define ARG1      ARG(1, 0)
#define ARG2      ARG(2, 0)
#define ARG3      ARG(3, 0)
#define ARG4      ARG(4, 0)
#define ARG5      ARG(5, 0)
#define ARG6      ARG(6, 0)
#define ARG7      ARG(7, 0)
#define ARG8      ARG(8, 0)
#define ARG9      ARG(9, 0)
#define ARG10     ARG(10, 0)


/* Bank switching macros. These should be called with a pointer to the
 * bitmap structure in %rdx, and the line number you want to access in
 * %rax. Registers will be unchanged, except %rax will return a pointer 
 * to the start of the selected scanline.
 */
#define WRITE_BANK()    call *BMP_WBANK(%rdx)
#define READ_BANK()     call *BMP_RBANK(%rdx)

#define UNWRITE_BANK()  movl BMP_VTABLE(%rdx), %rax ; call *VTABLE_UNBANK(%rax)
#define UNREAD_BANK()   movl BMP_VTABLE(%rdx), %rax ; call *VTABLE_UNBANK(%rax)


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
   movl BMP_LINE(tmp, y, sizeof(unsigned char*)), y


/* How many stacks to allocate for the irq wrappers. This can't be in the 
 * main headers, because it is used by both C and asm code. You could 
 * probably get away with fewer of these, if you want to save memory and
 * you are feeling brave...
 */
#define IRQ_STACKS      8



#endif   /* ifndef ALLEGRO_ASMDEFS_INC */

