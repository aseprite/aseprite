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
 *      256 color bitmap blitting (written for speed, not readability :-)
 *
 *      By Shawn Hargreaves.
 *
 *      Stefan Schimanski optimised the reverse blitting function.
 *
 *      MMX clear code by Robert Ohannessian.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "blit.inc"

#ifdef ALLEGRO_COLOR8

.text



/* void _linear_clear_to_color8(BITMAP *bitmap, int color);
 *  Fills a linear bitmap with the specified color.
 */
FUNC(_linear_clear_to_color8)
   pushl %ebp
   movl %esp, %ebp
   pushl %edi
   pushl %esi
   pushl %ebx
   movl ARG1, %edx               /* edx = bmp */
   pushl %es 

   movl BMP_SEG(%edx), %eax      /* select segment */
   movl %eax, %es

   movl BMP_CT(%edx), %ebx       /* line to start at */

   movl BMP_CR(%edx), %esi       /* width to clear */
   subl BMP_CL(%edx), %esi

#ifdef ALLEGRO_MMX               /* only use MMX if compiler supports it */

   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz clear_no_mmx

   movl %esi, %eax               /* if less than 32 pixels, use non-MMX */
   shrl $5, %eax
   orl %eax, %eax
   jz clear_no_mmx

   movb ARG2, %al                /* duplicate color 4 times */
   movb %al, %ah
   shll $16, %eax
   movb ARG2, %al 
   movb %al, %ah

   pushl %eax

   movl %ds, %eax
   movl %es, %ecx

   cmpw %ax, %cx                 /* can we use nearptr ? */
   jne clearMMXseg_loop          /* if not, then we have to decode segments...*/
				 /* else, we save one cycle per 8 pixels on PMMX/K6 */
   _align_
clearMMX_loop:
   movl %ebx, %eax
   WRITE_BANK()                  /* select bank */
   movl %eax, %edi 
   addl BMP_CL(%edx), %edi       /* get line address  */

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

   movq %mm0, (%edi)             /* we clear 8 pixels */

   andl $7, %eax                 /* we calc how may pixels we actually wanted to clear (8 - %eax) (see subl) */

   andl $0xFFFFFFF8, %edi        /* instruction pairing (see inc %edi) */

   subl $8, %eax

   addl $8, %edi                 /* we set %edi to the next aligned memory address */

   addl %eax, %ecx               /* and adjust %ecx to reflect the change */

clearMMX_aligned:
   movl %ecx, %eax               /* save for later */
   shrl $5, %ecx                 /* divide by 32 for 4 * 8-byte memory move */
   jz clearMMX_finish_line       /* if there's less than 32 pixels to clear, no need for MMX */

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

   testl $31, %ecx               /* check if there's any left */
   jz clearMMX_no_long
				 /* else, write trailing pixels */
   testl $16, %ecx
   jz clearMMX_finish_line2

   movq %mm0, (%edi)
   movq %mm0, 8(%edi)
   addl $16, %edi 

clearMMX_finish_line2:
   testl $8, %ecx
   jz clearMMX_finish_line3

   movq %mm0, (%edi)
   addl $8, %edi

clearMMX_finish_line3:
   andl $7, %ecx
   subl $8, %ecx

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
   WRITE_BANK()                  /* select bank */
   movl %eax, %edi 
   addl BMP_CL(%edx), %edi       /* get line address  */

   popl %eax                     /* Get eax back */

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

   movq %mm0, %es:(%edi)         /* we clear 8 pixels */

   andl $7, %eax                 /* we calc how may pixels we actually wanted to clear (8 - %eax) (see subl) */

   andl $0xFFFFFFF8, %edi        /* instruction pairing (see inc %edi) */

   subl $8, %eax

   addl $8, %edi                 /* we set %edi to the next aligned memory address */

   addl %eax, %ecx               /* and adjust %ecx to reflect the change */

clearMMXseg_aligned:
   movl %ecx, %eax               /* save for later */
   shrl $5, %ecx                 /* divide by 32 for 4 * 8-byte memory move */
   jz clearMMXseg_finish_line    /* if there's less than 32 pixels to clear, no need for MMX */

clearMMXseg_continue_line:
   movq %mm0, %es:(%edi)         /* move 4x 8 bytes */
   movq %mm0, %es:8(%edi)        /* MMX instructions can't pair when both write to memory */
   movq %mm0, %es:16(%edi)
   movq %mm0, %es:24(%edi)
   addl $32, %edi                /* inserting those in the MMX copy block makes no diffrence */
   decl %ecx
   jnz clearMMXseg_continue_line

