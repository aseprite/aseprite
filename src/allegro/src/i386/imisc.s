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
 *      Math routines, compiled sprite wrapper, etc.
 *
 *      By Shawn Hargreaves.
 *
 *      fixsqrt() and fixhypot() routines by David Kuhling.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"

.text



/* empty bank switch routine for the standard VGA mode and memory bitmaps */
FUNC(_stub_bank_switch)
   movl BMP_LINE(%edx, %eax, 4), %eax
   ret

FUNC(_stub_unbank_switch)
   ret

FUNC(_stub_bank_switch_end)
   ret




/* void apply_matrix_f(MATRIX_f *m, float x, float y, float z, 
 *                                  float *xout, float *yout, float *zout);
 *  Floating point vector by matrix multiplication routine.
 */
FUNC(apply_matrix_f)

   #define MTX    ARG1
   #define X      ARG2
   #define Y      ARG3
   #define Z      ARG4
   #define XOUT   ARG5
   #define YOUT   ARG6
   #define ZOUT   ARG7

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx

   movl MTX, %edx 
   movl XOUT, %eax 
   movl YOUT, %ebx 
   movl ZOUT, %ecx 

   flds  M_V00(%edx) 
   fmuls X 
   flds  M_V01(%edx) 
   fmuls Y 
   flds  M_V02(%edx) 
   fmuls Z 
   fxch  %st(2) 

   faddp %st(0), %st(1) 
   flds  M_V10(%edx) 
   fxch  %st(2) 

   faddp %st(0), %st(1) 
   fxch  %st(1) 

   fmuls X 
   fxch  %st(1) 

   fadds M_T0(%edx) 
   flds  M_V11(%edx) 

   fmuls Y 
   flds  M_V12(%edx) 

   fmuls Z 
   fxch  %st(1) 

   faddp %st(0), %st(3) 
   flds  M_V20(%edx) 
   fxch  %st(3) 

   faddp %st(0), %st(1) 
   fxch  %st(2) 

   fmuls X 
   fxch  %st(2) 

   fadds M_T1(%edx) 
   flds  M_V21(%edx) 

   fmuls Y 
   flds  M_V22(%edx) 

   fmuls Z 
   fxch  %st(4) 

   faddp %st(0), %st(1) 
   fxch  %st(1) 
   fstps (%ebx) 

   faddp %st(0), %st(2) 
   fstps (%eax) 

   fadds M_T2(%edx) 
   fstps (%ecx)

   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                              /* end of apply_matrix_f() */




#undef X
#undef Y




/* void draw_compiled_sprite(BITMAP *bmp, COMPILED_SPRITE *sprite, int x, y)
 *  Draws a compiled sprite onto the specified bitmap at the specified
 *  position, _ignoring_ clipping. The bitmap must be in the same format
 *  that the sprite was compiled for.
 */
FUNC(draw_compiled_sprite)

   #define BMP       ARG1
   #define SPRITE    ARG2
   #define X         ARG3
   #define Y         ARG4

   pushl %ebp
   movl %esp, %ebp
   subl $4, %esp                 /* 1 local variable: */

   #define PLANE     -4(%ebp)

   pushl %ebx
   pushl %esi
   pushl %edi

   movl BMP, %edx                /* bitmap pointer in edx */
 #ifdef USE_FS
   movw BMP_SEG(%edx), %fs       /* load segment selector into fs */
 #endif

   movl SPRITE, %ebx
   cmpw $0, CMP_PLANAR(%ebx)     /* is the sprite planar or linear? */
   je linear_compiled_sprite

   movl X, %ecx                  /* get write plane mask in bx */
   andb $3, %cl
   movl $0x1102, %ebx
   shlb %cl, %bh

   movl BMP_LINE+4(%edx), %ecx   /* get line width in ecx */
   subl BMP_LINE(%edx), %ecx

   movl X, %esi                  /* get destination address in edi */
   shrl $2, %esi
   movl Y, %edi
   movl BMP_LINE(%edx, %edi, 4), %edi
   addl %esi, %edi

   movl $0x3C4, %edx             /* port address in dx */

   movl $0, PLANE                /* zero the plane counter */

   _align_
