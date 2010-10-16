/*          ______   ___    ___
 *        /\  _  \ /\_ \  /\_ \
 *        \ \ \L\ \\//\ \ \//\ \      __     __   _ __   ___
 *         \ \  __ \ \ \ \  \ \ \   /'__`\ /'_ `\/\`'__\/ __`\
 *          \ \ \/\ \ \_\ \_ \_\ \_/\  __//\ \L\ \ \ \//\ \L\ \
 *           \ \_\ \_\/\____\/\____\ \____\ \____ \ \_\\ \____/
 *            \/_/\/_/\/____/\/____/\/____/\/___L\ \/_/ \/___/
 *                                           /\____/
 *                                           \_/__/
 *
 *      24 bit linear graphics functions.
 *
 *      By Michal Mertl.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"

#ifdef ALLEGRO_COLOR24

.text



/* void _linear_putpixel24(BITMAP *bmp, int x, int y, int color);
 *  Draws a pixel onto a linear bitmap.
 */
FUNC(_linear_putpixel24)
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
   leal (%ecx, %ecx, 2), %ecx
   movl (%eax, %ecx), %ebx       /* read pixel from pattern bitmap */
   andl $0xFFFFFF, %ebx

   movl ARG1, %edx               /* reload clobbered registers */
   movl ARG2, %ecx
   movl ARG3, %eax

   cmpl $DRAW_COPY_PATTERN, GLOBL(_drawing_mode)
   je putpix_solid_mode          /* draw the pattern pixel? */

   cmpl $MASK_COLOR_24, %ebx     /* test the pattern pixel */
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
   READ_BANK()                   /* read pixel from screen, and XOR it */
   leal (%ecx, %ecx, 2), %ecx    /* ecx *= 3 */
   movl %es:(%eax, %ecx), %eax
   xorl %eax, %ebx
   movl ARG3, %eax               /* re-read Y position */
   WRITE_BANK()
   movw %bx, %es:(%eax, %ecx)
   shrl $16, %ebx
   movb %bl, %es:2(%eax, %ecx)
   jmp putpix_done

   _align_
putpix_trans_mode:
   READ_BANK() 
   leal (%ecx, %ecx, 2), %ecx    /* ecx *= 3 */
   pushl %ecx
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %es:(%eax, %ecx)        /* read pixel from screen */
   pushl %ebx                    /* and the drawing color */
   call *GLOBL(_blender_func24)  /* blend */
   addl $12, %esp
   popl %edx
   popl %ecx
   movl %eax, %ebx
   movl ARG3, %eax               /* re-read Y position */
   WRITE_BANK()
   movw %bx, %es:(%eax, %ecx)
   shrl $16, %ebx
   movb %bl, %es:2(%eax, %ecx)
   jmp putpix_done

   _align_
putpix_solid_mode:
   WRITE_BANK()                  /* write the pixel */
   leal (%ecx, %ecx, 2), %ecx
   movw %bx, %es:(%eax, %ecx)
   shrl $16, %ebx
   movb %bl, %es:2(%eax, %ecx)
putpix_done:
   popw %es

   UNWRITE_BANK()

   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_putpixel24() */




/* int _linear_getpixel24(BITMAP *bmp, int x, int y);
 *  Reads a pixel from a linear bitmap.
 */
FUNC(_linear_getpixel24)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushw %es

   xorl %eax, %eax
   xorl %edx, %edx
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
   leal (%ecx, %ecx, 2), %ecx
   movl %es:(%eax, %ecx), %ebx
   andl $0xFFFFFF, %ebx

getpix_done:
   popw %es

   UNWRITE_BANK()

   movl %ebx, %eax
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_getpixel24() */




/* void _linear_hline24(BITMAP *bmp, int x1, int y, int x2, int color);
 *  Draws a horizontal line onto a linear bitmap.
 */
