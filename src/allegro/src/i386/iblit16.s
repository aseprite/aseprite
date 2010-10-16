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
 *      16 bit bitmap blitting (written for speed, not readability :-)
 *
 *      By Shawn Hargreaves.
 *
 *      MMX clear code by Robert Ohannessian.
 *
 *      Blitting and masked blitting optimised by Jose Antonio Luque.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "blit.inc"

#ifdef ALLEGRO_COLOR16

.text



/* void _linear_clear_to_color16(BITMAP *bitmap, int color);
 *  Fills a linear bitmap with the specified color.
 */
FUNC(_linear_clear_to_color16)
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

#ifdef ALLEGRO_MMX               /* only use MMX if compiler supports it */

   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz clear_no_mmx

   movl %esi, %eax               /* If there are less than 16 pixels to clear, then we use the non-MMX version */
   shrl $4, %eax
   orl %eax, %eax
   jz clear_no_mmx

   movl ARG2, %eax               /* duplicate color twice */
   movl %eax, %ecx
   shll $16, %eax
   andl $0xFFFF, %ecx
   orl %ecx, %eax

   pushl %eax

   movl %ds, %eax
   movl %es, %ecx

   cmpw %ax, %cx                 /* can we use nearptr ? */
   jne clearMMXseg_loop          /* if not, then we have to decode segments...*/
				 /* else, we save one cycle per 4 pixels on PMMX/K6 */
   _align_
clearMMX_loop:
   movl %ebx, %eax
   movl BMP_CL(%edx), %edi
   WRITE_BANK()                  /* select bank */
   leal (%eax, %edi, 2), %edi    /* get line address  */

   popl %eax                     /* get eax back */

   movl %esi, %ecx               /* width to clear */

   movd %eax, %mm0               /* restore mmx reg 0 in case it's been clobbered by WRITE_BANK() */
   movd %eax, %mm1
   psllq $32, %mm0
   por %mm1, %mm0

   pushl %eax                    /* save eax */

   testl $7, %edi                /* is destination aligned on 64-bit ? */
   jz clearMMX_aligned

clearMMX_do_alignment:
   movl %edi, %eax               /* we want to adjust %ecx  (pairing: see andl) */

   movq %mm0, (%edi)             /* we clear 4 pixels */

   andl $7, %eax                 /* we calc how may pixels we actually wanted to clear (8 - %eax) (see subl) */

   andl $0xFFFFFFF8, %edi        /* instruction pairing (see inc %edi) */

   shrl $1, %eax
   subl $4, %eax

   addl $8, %edi                 /* we set %edi to the next aligned memory address */

   addl %eax, %ecx               /* and adjust %ecx to reflect the change */

clearMMX_aligned:
   movl %ecx, %eax               /* save for later */
   shrl $4, %ecx                 /* divide by 16 for 4 * 8-byte memory move */
   jz clearMMX_finish_line       /* if there's less than 16 pixels to clear, no need for MMX */

clearMMX_continue_line:
   movq %mm0, (%edi)             /* move 4x 8 bytes */
   movq %mm0, 8(%edi)            /* MMX instructions can't pair when both write to memory */
   movq %mm0, 16(%edi)
   movq %mm0, 24(%edi)
   addl $32, %edi                /* inserting those in the MMX copy block makes no diffrence */
   decl %ecx
   jnz clearMMX_continue_line

clearMMX_finish_line:
   movl %eax, %ecx               /* get ecx back */

   testl $15, %ecx               /* check if there's any left */
   jz clearMMX_no_long
				 /* else, write trailing pixels */
   testl $8, %ecx 
   jz clearMMX_finish_line2

   movq %mm0, (%edi)
   movq %mm0, 8(%edi)
   addl $16, %edi 

clearMMX_finish_line2:
   testl $4, %ecx
   jz clearMMX_finish_line3

   movq %mm0, (%edi)
   addl $8, %edi

clearMMX_finish_line3:
   andl $3, %ecx
   subl $4, %ecx
   shll $1, %ecx

   movq %mm0, (%edi, %ecx)

clearMMX_no_long:
   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clearMMX_loop              /* and loop */

   popl %eax

   emms                          /* clear FPU tag word */

   jmp clear_done

