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
 *      15 bit linear graphics functions.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"

#ifdef ALLEGRO_COLOR16

.text



/* void _linear_putpixel15(BITMAP *bmp, int x, int y, int color);
 *  Draws a pixel onto a linear bitmap.
 */
FUNC(_linear_putpixel15)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushw %es

   movl ARG1, %edx               /* edx = bmp */
   movl ARG2, %ecx               /* ecx = x */
   movl ARG3, %eax               /* eax = y */

   cmpl $0, BMP_CLIP(%edx)       /* test clip flag */
   je putpix_noclip
   cmpl %ecx, BMP_CL(%edx)       /* test cl */
   jg putpix_done
   cmpl %ecx, BMP_CR(%edx)       /* test cr */
   jle putpix_done
   cmpl %eax, BMP_CT(%edx)       /* test ct */
   jg putpix_done
   cmpl %eax, BMP_CB(%edx)       /* test cb */
   jle putpix_done

putpix_noclip:
   movw BMP_SEG(%edx), %es       /* segment selector */
   movl ARG4, %ebx               /* bx = color */

   cmpl $DRAW_SOLID, GLOBL(_drawing_mode)
   je putpix_solid_mode          /* solid draw? */

   cmpl $DRAW_XOR, GLOBL(_drawing_mode)
   je putpix_xor_mode            /* XOR? */

   cmpl $DRAW_TRANS, GLOBL(_drawing_mode)
   je putpix_trans_mode          /* translucent? */

   LOOKUP_PATTERN_POS(%ecx, %eax, %edx)
   movw (%eax, %ecx, 2), %bx     /* read pixel from pattern bitmap */

   movl ARG1, %edx               /* reload clobbered registers */
   movl ARG2, %ecx 
   movl ARG3, %eax 

   cmpl $DRAW_COPY_PATTERN, GLOBL(_drawing_mode)
   je putpix_solid_mode          /* draw the pattern pixel? */

   cmpw $MASK_COLOR_15, %bx      /* test the pattern pixel */
   je putpix_zero_pattern

   movl ARG4, %ebx               /* if set, draw color */
   jmp putpix_solid_mode

   _align_
putpix_zero_pattern:
   cmpl $DRAW_MASKED_PATTERN, GLOBL(_drawing_mode)
   je putpix_done                /* skip zero pixels in masked mode */
   xorl %ebx, %ebx
   jmp putpix_solid_mode         /* draw zero pixels in solid mode */

   _align_
putpix_xor_mode:
   READ_BANK()                   /* read pixel from screen */
   xorw %es:(%eax, %ecx, 2), %bx /* XOR */
   movl ARG3, %eax               /* re-read Y position */
   jmp putpix_solid_mode

   _align_
putpix_trans_mode:
   READ_BANK() 
   pushl %ecx
   pushl %edx
   movw %es:(%eax, %ecx, 2), %dx /* read pixel from screen */
   pushl GLOBL(_blender_alpha)
   pushl %edx
   pushl %ebx                    /* and the drawing color */
   call *GLOBL(_blender_func15)  /* blend */
   addl $12, %esp
   popl %edx
   popl %ecx
   movl %eax, %ebx
   movl ARG3, %eax               /* re-read Y position */

   _align_
putpix_solid_mode:
   WRITE_BANK()                  /* select bank */
   movw %bx, %es:(%eax, %ecx, 2) /* store the pixel */

putpix_done:
   popw %es

   UNWRITE_BANK()

   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_putpixel15() */




/* void _linear_hline15(BITMAP *bmp, int x1, int y, int x2, int color);
 *  Draws a horizontal line onto a linear bitmap.
 */
FUNC(_linear_hline15)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi
   pushw %es

   movl ARG1, %edx               /* edx = bmp */
   movl ARG2, %ebx               /* ebx = x1 */
   movl ARG3, %eax               /* eax = y */
   movl ARG4, %ecx               /* ecx = x2 */
   cmpl %ebx, %ecx
   jge hline_no_xswap
   xchgl %ecx, %ebx