clearMMXseg_finish_line:
   movl %eax, %ecx               /* get ecx back */

   testl $31, %ecx               /* check if there's any left */
   jz clearMMXseg_no_long
				 /* else, write trailing pixels */
   testl $16, %ecx 
   jz clearMMXseg_finish_line2

   movq %mm0, %es:(%edi)
   movq %mm0, %es:8(%edi)
   addl $16, %edi 

clearMMXseg_finish_line2:
   testl $8, %ecx
   jz clearMMXseg_finish_line3

   movq %mm0, %es:(%edi)
   addl $8, %edi

clearMMXseg_finish_line3:
   andl $7, %ecx
   subl $8, %ecx

   movq %mm0, %es:(%edi, %ecx)

clearMMXseg_no_long:
   incl %ebx
   cmpl %ebx, BMP_CB(%edx)
   jg clearMMXseg_loop           /* and loop */

   popl %eax

   emms                          /* clear FPU tag word */

   jmp clear_done

#endif                           /* ALLEGRO_MMX */

clear_no_mmx:
   cld

   _align_
clear_loop:
   movl %ebx, %eax
   WRITE_BANK()                  /* select bank */
   movl %eax, %edi 
   addl BMP_CL(%edx), %edi       /* get line address  */

   movb ARG2, %al                /* duplicate color 4 times */
   movb %al, %ah
   shll $16, %eax
   movb ARG2, %al 
   movb %al, %ah

   movl %esi, %ecx               /* width to clear */
   shrl $1, %ecx                 /* halve for 16 bit clear */
   jnc clear_no_byte
   stosb                         /* clear an odd byte */

clear_no_byte:
   shrl $1, %ecx                 /* halve again for 32 bit clear */
   jnc clear_no_word
   stosw                         /* clear an odd word */

clear_no_word:
   jz clear_no_long 

   _align_
clear_x_loop:
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
   ret                           /* end of _linear_clear_to_color8() */




/* void _linear_blit8(BITMAP *source, BITMAP *dest, int source_x, source_y, 
 *                                    int dest_x, dest_y, int width, height);
 *  Normal forwards blitting routine for linear bitmaps.
 */
FUNC(_linear_blit8)
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

   shrl $1, B_WIDTH              /* halve counter for word copies */
   jz blit_only_one_byte
   jnc blit_even_bytes

   _align_
   BLIT_LOOP(words_and_byte, 1,  /* word at a time, plus leftover byte */
      rep ; movsw ;
      movsb
   )
   jmp blit_done

   _align_
blit_even_bytes: 
   shrl $1, B_WIDTH              /* halve counter again, for long copies */
   jz blit_only_one_word
   jnc blit_even_words

   _align_
   BLIT_LOOP(longs_and_word, 1,  /* long at a time, plus leftover word */
      rep ; movsl ;
      movsw
   )
   jmp blit_done

   _align_
blit_even_words: 
   BLIT_LOOP(even_words, 1,      /* copy a long at a time */
      rep ; movsl ;
   )
   jmp blit_done

   _align_
blit_only_one_byte: 
   BLIT_LOOP(only_one_byte, 1,   /* copy just the one byte */
      movsb
   )
   jmp blit_done

   _align_
blit_only_one_word: 
   BLIT_LOOP(only_one_word, 1,   /* copy just the one word */
      movsw
   )

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
   ret                           /* end of _linear_blit8() */




/* void _linear_blit_backward8(BITMAP *source, BITMAP *dest, int source_x, 
 *                      int source_y, int dest_x, dest_y, int width, height);
 *  Reverse blitting routine, for overlapping linear bitmaps.
 */
FUNC(_linear_blit_backward8)
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

   movl B_SOURCE_Y, %eax         /* if different line -> fast dword blit */
   cmpl B_DEST_Y, %eax
   jne blit_backwards_loop_fast

   movl B_SOURCE_X, %eax         /* B_SOURCE_X-B_DEST_X */
   subl B_DEST_X, %eax
   cmpl $3, %eax                 /* if greater than 3 -> fast dword blit */
   jg blit_backwards_loop_fast

   _align_