FUNC(_linear_hline24)
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

   pushl %ebx
   leal (%ebx, %ebx, 2), %ebx    /* ebx *= 3 */
   leal (%eax, %ebx), %edi       /* dest address in edi=eax+ebx */
   popl %ebx

   cmpl $DRAW_SOLID, GLOBL(_drawing_mode)
   je hline_solid_mode           /* solid draw? */

   cmpl $DRAW_XOR, GLOBL(_drawing_mode)
   jne hline_not_xor             /* XOR? */

   movl ARG3, %eax               /* select read bank */
   READ_BANK()
   leal (%ebx, %ebx, 2), %ebx
   leal (%eax, %ebx), %esi       /* source address in esi */

   movl ARG5, %eax               /* pixel color */
   movl %eax, %ebx
   movl %eax, ARG2               /* ARG2 == 12(%ebp) */
   movl %eax, 15(%ebp)           /* overwrite ARG2, ARG3, ARG4 */
   movl %eax, 18(%ebp)
   movw %ax, 21(%ebp)
   shrl $16, %eax
   movb %al, 23(%ebp)
   cmpl $4, %ecx
   jb hline_xor_short
   subl $4, %ecx

   _align_
hline_xor_loop:
   movl %es:(%esi), %eax         /* read 1st 4 bytes */
   xorl ARG2, %eax               /* xor it */
   movl %eax, %es:(%edi)         /* write it back */
   movl %es:4(%esi), %eax        /* 2nd 4 bytes */
   xorl ARG3, %eax
   movl %eax, %es:4(%edi)
   movl %es:8(%esi), %eax        /* 3rd 4 bytes */
   xorl ARG4, %eax
   movl %eax, %es:8(%edi)
   addl $12, %esi
   addl $12, %edi
   subl $4, %ecx
   jns  hline_xor_loop
   addl $4, %ecx
   jz  hline_done
   _align_
hline_xor_short:
   movl %es:(%esi), %eax
   xorl ARG5, %eax               /* xor */
   movw %ax, %es:(%edi)
   shrl $16, %eax
   movb %al, %es:2(%edi)
   addl $3, %esi
   addl $3, %edi
   decl %ecx
   jg hline_xor_short
   jmp hline_done

   _align_
hline_not_xor:
   cmpl $DRAW_TRANS, GLOBL(_drawing_mode)
   jne hline_not_trans           /* translucent? */

   movl ARG3, %eax               /* select read bank */
   READ_BANK() 
   leal (%ebx, %ebx, 2), %ebx
   leal (%eax, %ebx), %esi       /* source address in esi */

   movl %ecx, ARG4               /* loop counter on the stack */

   _align_
hline_trans_loop:
   pushl GLOBL(_blender_alpha)
   pushl %es:(%esi)              /* read pixel from screen */
   pushl ARG5                    /* and the drawing color */
   call *GLOBL(_blender_func24)  /* blend */
   addl $12, %esp
   movw %ax, %es:(%edi)          /* and write it */
   shrl $16, %eax
   movb %al, %es:2(%edi)
   addl $3, %esi
   addl $3, %edi
   decl ARG4
   jg hline_trans_loop
   jmp hline_done

   _align_
hline_not_trans:
   movl ARG3, %eax               /* get position in pattern bitmap */
   movl ARG2, %ebx
   LOOKUP_PATTERN_POS(%ebx, %eax, %edx)
   movl %eax, %esi
   movl GLOBL(_drawing_x_mask), %edx

   cmpl $DRAW_COPY_PATTERN, GLOBL(_drawing_mode)
   je hline_copy_pattern

   cmpl $DRAW_SOLID_PATTERN, GLOBL(_drawing_mode)
   je hline_solid_pattern_loop

   _align_
hline_masked_pattern_loop:
   leal (%ebx, %ebx, 2), %eax
   movl (%esi, %eax), %eax       /* read a pixel */
   andl $0xFFFFFF, %eax
   cmpl $MASK_COLOR_24, %eax     /* test it */
   je hline_masked_skip
   movl ARG5, %eax
   movw %ax, %es:(%edi)
   shrl $16, %eax 
   movb %al, %es:2(%edi)
   _align_