hline_no_xswap:
   cmpl $0, BMP_CLIP(%edx)       /* test bmp->clip */
   je hline_noclip

   cmpl BMP_CT(%edx), %eax       /* test bmp->ct */
   jl hline_done

   cmpl BMP_CB(%edx), %eax       /* test bmp->cb */
   jge hline_done

   cmpl BMP_CL(%edx), %ebx       /* test x1, bmp->cl */
   jge hline_x1_ok
   cmpl BMP_CL(%edx), %ecx       /* test x2, bmp->cl */
   jl hline_done
   movl BMP_CL(%edx), %ebx       /* clip x1 */

hline_x1_ok:
   cmpl BMP_CR(%edx), %ecx       /* test x2, bmp->cr */
   jl hline_noclip
   cmpl BMP_CR(%edx), %ebx       /* test x1, bmp->cr */
   jge hline_done
   movl BMP_CR(%edx), %ecx       /* clip x2 */
   decl %ecx

hline_noclip:
   subl %ebx, %ecx               /* loop counter */
   incl %ecx

   movw BMP_SEG(%edx), %es       /* segment selector */

   WRITE_BANK()                  /* select write bank */
   leal (%eax, %ebx, 2), %edi    /* dest address in edi */

   cmpl $DRAW_SOLID, GLOBL(_drawing_mode)
   je hline_solid_mode           /* solid draw? */

   cmpl $DRAW_XOR, GLOBL(_drawing_mode)
   jne hline_not_xor             /* XOR? */

   movl ARG3, %eax               /* select read bank */
   READ_BANK() 
   leal (%eax, %ebx, 2), %esi    /* source address in esi */

   movw ARG5, %bx                /* read the color */

   _align_
hline_xor_loop:
   movw %es:(%esi), %ax          /* read a pixel */
   xorw %bx, %ax                 /* xor */
   movw %ax, %es:(%edi)          /* and write it */
   addl $2, %esi
   addl $2, %edi
   decl %ecx
   jg hline_xor_loop
   jmp hline_done

   _align_
hline_not_xor:
   cmpl $DRAW_TRANS, GLOBL(_drawing_mode)
   jne hline_not_trans           /* translucent? */

   movl ARG3, %eax               /* select read bank */
   READ_BANK() 
   leal (%eax, %ebx, 2), %esi    /* source address in esi */

   movl %ecx, ARG4               /* loop counter on the stack */

   _align_
hline_trans_loop:
   movw %es:(%esi), %ax          /* read pixel from screen */
   pushl GLOBL(_blender_alpha)
   pushl %eax
   pushl ARG5                    /* and the drawing color */
   call *GLOBL(_blender_func15)  /* blend */
   addl $12, %esp
   movw %ax, %es:(%edi)          /* and write it */
   addl $2, %esi
   addl $2, %edi
   decl ARG4
   jg hline_trans_loop
   jmp hline_done

   _align_
hline_not_trans:
   movl ARG3, %eax               /* get position in pattern bitmap */
   LOOKUP_PATTERN_POS(%ebx, %eax, %edx)
   movl %eax, %esi
   movl GLOBL(_drawing_x_mask), %edx

   cmpl $DRAW_COPY_PATTERN, GLOBL(_drawing_mode)
   je hline_copy_pattern_loop

   cmpl $DRAW_SOLID_PATTERN, GLOBL(_drawing_mode)
   je hline_solid_pattern_loop

   _align_
hline_masked_pattern_loop:
   movw (%esi, %ebx, 2), %ax     /* read a pixel */
   cmpw $MASK_COLOR_15, %ax      /* test it */
   je hline_masked_skip
   movw ARG5, %ax
   movw %ax, %es:(%edi)          /* write a colored pixel */
hline_masked_skip:
   incl %ebx
   andl %edx, %ebx
   addl $2, %edi
   decl %ecx
   jg hline_masked_pattern_loop
   jmp hline_done

   _align_
hline_copy_pattern_loop:
   movw (%esi, %ebx, 2), %ax     /* read a pixel */
   movw %ax, %es:(%edi)          /* and write it */
   incl %ebx
   andl %edx, %ebx
   addl $2, %edi
   decl %ecx
   jg hline_copy_pattern_loop
   jmp hline_done

   _align_
hline_solid_pattern_loop:
   movw (%esi, %ebx, 2), %ax     /* read a pixel */
   cmpw $MASK_COLOR_15, %ax      /* test it */
   je hline_solid_zero

   movw ARG5, %ax
   movw %ax, %es:(%edi)          /* write a colored pixel */
   incl %ebx
   andl %edx, %ebx
   addl $2, %edi
   decl %ecx
   jg hline_solid_pattern_loop
   jmp hline_done

   _align_