planar_compiled_sprite_loop:
   movl %ebx, %eax               /* set the write plane */
   outw %ax, %dx 

   movl %edi, %eax               /* get address in eax */

   movl PLANE, %esi              /* get the drawer function in esi */
   shll $3, %esi
   addl SPRITE, %esi
   movl CMP_DRAW(%esi), %esi

   call *%esi                    /* and draw the plane! */

   incl PLANE                    /* next plane */
   cmpl $4, PLANE
   jge draw_compiled_sprite_done

   rolb $1, %bh                  /* advance the plane position */
   adcl $0, %edi
   jmp planar_compiled_sprite_loop

   _align_
linear_compiled_sprite:
   movl X, %eax
   movzwl CMP_COLOR_DEPTH(%ebx), %ecx
   cmpl $24, %ecx
   jne normal_linear_compiled_sprite
   leal (%eax, %eax, 2), %eax
   jmp end24bpp_linear_compiled_sprite

   _align_
normal_linear_compiled_sprite:
   addl $7, %ecx
   shrl $4, %ecx
   shll %cl, %eax

end24bpp_linear_compiled_sprite:
   movl %eax, %ecx               /* x coordinate in ecx */
   movl Y, %edi                  /* y coordinate in edi */
   movl BMP_WBANK(%edx), %esi    /* bank switch function in esi */
   movl CMP_DRAW(%ebx), %ebx     /* drawer function in ebx */

   call *%ebx                    /* and draw it! */

draw_compiled_sprite_done:
   movl BMP, %edx
   UNWRITE_BANK()

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of draw_compiled_sprite() */




/* void _do_stretch(BITMAP *source, BITMAP *dest, void *drawer, 
 *                  int sx, fixed sy, fixed syd, int dx, int dy, int dh, 
 *                  int color_depth);
 *
 *  Helper function for stretch_blit(), calls the compiled line drawer.
 */
FUNC(_do_stretch)

   #define SOURCE       ARG1
   #define DEST         ARG2
   #define DRAWER       ARG3
   #define SX           ARG4
   #define SY           ARG5
   #define SYD          ARG6
   #define DX           ARG7
   #define DY           ARG8
   #define DH           ARG9
   #define COL_DEPTH    ARG10

   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushl %esi
   pushl %ebx
   pushw %es

   movl DEST, %edx
   movw BMP_SEG(%edx), %es       /* load destination segment */
   movl DRAWER, %ebx             /* the actual line drawer */

   movl BMP_ID(%edx), %eax
   testl $BMP_ID_PLANAR, %eax
   jnz stretch_modex_loop
   movl COL_DEPTH, %eax
   cmpl $8, %eax
   je stretch_normal_loop
   cmpl $15, %eax
   je stretch_bpp_16
   cmpl $16, %eax
   je stretch_bpp_16
   cmpl $24, %eax
   je stretch_bpp_24
   cmpl $32, %eax
   je stretch_bpp_32
   jmp stretch_done


   /* special loop for 24 bit */
   _align_
stretch_bpp_24:
   movl SX, %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, SX
   movl DX, %eax
   leal (%eax, %eax, 2), %eax
   movl %eax, DX

   _align_
stretch_loop24:
   movl SOURCE, %edx             /* get source line (in esi) and bank */
   movl SY, %eax
   shrl $16, %eax
   READ_BANK()
   movl %eax, %esi
   addl SX, %esi

   movl DEST, %edx               /* get dest line (in edi) and bank */
   movl DY, %eax
   WRITE_BANK()
   movl %eax, %edi
   addl DX, %edi
   pushl %edx
   pushl %ebx

   call *%ebx                    /* draw (clobbers eax, ebx, ecx, edx) */

   popl %ebx
   popl %edx
   movl SYD, %eax                /* next line in source bitmap */
   addl %eax, SY
   incl DY                       /* next line in dest bitmap */
   decl DH
   jg stretch_loop24
   jmp stretch_done


   /* special loop for mode-X */
   _align_