hline_masked_skip:
   incl %ebx 
   andl %edx, %ebx
   addl $3, %edi
   decl %ecx
   jg hline_masked_pattern_loop
   jmp hline_done

hline_copy_pattern:
   decl %ecx
   jz short_copy_pattern
   _align_
hline_copy_pattern_loop:
   leal (%ebx, %ebx, 2), %eax
   movl (%esi, %eax), %eax       /* read a pixel */
   movl %eax, %es:(%edi)
   incl %ebx
   andl %edx, %ebx
   addl $3, %edi
   decl %ecx
   jg hline_copy_pattern_loop
short_copy_pattern:
   leal (%ebx, %ebx, 2), %eax
   movl (%esi, %eax), %eax       /* read a pixel */
   movw %ax, %es:(%edi)
   shrl $16, %eax
   movb %al, %es:2(%edi)

   jmp hline_done

   _align_
hline_solid_pattern_loop:
   leal (%ebx, %ebx, 2), %eax
   movl (%esi, %eax), %eax       /* read a pixel */
   andl $0xFFFFFF, %eax
   cmpl $MASK_COLOR_24, %eax     /* test it */
   je hline_solid_zero

   movl ARG5, %eax
   movw %ax, %es:(%edi)
   shrl $16, %eax 
   movb %al, %es:2(%edi)         /* write a colored pixel */
   incl %ebx
   andl %edx, %ebx
   addl $3, %edi
   decl %ecx
   jg hline_solid_pattern_loop
   jmp hline_done

   _align_
hline_solid_zero:
   movw $0, %es:(%edi)           /* write a zero pixel */
   movb $0, %es:2(%edi)
   incl %ebx
   andl %edx, %ebx
   addl $3, %edi
   decl %ecx
   jg hline_solid_pattern_loop
   jmp hline_done

   _align_
hline_solid_mode:                /* regular hline drawer */
   movl ARG5, %eax               /* ARG2 = 12(%ebp) pixel color */
   movl %eax, ARG2               /* overwrite ARG2, ARG3 */
   movl %eax, 15(%ebp)
   movl ARG2, %eax
   movl 13(%ebp), %ebx
   movl 14(%ebp), %edx

     /*   al,ah,high al,ah,bl,bh ,high bl,bh,dl,dh,high dl,dh  */
     /*   1st pixel    ,2nd pixel,3rd pixel    ,4th pixel      */ 
     /*   ------eax-------,------ebx--------,------edx-------  */

   cmpl $4, %ecx 
   jb short_line 
   subl $4, %ecx                 /* test for sign flag */
   cld
   _align_
hline_solid_loop:                /* plots 4 pixels an once */
   movl %eax, %es:(%edi)
   movl %ebx, %es:4(%edi)
   movl %edx, %es:8(%edi)
   addl $12, %edi
   subl $4, %ecx
   jns  hline_solid_loop
   addl $4, %ecx                 /* add the helper back */
   jz  hline_done
   _align_
short_line:
   movw %ax, %es:(%edi)          /* plots remaining pixels (max 3) */
   movb %dl, %es:2(%edi)
   addl $3, %edi
   decl %ecx
   jg short_line
hline_done:
   popw %es

   movl ARG1, %edx
   UNWRITE_BANK()

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_hline24 */




/* void _linear_vline24(BITMAP *bmp, int x, int y1, int y2, int color);
 *  Draws a vertical line onto a linear bitmap.
 */
FUNC(_linear_vline24)
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
   call GLOBL(_linear_putpixel24)/* draw the pixel */
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
   leal (%edi, %edi, 2), %edi    /* xadd on each line */
   movl %ebx, %eax
   shrl $16, %eax
   movl %eax, ARG4

   _align_
vline_loop:
   movl %esi, %eax
   WRITE_BANK()                  /* select bank */

   movl ARG5, %ebx
   movw %bx, %es:(%eax, %edi)
   movl ARG4, %ebx
   movb %bl, %es:2(%eax, %edi)

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
   ret                           /* end of _linear_vline24() */




#endif      /* ifdef ALLEGRO_COLOR24 */