hline_solid_zero:
   movw $0, %es:(%edi)           /* write a zero pixel */
   incl %ebx
   andl %edx, %ebx
   addl $2, %edi
   decl %ecx
   jg hline_solid_pattern_loop
   jmp hline_done

   _align_
hline_solid_mode:                /* regular hline drawer */
   movw ARG5, %ax                /* get color */
   shll $16, %eax
   movw ARG5, %ax                /* get two copies of the color */
   cld

   testl $2, %edi                /* are we long aligned? */
   jz hline_l_aligned
   stosw                         /* if not, copy a word */
   decl %ecx

hline_l_aligned:
   shrl $1, %ecx                 /* for long copy */
   jz hline_no_long
   rep ; stosl                   /* do some 32 bit copies */

hline_no_long:
   jnc hline_done
   stosw                         /* do we need a 16 bit copy? */

hline_done:
   popw %es

   movl ARG1, %edx
   UNWRITE_BANK()

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_hline15() */




/* void _linear_vline15(BITMAP *bmp, int x, int y1, int y2, int color);
 *  Draws a vertical line onto a linear bitmap.
 */
FUNC(_linear_vline15)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi
   pushw %es 

   movl ARG1, %edx               /* edx = bmp */
   movl ARG2, %edi               /* edi = x */
   movl ARG3, %esi               /* esi = y1 */
   movl ARG4, %ecx               /* ecx = y2 */
   cmpl %esi, %ecx
   jge vline_no_xswap
   xchgl %ecx, %esi

vline_no_xswap:
   cmpl $0, BMP_CLIP(%edx)       /* test bmp->clip */
   je vline_noclip

   cmpl BMP_CL(%edx), %edi       /* test bmp->cl */
   jl vline_done

   cmpl BMP_CR(%edx), %edi       /* test bmp->cr */
   jge vline_done

   cmpl BMP_CT(%edx), %esi       /* test y1, bmp->ct */
   jge vline_y1_ok
   cmpl BMP_CT(%edx), %ecx       /* test y2, bmp->ct */
   jl vline_done
   movl BMP_CT(%edx), %esi       /* clip y1 */

vline_y1_ok:
   cmpl BMP_CB(%edx), %ecx       /* test y2, bmp->cb */
   jl vline_noclip
   cmpl BMP_CB(%edx), %esi       /* test y1, bmp->cb */
   jge vline_done
   movl BMP_CB(%edx), %ecx       /* clip y2 */
   decl %ecx

vline_noclip:
   subl %esi, %ecx               /* loop counter */
   incl %ecx

   movw BMP_SEG(%edx), %es       /* load segment */

   cmpl $DRAW_SOLID, GLOBL(_drawing_mode)
   je vline_solid_mode           /* solid draw? */

   movl BMP_CLIP(%edx), %eax     /* store destination clipping flag */
   pushl %eax
   movl $0, BMP_CLIP(%edx)       /* turn clipping off (we already did it) */

   movl ARG5, %eax
   pushl %eax                    /* push color */
   pushl %esi                    /* push y */
   pushl %edi                    /* push x */
   pushl %edx                    /* push bitmap */

   movl %ecx, %ebx               /* ebx = loop counter */

   _align_
vline_special_mode_loop:         /* vline drawer for funny modes */
   call GLOBL(_linear_putpixel15)/* draw the pixel */
   incl 8(%esp)                  /* next y */
   decl %ebx
   jg vline_special_mode_loop    /* loop */

   popl %edx                     /* clean up stack */
   addl $12, %esp
   popl %eax 
   movl %eax, BMP_CLIP(%edx)     /* restore bitmap clipping flag */
   jmp vline_done

   _align_
vline_solid_mode:                /* normal vline drawer */
   movl ARG5, %ebx               /* get color */

   _align_
vline_loop:
   movl %esi, %eax
   WRITE_BANK()                  /* select bank */

   movw %bx, %es:(%eax, %edi, 2) /* write pixel */

   incl %esi 
   decl %ecx
   jg vline_loop                 /* loop */

vline_done:
   popw %es 

   UNWRITE_BANK()

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_vline15() */




#endif      /* ifdef ALLEGRO_COLOR16 */