stretch_modex_loop:
   movl SOURCE, %edx             /* get source line (in esi) and bank */
   movl SY, %eax
   shrl $16, %eax
   movl BMP_LINE(%edx, %eax, 4), %esi
   addl SX, %esi

   movl DEST, %edx               /* get dest line (in edi) and bank */
   movl DY, %eax
   movl BMP_LINE(%edx, %eax, 4), %edi
   addl DX, %edi

   call *%ebx                    /* draw the line (clobbers eax and ecx) */

   movl SYD, %eax                /* next line in source bitmap */
   addl %eax, SY
   incl DY                       /* next line in dest bitmap */
   decl DH
   jg stretch_modex_loop
   jmp stretch_done


   _align_
stretch_bpp_16:
   shll $1, SX
   shll $1, DX
   jmp stretch_normal_loop

   _align_
stretch_bpp_32:
   shll $2, SX
   shll $2, DX


   /* normal stretching loop */
   _align_
stretch_normal_loop:
   movl SOURCE, %edx             /* get source line (in esi) and bank */
   movl SY, %eax
   shrl $16, %eax
   READ_BANK()
   movl %eax, %esi
   addl SX, %esi

   movl DEST, %edx               /* get dest line (in edi) and bank */
   movl DY, %eax
   WRITE_BANK()
   movl %eax, %edi
   addl DX, %edi

   call *%ebx                    /* draw the line (clobbers eax and ecx) */

   movl SYD, %eax                /* next line in source bitmap */
   addl %eax, SY
   incl DY                       /* next line in dest bitmap */
   decl DH
   jg stretch_normal_loop


stretch_done:
   popw %es

   movl SOURCE, %edx
   UNWRITE_BANK()

   movl DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _do_stretch() */




/* unsigned long _blender_trans24(unsigned long x, y, n);
 *  24 bit trans blender function. See colblend.c for the others.
 */
FUNC(_blender_trans24)
   pushl %ebp
   movl %esp, %ebp
   pushl %esi
   pushl %ecx
   pushl %ebx

   movl ARG1, %esi
   movl ARG2, %ebx
   movl ARG3, %ecx

   movl %esi, %eax
   movl %ebx, %edx
   andl $0xFF00FF, %eax
   andl $0xFF00FF, %edx

   orl %ecx, %ecx
   jz noinc

   incl %ecx

noinc:
   subl %edx, %eax
   imull %ecx, %eax
   shrl $8, %eax
   addl %ebx, %eax

   andl $0xFF00, %ebx
   andl $0xFF00, %esi

   subl %ebx, %esi
   imull %ecx, %esi
   shrl $8, %esi
   addl %ebx, %esi
   andl $0xFF00FF, %eax
   andl $0xFF00, %esi

   orl %esi, %eax

   popl %ebx
   popl %ecx
   popl %esi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _blender_trans24() */




/* fixed fixsqrt(fixed x);
 *  Fixed point square root routine. This code is based on the fixfloat
 *  library by Arne Steinarson.
 */
FUNC(fixsqrt)
   pushl %ebp
   movl %esp, %ebp

   /* This routine is based upon the following idea:
    *    sqrt (x) = sqrt (x/d) * sqrt(d)
    *    d = 2^(2n)
    *    sqrt (x) = sqrt (x / 2^(2n)) * 2^n
    * `x/2^(2n)' has to fall into the range 0..255 so that we can use the
    * square root lookup table. So `2n' is the number of bits `x' has to be
    * shifted to the left to become smaller than 256. The best way to find `2n'
    * is to do a reverse bit scan on `x'. This is achieved by the i386 ASM
    * instruction `bsr'.
    */

   movl ARG1, %eax               /* eax = `x' */
   orl %eax, %eax                /* check whether `x' is negative... */
   jle  sqrt_error_check         /* jump to error-checking if x <= 0 */

   movl %eax, %edx               /* bit-scan is done on edx */
   shrl $6, %edx
   xorl %ecx, %ecx               /* if no bit set: default %cl = 2n = 0 */
   bsrl %edx, %ecx 
   andb $0xFE, %cl               /* make result even -->  %cl = 2n */
   shrl %cl, %eax                /* shift x to fall into range 0..255 */

				 /* table lookup... */
   movzwl GLOBL(_sqrt_table)(,%eax,2), %eax

   shrb $1, %cl                  /* %cl = n */
   shll %cl, %eax                /* multiply `sqrt(x/2^(2n))' by `2^n' */
   shrl $4, %eax                 /* adjust the result */
   jmp sqrt_done

   _align_