clearMMXseg_loop:
   movl %ebx, %eax
   movl BMP_CL(%edx), %edi
   WRITE_BANK()                  /* select bank */
   leal (%eax, %edi, 2), %edi    /* get line address  */

   popl %eax                     /* get eax back */

   movl %esi, %ecx               /* width to clear */

   movd %eax, %mm0               /* restore mmx reg 0 in case it's been clobbered by WRITE_BANK() */
   movd %eax, %mm1
   psllq $32, %mm0
   por %mm1, %mm0

   pushl %eax                    /* save eax */

   testl $7, %edi                /* is destination aligned on 64-bit ? */
   jz clearMMXseg_aligned

clearMMXseg_do_alignment:
   movl %edi, %eax               /* we want to adjust %ecx  (pairing: see andl) */

   movq %mm0, %es:(%edi)         /* we clear 4 pixels */

   andl $7, %eax                 /* we calc how may pixels we actually wanted to clear (8 - %eax) (see subl) */

   andl $0xFFFFFFF8, %edi        /* instruction pairing (see inc %edi) */

   shrl $1, %eax
   subl $4, %eax

   addl $8, %edi                 /* we set %edi to the next aligned memory address */

   addl %eax, %ecx               /* and adjust %ecx to reflect the change */

clearMMXseg_aligned:
   movl %ecx, %eax               /* save for later */
   shrl $4, %ecx                 /* divide by 16 for 4 * 8-byte memory move */
   jz clearMMXseg_finish_line    /* if there's less than 16 pixels to clear, no need for MMX */

clearMMXseg_continue_line:
   movq %mm0, %es:(%edi)
   movq %mm0, %es:8(%edi)
   movq %mm0, %es:16(%edi)
   movq %mm0, %es:24(%edi)
   addl $32, %edi
   decl %ecx
   jnz clearMMXseg_continue_line

clearMMXseg_finish_line:
   movl %eax, %ecx

   testl $15, %ecx               /* check if there's any left */
   jz clearMMXseg_no_long

				 /* else, write trailing pixels */
   testl $8, %ecx
   jz clearMMXseg_finish_line2

   movq %mm0, %es:(%edi)
   movq %mm0, %es:8(%edi)
   addl $16, %edi

clearMMXseg_finish_line2: 
   testl $4, %ecx
   jz clearMMXseg_finish_line3

   movq %mm0, %es:(%edi)
   addl $8, %edi

clearMMXseg_finish_line3:
   andl $3, %ecx
   subl $4, %ecx
   shll $1, %ecx

   movq %mm0, %es:(%edi, %ecx)

clearMMXseg_no_long:
   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clearMMXseg_loop           /* and loop */

   popl %eax

   emms                          /* clear FPU tag word */

   jmp clear_done

#endif                           /* ALLEGRO_MMX */

clear_no_mmx:                    /* If no MMX is available, use the non-MMX version */
   cld

   _align_
clear_loop:
   movl %ebx, %eax
   movl BMP_CL(%edx), %edi 
   WRITE_BANK()                  /* select bank */
   leal (%eax, %edi, 2), %edi    /* get line address  */

   movw ARG2, %ax                /* duplicate color twice */
   shll $16, %eax
   movw ARG2, %ax 

   movl %esi, %ecx               /* width to clear */
   shrl $1, %ecx                 /* halve for 32 bit clear */
   jnc clear_no_word
   stosw                         /* clear an odd word */

clear_no_word:
   jz clear_no_long 

   rep ; stosl                   /* clear the line */

clear_no_long:
   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clear_loop                 /* and loop */

clear_done:
   popl %es

   UNWRITE_BANK()

   popl %ebx
   popl %esi
   popl %edi
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _linear_clear_to_color16() */




/* void _linear_blit16(BITMAP *source, BITMAP *dest, int source_x, source_y, 
 *                                     int dest_x, dest_y, int width, height);
 *  Normal forwards blitting routine for linear bitmaps.
 */
FUNC(_linear_blit16)
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