blit_backwards_loop_slow:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   movl B_DEST_X, %edi           /* x offset */
   WRITE_BANK()                  /* select bank */
   addl %eax, %edi

   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   movl B_SOURCE_X, %esi         /* x offset */
   READ_BANK()                   /* select bank */
   addl %eax, %esi

   std                           /* backwards */
   movl B_WIDTH, %ecx            /* x loop counter */
   movl BMP_SEG(%edx), %eax      /* load data segment */
   movl %eax, %ds
   rep ; movsb                   /* copy the line */

   movl %ebx, %ds                /* restore data segment */
   decl B_SOURCE_Y
   decl B_DEST_Y
   decl B_HEIGHT
   jg blit_backwards_loop_slow   /* and loop */

   jmp blit_backwards_end

   _align_
blit_backwards_loop_fast:
   movl B_DEST, %edx             /* destination bitmap */
   movl B_DEST_Y, %eax           /* line number */
   movl B_DEST_X, %edi           /* x offset */
   WRITE_BANK()                  /* select bank */
   addl %eax, %edi

   movl B_SOURCE, %edx           /* source bitmap */
   movl B_SOURCE_Y, %eax         /* line number */
   movl B_SOURCE_X, %esi         /* x offset */
   READ_BANK()                   /* select bank */
   addl %eax, %esi

   std                           /* backwards */
   movl B_WIDTH, %eax            /* x loop counter */
   movl BMP_SEG(%edx), %edx      /* load data segment */
   movl %edx, %ds

   movl %eax, %ecx
   andl $3, %ecx                 /* copy bytes */
   rep ; movsb                   /* copy the line */

   subl $3, %esi
   subl $3, %edi

   movl %eax, %ecx
   shrl $2, %ecx                 /* copy dwords */
   rep ; movsl                   /* copy the line */

   movl %ebx, %ds                /* restore data segment */
   decl B_SOURCE_Y
   decl B_DEST_Y
   decl B_HEIGHT
   jg blit_backwards_loop_fast   /* and loop */

blit_backwards_end:
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
   ret                           /* end of _linear_blit_backward8() */

FUNC(_linear_blit8_end)
   ret




/* void _linear_masked_blit8(BITMAP *source, *dest, int source_x, source_y, 
 *                           int dest_x, dest_y, int width, height);
 *  Masked (skipping zero pixels) blitting routine for linear bitmaps.
 */
