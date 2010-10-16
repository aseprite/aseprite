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
 *      32 bit bitmap blitting (written for speed, not readability :-)
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "blit.inc"

#ifdef ALLEGRO_COLOR32

.text



/* void _linear_clear_to_color32(BITMAP *bitmap, int color);
 *  Fills a linear bitmap with the specified color.
 */
FUNC(_linear_clear_to_color32)
   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushl %esi
   pushl %ebx
   pushl %es 

   movl ARG1, %edx               /* edx = bmp */
   movl BMP_CT(%edx), %ebx       /* line to start at */

   movl BMP_SEG(%edx), %eax      /* select segment */
   movl %eax, %es

   movl BMP_CR(%edx), %esi       /* width to clear */
   subl BMP_CL(%edx), %esi
   cld

   _align_
clear_loop:
   movl %ebx, %eax
   movl BMP_CL(%edx), %edi 
   WRITE_BANK()                  /* select bank */
   leal (%eax, %edi, 4), %edi    /* get line address  */

   movl ARG2, %eax 
   movl %esi, %ecx 
   rep ; stosl                   /* clear the line */

   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clear_loop                 /* and loop */

   popl %es

   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_clear_to_color() */




/* void _linear_blit32(BITMAP *source, BITMAP *dest, int source_x, source_y, 
 *                                     int dest_x, dest_y, int width, height);
 *  Normal forwards blitting routine for linear bitmaps.
 */
FUNC(_linear_blit32)
   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushl %esi
   pushl %ebx
   pushl %es

   movl B_DEST, %edx
   movl %ds, %ebx                /* save data segment selector */
   movl BMP_SEG(%edx), %eax      /* load destination segment */
   movl %eax, %es
   cld                           /* for forward copy */

   _align_
   BLIT_LOOP(blitter, 4,         /* copy the data */
      rep ; movsl
   )

   popl %es

   movl B_SOURCE, %edx
   UNREAD_BANK()

   movl B_DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_blit32() */




/* void _linear_blit_backward32(BITMAP *source, BITMAP *dest, int source_x, 
 *                      int source_y, int dest_x, dest_y, int width, height);
 *  Reverse blitting routine, for overlapping linear bitmaps.
 */
FUNC(_linear_blit_backward32)
   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushl %esi
   pushl %ebx
   pushl %es

   movl B_HEIGHT, %eax           /* y values go from high to low */
   decl %eax
   addl %eax, B_SOURCE_Y
   addl %eax, B_DEST_Y

   movl B_WIDTH, %eax            /* x values go from high to low */
   decl %eax
   addl %eax, B_SOURCE_X
   addl %eax, B_DEST_X

   movl B_DEST, %edx
   movl %ds, %ebx                /* save data segment selector */
   movl BMP_SEG(%edx), %edx      /* load destination segment */
   movl %edx, %es

   _align_
blit_backwards_loop:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   movl B_DEST_X, %edi           /* x offset */
   WRITE_BANK()                  /* select bank */
   leal (%eax, %edi, 4), %edi

   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   movl B_SOURCE_X, %esi         /* x offset */
   READ_BANK()                   /* select bank */
   leal (%eax, %esi, 4), %esi

   movl B_WIDTH, %ecx            /* x loop counter */
   movl BMP_SEG(%edx), %eax      /* load data segment */
   movl %eax, %ds
   std                           /* backwards */
   rep ; movsl                   /* copy the line */

   movl %ebx, %ds                /* restore data segment */
   decl B_SOURCE_Y
   decl B_DEST_Y
   decl B_HEIGHT
   jg blit_backwards_loop        /* and loop */

   cld                           /* finished */

   popl %es

   movl B_SOURCE, %edx
   UNREAD_BANK()

   movl B_DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_blit_backward32() */

FUNC(_linear_blit32_end)
   ret




/* void _linear_masked_blit32(BITMAP *source, *dest, int source_x, source_y, 
 *                            int dest_x, dest_y, int width, height);
 *  Masked (skipping pink pixels) blitting routine for linear bitmaps.
 */
FUNC(_linear_masked_blit32)
   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushl %esi
   pushl %ebx
   pushl %es

   movl B_DEST, %edx
   movl %ds, %ebx 
   movl BMP_SEG(%edx), %eax
   movl %eax, %es
   cld 