#ifdef ALLEGRO_MMX               /* only use MMX if the compiler supports it */

   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz blit_no_mmx

   shrl $1, B_WIDTH              /* divide for use longs */
   jz blit_only_one_word         /* blit only one word */
   jnc blit_longsmmx
   shrl $1, B_WIDTH              /* divide for use longs64 */
   jz blit_long_word             /* blit one long and word */
   jnc blit_even_wmmxlongs       /* blit longs64 and word */
   jmp blit_mmxlong_word         /* blit longs64 and long and word */
blit_longsmmx:
   shrl $1, B_WIDTH              /* divide for use longs64 */
   jz blit_only_one_long         /* blit only one long */
   jnc blit_even_mmxlongs        /* blit longs64 */

   _align_
blit_mmxlong_long:               /* blit longs64 and long */
   #define BLIT_CODE              \
   even_llmmx_loop:               \
      movq %ds:(%esi), %mm0 ;     \
      addl $8, %esi ;             \
      movq %mm0, %es:(%edi) ;     \
      addl $8, %edi ;             \
      decl %ecx ;                 \
      jnz even_llmmx_loop ;       \
      movsl
   BLIT_LOOP(long_longsmmx, 2, BLIT_CODE)
   #undef BLIT_CODE
   emms
   jmp blit_done

   _align_
blit_mmxlong_word:
   #define BLIT_CODE              \
   even_wlmmx_loop:               \
      movq %ds:(%esi), %mm0 ;     \
      addl $8, %esi ;             \
      movq %mm0, %es:(%edi) ;     \
      addl $8, %edi ;             \
      decl %ecx ;                 \
      jnz even_wlmmx_loop ;       \
      movsl ;                     \
      movsw
   BLIT_LOOP(word_longsmmx, 2, BLIT_CODE)
   #undef BLIT_CODE
   emms
   jmp blit_done

   _align_
blit_even_wmmxlongs:
   #define BLIT_CODE              \
   even_wmmx_loop:                \
      movq %ds:(%esi), %mm0 ;     \
      addl $8, %esi ;             \
      movq %mm0, %es:(%edi) ;     \
      addl $8, %edi ;             \
      decl %ecx ;                 \
      jnz even_wmmx_loop ;        \
      movsw
   BLIT_LOOP(word_wlongsmmx, 2, BLIT_CODE)
   #undef BLIT_CODE
   emms
   jmp blit_done

   _align_
blit_even_mmxlongs:
   #define BLIT_CODE              \
   even_lmmx_loop:                \
      movq %ds:(%esi), %mm0 ;     \
      addl $8, %esi ;             \
      movq %mm0, %es:(%edi) ;     \
      addl $8, %edi ;             \
      decl %ecx ;                 \
      jnz even_lmmx_loop
   BLIT_LOOP(even_longsmmx, 2, BLIT_CODE)
   #undef BLIT_CODE
   emms
   jmp blit_done

   _align_
blit_long_word:
   #define BLIT_CODE              \
      movsl ;                     \
      movsw
   BLIT_LOOP(long_word, 2, BLIT_CODE)
   #undef BLIT_CODE
   emms
   jmp blit_done

   _align_
blit_only_one_long:
   BLIT_LOOP(only_one_wordmmx, 2, movsl)
   emms
   jmp blit_done

#endif                           /* ALLEGRO_MMX */

blit_no_mmx:
   cld                           /* for forward copy */

   shrl $1, B_WIDTH              /* halve counter for long copies */
   jz blit_only_one_word
   jnc blit_even_words

   _align_
   #define BLIT_CODE              \
      rep ; movsl ;               \
      movsw
   BLIT_LOOP(longs_and_word, 2, BLIT_CODE)  /* long at a time, plus leftover word */
   #undef BLIT_CODE
   jmp blit_done

   _align_
blit_even_words:
   #define BLIT_CODE              \
      rep ; movsl
   BLIT_LOOP(even_words, 2, BLIT_CODE)  /* copy a long at a time */
   #undef BLIT_CODE
   jmp blit_done

   _align_
blit_only_one_word: 
   BLIT_LOOP(only_one_word, 2, movsw)  /* copy just the one word */

   _align_
blit_done:
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
   ret                           /* end of _linear_blit16() */




/* void _linear_blit_backward16(BITMAP *source, BITMAP *dest, int source_x, 
 *                      int source_y, int dest_x, dest_y, int width, height);
 *  Reverse blitting routine, for overlapping linear bitmaps.
 */
