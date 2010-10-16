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
 *      256 color linear graphics functions.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"

#ifdef ALLEGRO_COLOR8

.text



/* void _linear_putpixel8(BITMAP *bmp, int x, int y, int color);
 *  Draws a pixel onto a linear bitmap.
 */
FUNC(_linear_putpixel8)
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
   movb (%eax, %ecx), %bl        /* read pixel from pattern bitmap */

   movl ARG1, %edx               /* reload clobbered registers */
   movl ARG2, %ecx 
   movl ARG3, %eax 

   cmpl $DRAW_COPY_PATTERN, GLOBL(_drawing_mode)
   je putpix_solid_mode          /* draw the pattern pixel? */

   testb %bl, %bl                /* test the pattern pixel */
   jz putpix_zero_pattern

   movl ARG4, %ebx               /* if set, draw color */
   jmp putpix_solid_mode

   _align_
putpix_zero_pattern:
   cmpl $DRAW_MASKED_PATTERN, GLOBL(_drawing_mode)
   je putpix_done                /* skip zero pixels in masked mode */
   jmp putpix_solid_mode         /* draw zero pixels in solid mode */

   _align_
putpix_xor_mode:
   READ_BANK()                   /* read pixel from screen */
   xorb %es:(%eax, %ecx), %bl    /* XOR */
   movl ARG3, %eax               /* re-read Y position */
   jmp putpix_solid_mode

   _align_
putpix_trans_mode:
   shll $8, %ebx                 /* shift color into high byte */
   READ_BANK()                   /* read from screen */
   movb %es:(%eax, %ecx), %bl    /* screen pixel into low byte */
   movl GLOBL(color_map), %eax   /* color map in eax */
   andl $0xFFFF, %ebx            /* mask high bits of ebx */
   movb (%eax, %ebx), %bl        /* lookup color in the table */
   movl ARG3, %eax               /* re-read Y position */

   _align_
putpix_solid_mode:
   WRITE_BANK()                  /* select bank */
   movb %bl, %es:(%eax, %ecx)    /* store the pixel */

putpix_done:
   popw %es

   UNWRITE_BANK()

   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_putpixel8() */




/* int _linear_getpixel8(BITMAP *bmp, int x, int y);
 *  Reads a pixel from a linear bitmap.
 */
FUNC(_linear_getpixel8)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushw %es

   movl ARG1, %edx               /* edx = bmp */
   movl ARG2, %ecx               /* ecx = x */
   movl ARG3, %eax               /* eax = y */

   testl %ecx, %ecx              /* test that we are on the bitmap */
   jl getpix_skip
   cmpl %ecx, BMP_W(%edx)
   jle getpix_skip
   testl %eax, %eax
   jl getpix_skip
   cmpl %eax, BMP_H(%edx)
   jg getpix_ok

   _align_
getpix_skip:                     /* return -1 if we are off the edge */
   movl $-1, %ebx
   jmp getpix_done

   _align_
getpix_ok:
   READ_BANK()                   /* select the bank */

   movw BMP_SEG(%edx), %es       /* segment selector */
   movl %eax, %ebx
   movzbl %es:(%ebx, %ecx), %ebx /* get the pixel */

getpix_done:
   popw %es

   UNWRITE_BANK()

   movl %ebx, %eax
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_getpixel8() */




/* void _linear_hline8(BITMAP *bmp, int x1, int y, int x2, int color);
 *  Draws a horizontal line onto a linear bitmap.
 */
FUNC(_linear_hline8)
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
   movl %eax, %edi
   addl %ebx, %edi               /* dest address in edi */

   cmpl $DRAW_SOLID, GLOBL(_drawing_mode)
   je hline_solid_mode           /* solid draw? */

   cmpl $DRAW_XOR, GLOBL(_drawing_mode)
   jne hline_not_xor             /* XOR? */

   movl ARG3, %eax               /* select read bank */
   READ_BANK() 
   movl %eax, %esi
   addl %ebx, %esi               /* source address in esi */

   movb ARG5, %bl                /* read the color */

   _align_
hline_xor_loop:
   movb %es:(%esi), %al          /* read a pixel */
   xorb %bl, %al                 /* xor */
   movb %al, %es:(%edi)          /* and write it */
   incl %esi
   incl %edi
   decl %ecx
   jg hline_xor_loop
   jmp hline_done

   _align_
hline_not_xor:
   cmpl $DRAW_TRANS, GLOBL(_drawing_mode)
   jne hline_not_trans           /* translucent? */

   movl ARG3, %eax               /* select read bank */
   READ_BANK() 
   movl %eax, %esi
   addl %ebx, %esi               /* source address in esi */

   xorl %ebx, %ebx
   movb ARG5, %bh                /* read color into high byte */

   movl GLOBL(color_map), %edx   /* color mapping table in edx */

   shrl $1, %ecx                 /* adjust counter for 16 bit copies */
   jnc hline_no_trans_byte

   movb %es:(%esi), %bl          /* write odd pixel */
   movb (%edx, %ebx), %al        /* table lookup */
   movb %al, %es:(%edi) 
   incl %esi
   incl %edi