#ifdef ALLEGRO_SSE  /* Use SSE if the compiler supports it */
      
   /* Speed improvement on the Pentium 3 only, so we need to check for MMX+ and no 3DNow! */
   movl GLOBL(cpu_capabilities), %ecx     /* if MMX+ is enabled (or not disabled :) */
   andl $CPU_MMXPLUS | $CPU_3DNOW, %ecx
   cmpl $CPU_MMXPLUS, %ecx
   jne masked32_no_mmx


   movl B_WIDTH, %ecx
   shrl $2, %ecx                   /* Are there more than 4 pixels? Otherwise, use non-MMX code */
   jz masked32_no_mmx
   
   movl $MASK_COLOR_32, %eax
   movd %eax, %mm0                /* Create mask (%mm0) */
   movd %eax, %mm1
   psllq $32, %mm0
   por  %mm1, %mm0
   
   pcmpeqd %mm4, %mm4             /* Create inverter mask */
  
   /* ??? maskmovq is an SSE instruction! */

   #define BLIT_CODE                                                          \
      movd %ecx, %mm2;            /* Save line length (%mm2) */               \
      shrl $2, %ecx;                                                          \
                                                                              \
      pushl %es;  /* Swap ES and DS */                                        \
      pushl %ds;                                                              \
      popl  %es;                                                              \
      popl  %ds;                                                              \
                                                                              \
      _align_;                                                                \
      masked32_mmx_x_loop:                                                    \
                                                                              \
      movq %es:(%esi), %mm1;      /* Read 4 pixels */                         \
      movq %mm0, %mm3;                                                        \
      movq %es:8(%esi), %mm5;     /* Read 4 more pixels */                    \
      movq %mm0, %mm6;                                                        \
                                                                              \
      pcmpeqd %mm1, %mm3;         /* Compare with mask (%mm3/%mm6) */         \
      pcmpeqd %mm5, %mm6;                                                     \
      pxor %mm4, %mm3;            /* Turn 1->0 and 0->1 */                    \
      pxor %mm4, %mm6;                                                        \
      addl $16, %esi;             /* Update src */                            \
      maskmovq %mm3, %mm1;        /* Write if not equal to mask. */           \
      addl $8, %edi;                                                          \
      maskmovq %mm6, %mm5;                                                    \
                                                                              \
      addl $8, %edi;              /* Update dest */                           \
                                                                              \
      decl %ecx;                  /* Any pixel packs left for this line? */   \
      jnz masked32_mmx_x_loop;                                                \
                                                                              \
      movd %mm2, %ecx;            /* Restore pixel count */                   \
      movd %mm0, %edx;                                                        \
      andl $3, %ecx;                                                          \
      jz masked32_mmx_loop_end;   /* Nothing else to do? */                   \
      shrl $1, %ecx;              /* 1 pixels left */                         \
      jnc masked32_mmx_qword;                                                 \
                                                                              \
      movl %es:(%esi), %eax;      /* Read 1 pixel */                          \
      addl $4, %esi;                                                          \
      addl $4, %edi;                                                          \
      cmpl %eax, %edx;            /* Compare with mask */                     \
      je masked32_mmx_qword;                                                  \
      movl %eax, -4(%edi);        /* Write the pixel */                       \
                                                                              \
      _align_;                                                                \
      masked32_mmx_qword:                                                     \
                                                                              \
      shrl $1, %ecx;              /* 2 pixels left */                         \
      jnc masked32_mmx_loop_end;                                              \
                                                                              \
      movq %es:(%esi), %mm1;      /* Read 2 more pixels */                    \
      movq %mm0, %mm3;                                                        \
                                                                              \
      pcmpeqd %mm1, %mm3;         /* Compare with mask (%mm3, %mm6) */        \
      pxor %mm4, %mm3;            /* Turn 1->0 and 0->1 */                    \
      maskmovq %mm3, %mm1;        /* Write if not equal to mask. */           \
                                                                              \
      _align_;                                                                \
      masked32_mmx_loop_end:                                                  \
                                                                              \
      pushl %ds;                  /* Swap back ES and DS */                   \
      popl  %es;
   BLIT_LOOP(masked32_mmx_loop, 4, BLIT_CODE)
   #undef BLIT_CODE
   emms
   
   jmp masked32_end;
   
#endif
   
	_align_
	masked32_no_mmx:


   _align_
   #define BLIT_CODE                                      \
      _align_ ;                                           \
   masked_blit_x_loop:                                    \
      movl (%esi), %eax ;        /* read a byte */        \
      addl $4, %esi ;                                     \
                                                          \
      cmpl $MASK_COLOR_32, %eax ;/* test it */            \
      je masked_blit_skip ;                               \
                                                          \
      movl %eax, %es:(%edi) ;    /* write the pixel */    \
      addl $4, %edi ;                                     \
      decl %ecx ;                                         \
      jg masked_blit_x_loop ;                             \
      jmp masked_blit_x_loop_done ;                       \
                                                          \
      _align_ ;                                           \
   masked_blit_skip:                                      \
      addl $4, %edi ;            /* skip zero pixels */   \
      decl %ecx ;                                         \
      jg masked_blit_x_loop ;                             \
                                                          \
   masked_blit_x_loop_done:
   BLIT_LOOP(masked, 4, BLIT_CODE)
   #undef BLIT_CODE

   masked32_end:

   popl %es

   /* the source must be a memory bitmap, no need for
    *  movl B_SOURCE, %edx
    *  UNREAD_BANK()
    */

   movl B_DEST, %edx
   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_masked_blit32() */




#endif      /* ifdef ALLEGRO_COLOR32 */