FUNC(_linear_masked_blit8)
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
   jne masked8_no_mmx


   movl B_WIDTH, %ecx
   shrl $4, %ecx                 /* Are there more than 16 pixels? Otherwise, use non-MMX code */
   jz masked8_no_mmx

   pxor %mm0, %mm0
   
   pcmpeqd %mm4, %mm4            /* Create inverter mask */
  
   /* Note: maskmovq is an SSE instruction! */

   #define BLIT_CODE                                                                 \
      movd %ecx, %mm2;            /* Save line length (%mm2) */                      \
      shrl $4, %ecx;                                                                 \
                                                                                     \
      pushl %es;  /* Swap ES and DS */                                               \
      pushl %ds;                                                                     \
      popl  %es;                                                                     \
      popl  %ds;                                                                     \
                                                                                     \
      _align_;                                                                       \
      masked8_mmx_x_loop:                                                            \
                                                                                     \
      movq %es:(%esi), %mm1;       /* Read 8 pixels */                               \
      movq %mm0, %mm3;                                                               \
      movq %es:8(%esi), %mm5;      /* Read 8 more pixels */                          \
      movq %mm0, %mm6;                                                               \
                                                                                     \
      pcmpeqb %mm1, %mm3;         /* Compare with mask (%mm3/%mm6) */                \
      pcmpeqb %mm5, %mm6;                                                            \
      pxor %mm4, %mm3;            /* Turn 1->0 and 0->1 */                           \
      pxor %mm4, %mm6;                                                               \
      addl $16, %esi;             /* Update src */                                   \
      maskmovq %mm3, %mm1;        /* Write if not equal to mask. */                  \
      addl $8, %edi;                                                                 \
      maskmovq %mm6, %mm5;                                                           \
                                                                                     \
      addl $8, %edi;              /* Update dest */                                  \
                                                                                     \
      decl %ecx;                  /* Any pixel packs left for this line? */          \
      jnz masked8_mmx_x_loop;                                                        \
                                                                                     \
      movd %mm2, %ecx;            /* Restore pixel count */                          \
      andl $15, %ecx;                                                                \
      jz masked8_mmx_loop_end;    /* Nothing else to do? */                          \
      shrl $1, %ecx;              /* 1 pixels left */                                \
      jnc masked8_mmx_word;                                                          \
                                                                                     \
      movb %es:(%esi), %al;       /* Read 1 pixel */                                 \
      incl %esi;                                                                     \
      incl %edi;                                                                     \
      orb %al, %al;               /* Compare with mask */                            \
      jz masked8_mmx_word;                                                           \
      movb %al, -1(%edi);         /* Write the pixel */                              \
                                                                                     \
      masked8_mmx_word:                                                              \
      shrl $1, %ecx;              /* 2 pixels left */                                \
      jnc masked8_mmx_long;                                                          \
                                                                                     \
      movb %es:(%esi), %al;       /* Read 2 pixels */                                \
      movb %es:1(%esi), %ah;                                                         \
      addl $2, %esi;                                                                 \
      addl $2, %edi;                                                                 \
      orb %al, %al;                                                                  \
      jz masked8_mmx_word_2;                                                         \
      movb %al, -2(%edi);         /* Write pixel */                                  \
                                                                                     \
      masked8_mmx_word_2:                                                            \
      orb %ah, %ah;                                                                  \
      jz masked8_mmx_long;                                                           \
      movb %ah, -1(%edi);         /* Write other pixel */                            \
                                                                                     \
      _align_;                                                                       \
      masked8_mmx_long:                                                              \
                                                                                     \
      shrl $1, %ecx;              /* 4 pixels left */                                \
      jnc masked8_mmx_qword;                                                         \
                                                                                     \
      movl %es:(%esi), %eax;      /* Read 4 pixels */                                \
      addl $4, %esi;                                                                 \
      movd %eax, %mm1;                                                               \
      movl $-1, %eax;                                                                \
      movq %mm0, %mm3;                                                               \
      movd %eax, %mm5;            /* Build XOR flag */                               \
                                                                                     \
      pcmpeqb %mm1, %mm3;         /* Compare with mask (%mm3/%mm6) */                \
      pxor %mm5, %mm3;            /* Turn 1->0 and 0->1 */                           \
      pand %mm5, %mm3;            /* Make sure only the bottom 32 bits are used */   \
      maskmovq %mm3, %mm1;        /* Write if not equal to mask. */                  \
      addl $4, %edi;                                                                 \
                                                                                     \
      _align_;                                                                       \
      masked8_mmx_qword:                                                             \
      shrl $1, %ecx;              /* 8 pixels left */                                \
      jnc masked8_mmx_loop_end;                                                      \
                                                                                     \
      movq %es:(%esi), %mm1;      /* Read 8 more pixels */                           \
      movq %mm0, %mm3;                                                               \
                                                                                     \
      pcmpeqw %mm1, %mm3;         /* Compare with mask (%mm3) */                     \
      pxor %mm4, %mm3;            /* Turn 1->0 and 0->1 */                           \
      maskmovq %mm3, %mm1;        /* Write if not equal to mask. */                  \
                                                                                     \
      _align_;                                                                       \
      masked8_mmx_loop_end:                                                          \
                                                                                     \
      pushl %ds;                  /* Swap back ES and DS */                          \
      popl %es;
   BLIT_LOOP(masked8_mmx_loop, 1, BLIT_CODE)
   #undef BLIT_CODE
   
   emms
   
   jmp masked8_end;
   