FUNC(_linear_blit_backward16)
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
   movl BMP_SEG(%edx), %eax      /* load destination segment */
   movl %eax, %es

   _align_
blit_backwards_loop:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   movl B_DEST_X, %edi           /* x offset */
   WRITE_BANK()                  /* select bank */
   leal (%eax, %edi, 2), %edi

   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   movl B_SOURCE_X, %esi         /* x offset */
   READ_BANK()                   /* select bank */
   leal (%eax, %esi, 2), %esi

   movl B_WIDTH, %ecx            /* x loop counter */
   movl BMP_SEG(%edx), %edx      /* load data segment */
   movl %edx, %ds
   std                           /* backwards */
   rep ; movsw                   /* copy the line */

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
   ret                           /* end of _linear_blit_backward16() */

FUNC(_linear_blit16_end)
   ret




/* void _linear_masked_blit16(BITMAP *source, *dest, int source_x, source_y, 
 *                            int dest_x, dest_y, int width, height);
 *  Masked (skipping zero pixels) blitting routine for linear bitmaps.
 */
FUNC(_linear_masked_blit16)
   pushl %ebp
   movl %esp, %ebp
   subl $4, %esp                 /* one local variable */
   pushl %edi
   pushl %esi
   pushl %ebx
   pushl %es

   #define V_MASK    -4(%ebp)

   movl B_DEST, %edx
   movl %ds, %ebx 
   movl BMP_SEG(%edx), %edx
   movl %edx, %es
   cld 

   movl B_SOURCE, %edx
   movl BMP_VTABLE(%edx), %edx
   movl VTABLE_MASK_COLOR(%edx), %eax
   movl %eax, %ecx
   shll $16, %eax
   orw %cx, %ax
   movl %eax, V_MASK

   
#ifdef ALLEGRO_SSE  /* Use SSE if the compiler supports it */
      
   /* Speed improvement on the Pentium 3 only, so we need to check for MMX+ and no 3DNow! */
   movl GLOBL(cpu_capabilities), %ecx     /* if MMX+ is enabled (or not disabled :) */
   andl $CPU_MMXPLUS | $CPU_3DNOW, %ecx
   cmpl $CPU_MMXPLUS, %ecx
   jne masked16_no_mmx


   movl B_WIDTH, %ecx
   shrl $3, %ecx                 /* Are there more than 8 pixels? Otherwise, use non-MMX code */
   jz masked16_no_mmx
   
   movd V_MASK, %mm0             /* Create mask (%mm0) */
   movd V_MASK, %mm1
   psllq $32, %mm0
   por  %mm1, %mm0
   
   pcmpeqd %mm4, %mm4            /* Create inverter mask */

   /* ??? maskmovq is an SSE instruction! */

   #define BLIT_CODE                                                          \
      movd %ecx, %mm2;            /* Save line length (%mm2) */               \
      shrl $3, %ecx;                                                          \
      movl V_MASK, %edx;          /* Save 32 bit mask */                      \
                                                                              \
      pushl %es;  /* Swap ES and DS */                                        \
      pushl %ds;                                                              \
      popl  %es;                                                              \
      popl  %ds;                                                              \
                                                                              \
      _align_;                                                                \
      masked16_mmx_x_loop:                                                    \
                                                                              \
      movq %es:(%esi), %mm1;       /* Read 4 pixels */                        \
      movq %mm0, %mm3;                                                        \
      movq %es:8(%esi), %mm5;      /* Read 4 more pixels */                   \
      movq %mm0, %mm6;                                                        \
                                                                              \
      pcmpeqw %mm1, %mm3;         /* Compare with mask (%mm3/%mm6) */         \
      pcmpeqw %mm5, %mm6;                                                     \
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
      jnz masked16_mmx_x_loop;                                                \
                                                                              \
      movd %mm2, %ecx;            /* Restore pixel count */                   \
      andl $7, %ecx;                                                          \
      jz masked16_mmx_loop_end;   /* Nothing else to do? */                   \
      shrl $1, %ecx;              /* 1 pixels left */                         \
      jnc masked16_mmx_long;                                                  \
                                                                              \
      movw %es:(%esi), %ax;       /* Read 1 pixel */                          \
      addl $2, %esi;                                                          \
      addl $2, %edi;                                                          \
      cmpw %ax, %dx;              /* Compare with mask */                     \
      je masked16_mmx_long;                                                   \
      movw %ax, -2(%edi);         /* Write the pixel */                       \
                                                                              \
      masked16_mmx_long:                                                      \
                                                                              \
      shrl $1, %ecx;              /* 2 pixels left */                         \
      jnc masked16_mmx_qword;                                                 \
                                                                              \
      movl %es:(%esi), %eax;      /* Read 2 pixels */                         \
      addl $4, %esi;                                                          \
      addl $4, %edi;                                                          \
      cmpw %ax, %dx;              /* Compare with mask */                     \
      je masked16_mmx_long_2;                                                 \
      movw %ax, -4(%edi);         /* Write pixel */                           \
                                                                              \
      masked16_mmx_long_2:                                                    \
      shrl $16, %eax;                                                         \
      shrl $16, %edx;                                                         \
      cmpl %eax, %edx;                                                        \
      je masked16_mmx_qword;                                                  \
      movw %ax, -2(%edi);                                                     \
                                                                              \
      _align_;                                                                \
      masked16_mmx_qword:                                                     \
      shrl $1, %ecx;              /* 4 pixels left */                         \
      jnc masked16_mmx_loop_end;                                              \
                                                                              \
      movq %es:(%esi), %mm1;      /* Read 4 more pixels */                    \
      movq %mm0, %mm3;                                                        \
                                                                              \
      pcmpeqw %mm1, %mm3;         /* Compare with mask (%mm3, %mm6) */        \
      pxor %mm4, %mm3;            /* Turn 1->0 and 0->1 */                    \
      maskmovq %mm3, %mm1;        /* Write if not equal to mask. */           \
                                                                              \
      _align_;                                                                \
      masked16_mmx_loop_end:                                                  \
                                                                              \
      pushl %ds;                  /* Swap back ES and DS */                   \
      popl  %es;
   BLIT_LOOP(masked16_mmx_loop, 2, BLIT_CODE)
   #undef BLIT_CODE

   emms
   
   jmp masked16_end;
   