hline_no_trans_byte:
   shrl $1, %ecx                 /* adjust counter for 32 bit copies */
   jnc hline_no_trans_word

   movw %es:(%esi), %ax          /* write two odd pixels */
   movb %al, %bl
   movb (%edx, %ebx), %al        /* table lookup 1 */
   movb %ah, %bl
   movb (%edx, %ebx), %ah        /* table lookup 2 */
   movw %ax, %es:(%edi) 
   addl $2, %esi
   addl $2, %edi

hline_no_trans_word:
   orl %ecx, %ecx                /* anything left to copy? */
   jz hline_done

   _align_
hline_trans_loop:
   movl %es:(%esi), %eax         /* read four pixels at a time */
   movb %al, %bl
   movb (%edx, %ebx), %al        /* table lookup 1 */
   movb %ah, %bl
   movb (%edx, %ebx), %ah        /* table lookup 2 */
   roll $16, %eax
   movb %al, %bl
   movb (%edx, %ebx), %al        /* table lookup 3 */
   movb %ah, %bl
   movb (%edx, %ebx), %ah        /* table lookup 4 */
   roll $16, %eax
   movl %eax, %es:(%edi)         /* write four pixels */
   addl $4, %esi
   addl $4, %edi
   decl %ecx
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

   movb ARG5, %ah                /* color in ah */

   cmpl $DRAW_SOLID_PATTERN, GLOBL(_drawing_mode)
   je hline_solid_pattern_loop

   _align_
hline_masked_pattern_loop:
   movb (%esi, %ebx), %al        /* read a pixel */
   testb %al, %al                /* test it */
   jz hline_masked_skip
   movb %ah, %es:(%edi)          /* write a colored pixel */
hline_masked_skip:
   incl %ebx
   andl %edx, %ebx
   incl %edi
   decl %ecx
   jg hline_masked_pattern_loop
   jmp hline_done

   _align_
hline_copy_pattern_loop:
   movb (%esi, %ebx), %al        /* read a pixel */
   movb %al, %es:(%edi)          /* and write it */
   incl %ebx
   andl %edx, %ebx
   incl %edi
   decl %ecx
   jg hline_copy_pattern_loop
   jmp hline_done

   _align_
hline_solid_pattern_loop:
   movb (%esi, %ebx), %al        /* read a pixel */
   testb %al, %al                /* test it */
   jz hline_solid_zero

   movb %ah, %es:(%edi)          /* write a colored pixel */
   incl %ebx
   andl %edx, %ebx
   incl %edi
   decl %ecx
   jg hline_solid_pattern_loop
   jmp hline_done

   _align_
hline_solid_zero:
   movb $0, %es:(%edi)           /* write a zero pixel */
   incl %ebx
   andl %edx, %ebx
   incl %edi
   decl %ecx
   jg hline_solid_pattern_loop
   jmp hline_done

   _align_
hline_solid_mode:                /* regular hline drawer */
   movb ARG5, %dl                /* get color */
   movb %dl, %dh
   shrdl $16, %edx, %eax
   movw %dx, %ax                 /* get four copies of the color */
   cld

   testl $1, %edi                /* are we word aligned? */
   jz hline_w_aligned
   stosb                         /* if not, copy a pixel */
   decl %ecx
hline_w_aligned:
   testl $2, %edi                /* are we long aligned? */
   jz hline_l_aligned
   subl $2, %ecx
   jl hline_oops
   stosw                         /* if not, copy a word */
hline_l_aligned:
   movw %cx, %dx                 /* store low bits */
   shrl $2, %ecx                 /* for long copy */
   jz hline_no_long
   rep ; stosl                   /* do some 32 bit copies */
hline_no_long:
   testw $2, %dx
   jz hline_no_word
   stosw                         /* do we need a 16 bit copy? */
hline_no_word:
   testw $1, %dx
   jz hline_done
   stosb                         /* do we need a byte copy? */

hline_done:
   popw %es

   movl ARG1, %edx
   UNWRITE_BANK()

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret

hline_oops:                      /* fix up ecx for short line? */
   addl $2, %ecx
   jmp hline_l_aligned




/* void _linear_vline8(BITMAP *bmp, int x, int y1, int y2, int color);
 *  Draws a vertical line onto a linear bitmap.
 */
FUNC(_linear_vline8)
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
   call GLOBL(_linear_putpixel8) /* draw the pixel */
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

   movb %bl, %es:(%eax, %edi)    /* write pixel */

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
   ret                           /* end of _linear_vline8() */




#endif      /* ifdef ALLEGRO_COLOR8 */