#endif
   
	_align_
	masked8_no_mmx:


   #define BLIT_CODE                                                  \
      test $1, %ecx ;            /* 16 bit aligned->use new code */   \
      jz masked16_blit ;         /* width 16 bit aligned */           \
      movb (%esi), %al ;         /* read a byte */                    \
      incl %esi ;                                                     \
      orb %al, %al ;             /* test it */                        \
      jz masked8_skip ;                                               \
      movb %al, %es:(%edi) ;     /* write the pixel */                \
   masked8_skip:                                                      \
      incl %edi ;                                                     \
      decl %ecx ;                                                     \
      jng masked16_blit_end ;                                         \
                                                                      \
   masked16_blit:                                                     \
      test $3, %ecx ;            /* 32 bit aligned->use new code */   \
      jz masked16_blit_x_loop ;  /* width 32 bit aligned */           \
      movw (%esi), %ax ;         /* read two pixels */                \
      orw %ax, %ax ;                                                  \
      jz masked16_blit_end2 ;                                         \
      orb %al,%al ;                                                   \
      jz masked16_blit_wskip1 ;                                       \
      orb %ah,%ah ;                                                   \
      jz masked16_blit_p1wskip2 ;                                     \
      movw %ax, %es:(%edi) ;     /* write the pixel */                \
      jmp masked16_blit_end2 ;                                        \
      _align_ ;                                                       \
   masked16_blit_p1wskip2:                                            \
      movb %al, %es:(%edi) ;    /* write the pixel */                 \
      jmp masked16_blit_end2 ;                                        \
      _align_ ;                                                       \
   masked16_blit_wskip1:                                              \
      movb %ah, %es:1(%edi) ;    /* write the pixel */                \
      _align_ ;                                                       \
   masked16_blit_end2:                                                \
      subl $2, %ecx ;                                                 \
      jng masked16_blit_end ;                                         \
      addl $2, %esi ;                                                 \
      addl $2, %edi ;                                                 \
                                                                      \
      _align_ ;                                                       \
   masked16_blit_x_loop:                                              \
      movl (%esi), %eax ;         /* read four pixels */              \
      addl $4, %esi ;                                                 \
      movl %eax, %edx ;                                               \
      shrl $16,%edx ;                                                 \
      orl %eax, %eax ;                                                \
      jz masked16_blit_skip4 ;                                        \
      orw %ax, %ax ;                                                  \
      jz masked16_blit_skip2 ;                                        \
      orb %al,%al ;                                                   \
      jz masked16_blit_skip1 ;                                        \
      orb %ah, %ah ;                                                  \
      jz masked16_put1skip2 ;                                         \
      orb %dl,%dl ;                                                   \
      jz masked16_put12_skip3 ;                                       \
      orb %dh,%dh ;                                                   \
      jz masked16_put123_skip4 ;                                      \
      movl %eax, %es:(%edi) ;     /* write the pixel */               \
      jmp masked16_blit_skip4 ;                                       \
                                                                      \
      _align_ ;                                                       \
   masked16_put1skip2:                                                \
      movb %al, %es:(%edi) ;     /* write the pixel */                \
      jmp masked16_blit_skip2 ;                                       \
      _align_ ;                                                       \
   masked16_put12_skip3:                                              \
      movw %ax, %es:(%edi) ;     /* write the pixel */                \
      orb %dh, %dh ;                                                  \
      jnz masked16_blit_skip3 ;                                       \
      jmp masked16_blit_skip4 ;                                       \
      _align_ ;                                                       \
   masked16_put123_skip4:                                             \
      movw %ax, %es:(%edi) ;     /* write the pixel */                \
      movb %dl, %es:2(%edi) ;     /* write the pixel */               \
      addl $4, %edi ;                                                 \
      subl $4, %ecx ;                                                 \
      jg masked16_blit_x_loop ;                                       \
      jmp masked16_blit_end ;                                         \
                                                                      \
   masked16_blit_skip1:                                               \
      movb %ah, %es:1(%edi) ;    /* write the pixel */                \
   masked16_blit_skip2:                                               \
      orw %dx, %dx ;                                                  \
      jz masked16_blit_skip4 ;                                        \
      orb %dl,%dl ;                                                   \
      jz masked16_blit_skip3 ;                                        \
      orb %dh, %dh ;                                                  \
      jz masked16_put3skip4 ;                                         \
      movw %dx, %es:2(%edi) ;     /* write the pixel */               \
      jmp masked16_blit_skip4 ;                                       \
                                                                      \
      _align_ ;                                                       \
   masked16_put3skip4:                                                \
      movb %dl, %es:2(%edi) ;     /* write the pixel */               \
      addl $4, %edi ;                                                 \
      subl $4, %ecx ;                                                 \
      jg masked16_blit_x_loop ;                                       \
      jmp masked16_blit_end ;                                         \
                                                                      \
   masked16_blit_skip3:                                               \
      movb %dh, %es:3(%edi) ;    /* write the pixel */                \
   masked16_blit_skip4:                                               \
      addl $4, %edi ;                                                 \
      subl $4, %ecx ;                                                 \
      jg masked16_blit_x_loop ;                                       \
   masked16_blit_end:
   BLIT_LOOP(masked16, 1, BLIT_CODE)
   #undef BLIT_CODE

masked8_end:

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
   ret                           /* end of _linear_masked_blit8() */




#endif      /* ifdef ALLEGRO_COLOR8 */