#endif
   
	_align_
	masked16_no_mmx:

   #define BLIT_CODE                                                   \
       movl V_MASK, %edx ;                                             \
                                                                       \
      test $1, %ecx ;             /* 32 bit aligned->use new code */   \
      jz masked32_blit_x_loop ;                                        \
      movw (%esi), %ax ;          /* read a pixel */                   \
      cmpw %ax, %dx ;             /* test it */                        \
      je masked16_blit_skip ;                                          \
      movw %ax, %es:(%edi) ;      /* write the pixel */                \
   masked16_blit_skip:                                                 \
      decl %ecx ;                                                      \
      jng masked32_blit_end ;                                          \
      addl $2, %esi ;                                                  \
      addl $2, %edi ;                                                  \
                                                                       \
      _align_ ;                                                        \
   masked32_blit_x_loop:                                               \
      movl (%esi), %eax ;        /* read two pixels */                 \
      addl $4, %esi ;                                                  \
      cmpl %eax, %edx ;          /* test it */                         \
      je masked32_blit_skip2 ;                                         \
      cmpw %ax, %dx ;            /* test it */                         \
      je masked32_blit_skip1 ;                                         \
      movw %ax, %es:(%edi) ;     /* write the pixel */                 \
   masked32_blit_skip1:                                                \
      shrl $16, %eax ;                                                 \
      cmpw %ax, %dx ;            /* test it */                         \
      je masked32_blit_skip2 ;                                         \
      movw %ax, %es:2(%edi) ;    /* write the pixel */                 \
   masked32_blit_skip2:                                                \
      addl $4, %edi ;                                                  \
      subl $2, %ecx ;                                                  \
      jg masked32_blit_x_loop ;                                        \
   masked32_blit_end:
   BLIT_LOOP(masked32, 2, BLIT_CODE)
   #undef BLIT_CODE

masked16_end:

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
   ret                           /* end of _linear_masked_blit16() */




#endif      /* ifdef ALLEGRO_COLOR16 */