sqrt_error_check:                /* here we go if x<=0 */
   jz sqrt_done                  /* if zero, return eax=0 */

   movl GLOBL(allegro_errno), %edx
   movl $ERANGE, (%edx)          /* on overflow, set errno */
   xorl %eax, %eax               /* return zero */

   _align_
sqrt_done:
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of fixsqrt() */




/* fixed fixhypot(fixed x, fixed y);
 *  Return fixed point sqrt (x*x+y*y), which is the length of the 
 *  hypotenuse of a right triangle with sides of length x and y, or the 
 *  distance of point (x|y) from the origin. This routine is faster and more 
 *  accurate than using the direct formula fixsqrt (fixmul (x,x), fixmul(y,y)). 
 *  It will also return correct results for x>=256 or y>=256 where fixmul(x) 
 *  or fixmul(y) would overflow.
 */
FUNC(fixhypot)
   pushl %ebp
   movl %esp, %ebp

   /* The idea of this routine is:
    *    sqrt (x^2+y^2) = sqrt ((x/d)^2+(y/d)^2) * d
    *    d = 2^n
    * Since `x' and `y' are fixed point numbers, they are multiplied in the 
    * following way:
    *    x^2 = (x*x)/2^16
    * so we come to the formula:
    *    sqrt(x^2+y^2) = sqrt((x*x + y*y)/2^(16+2n)) * 2^n
    * and this is almost the same problem as calculating the square root in
    * `fixsqrt': find `2n' so that `(x*x+y*y)/2^(16+2n)' is in the range 0..255
    * so that we can use the square root lookup table.
    */

   movl ARG1, %eax               /* edx:eax = x*x */
   imull %eax
   movl %eax, %ecx               /* save edx:eax */
   pushl %edx
   movl ARG2, %eax               /* edx:eax = y*y */
   imull %eax
   addl %ecx, %eax               /* edx:eax = x*x + y*y */
   popl %ecx
   adcl %ecx, %edx
   cmpl $0x3FFFFFFF, %edx        /* check for overflow */
   ja hypot_overflow

   /* And now we're doing a bit-scan on `x*x+y*y' to find out by how 
    * many bits it needs to be shifted to fall into the range 0..255. 
    * Since the intermediate result is 64 bit we may need two bitscans 
    * in case that no bit is set in the upper 32 bit.
    */ 
   bsrl %edx, %ecx
   jz hypot_part2

   /* we got the bit with the first step */
   incb %cl                      /* make cl even */
   incb %cl
   andb $0xFE, %cl 
   shrdl %cl, %edx, %eax         /* make eax fall into range 0..255 */
   shrl $24, %eax
				 /* eax = table lookup square root */
   movzwl GLOBL(_sqrt_table)(,%eax,2), %eax
   shrb $1, %cl                  /* adjust result... */
   shll %cl, %eax 
   jmp hypot_done

   /* we didn't get the bit with the first step -- so we make another
    * scan on the remaining bits in `eax' to get `2n'.
    */
   _align_
hypot_part2:
   shrl $16, %eax                /* eax = (x*x+y*y)/2^16 */
   movl %eax, %edx               /* edx is used for scanning */
   shrl $6, %edx 
   xorl %ecx, %ecx               /* default `2n' if no bit is set */
   bsrl %edx, %ecx
   andb $0xFE, %cl               /* make cl=2n even */
   shrl %cl, %eax                /* make eax fall into range 0..255 */
				 /* eax = table lookup square root */
   movzwl GLOBL(_sqrt_table)(,%eax,2), %eax
   shrb $1, %cl                  /* cl = n */
   shll %cl, %eax                /* adjust result... */
   shrl $4, %eax 
   jmp hypot_done

   _align_
hypot_overflow:                  /* overflow */
   movl GLOBL(allegro_errno), %eax
   movl $ERANGE, (%eax)          /* set errno */
   movl $0x7FFFFFFF, %eax        /* and return MAXINT */

   _align_
hypot_done:
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of fixhypot() */

