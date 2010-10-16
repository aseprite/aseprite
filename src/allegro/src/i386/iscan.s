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
 *      Polygon scanline filler helpers (gouraud shading, tmapping, etc).
 *      MMX scanline fillers are defined in 'iscanmmx.s'
 *
 *      By Calin Andrian.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"


.text



/* all these functions share the same parameters */

#define ADDR      ARG1
#define W         ARG2
#define INFO      ARG3



#ifdef ALLEGRO_COLOR8
/* void _poly_scanline_gcol8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills a single-color gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_gcol8)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   movl INFO, %edi               /* load registers */
   movl POLYSEG_C(%edi), %eax
   movl POLYSEG_DC(%edi), %esi
   movl W, %ecx
   movl ADDR, %edi

   sarl $8, %eax                 /* convert 16.16 fixed into 8.8 fixed */
   sarl $8, %esi
   jge gcol_loop
   incl %esi

   _align_
gcol_loop:
   movb %ah, FSEG(%edi)
   addl %esi, %eax
   incl %edi
   decl %ecx
   jg gcol_loop

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_gcol8() */




/* void _poly_scanline_grgb8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb8)
   pushl %ebp
   movl %esp, %ebp
   subl $8, %esp                /* two local variables: */

#define DRB    -4(%ebp)
#define DG     -8(%ebp)

   pushl %ebx
   pushl %esi
   pushl %edi
grgb_entry:
   movl INFO, %esi               /* load registers */

   movl POLYSEG_DR(%esi), %eax
   movl POLYSEG_DB(%esi), %ecx
   sall $8, %eax
   movl POLYSEG_DG(%esi), %ebx

   sarl $8, %ecx
   andl $0xffff0000, %eax
   sarl $8, %ebx
   addl %ecx, %eax

   movl %ebx, DG
   movl %eax, DRB

   movl POLYSEG_R(%esi), %eax
   movl POLYSEG_B(%esi), %ecx
   sall $8, %eax
   movl POLYSEG_G(%esi), %ebx

   sarl $8, %ecx
   andl $0xffff0000, %eax
   sarl $8, %ebx
   addl %ecx, %eax

   movl ADDR, %edi
   decl %edi
   movl %ebx, %edx			/* green */

   _align_
grgb_loop:
   movl %eax, %ecx			/* red and blue */
   shrl $6, %edx
   andl $0xf800f800, %ecx
   addl DRB, %eax
   shrl $11, %ecx
   andl $0x3e0, %edx
   movl %ecx, %esi
   addl DG, %ebx
   andl $0xffff, %esi			/* blue */
   shrl $6, %ecx				/* red */
   addl %edx, %esi			/* green and blue */
   addl GLOBL(rgb_map), %ecx
   movl %ebx, %edx

   movb (%ecx, %esi), %cl
   movb %cl, FSEG(%edi)          /* write the pixel */
   incl %edi
   decl W
   jg grgb_loop

   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_grgb8() */

#undef DRB
#undef DG

#define DU        -4(%ebp)
#define DVL       -8(%ebp)
#define DVH       -12(%ebp)
#define VMASK     -16(%ebp)
#define TEXTURE   -20(%ebp)
#define C		-24(%ebp)
#define DC		-28(%ebp)
#define TMP		-32(%ebp)

/* helper for setting up an affine texture mapping operation */
#define INIT_ATEX(extra)						 \
   pushl %ebp								;\
   movl %esp, %ebp							;\
   subl $32, %esp							;\
   pushl %ebx								;\
   pushl %esi								;\
   pushl %edi								;\
									;\
   movl INFO, %esi							;\
   movl POLYSEG_VSHIFT(%esi), %ecx					;\
   movl POLYSEG_VMASK(%esi), %ebx					;\
   negl %ecx								;\
   addl $16, %ecx							;\
   movl %ebx, VMASK							;\
   movl POLYSEG_TEXTURE(%esi), %ebx					;\
   movl %ebx, TEXTURE							;\
									;\
   extra								;\
									;\
   xorl %edx, %edx							;\
   movl POLYSEG_DV(%esi), %ebx						;\
   movl POLYSEG_DU(%esi), %eax						;\
   shldl $16, %ebx, %edx						;\
   shll %cl, %eax							;\
   shll $16, %ebx							;\
   movl %eax, DU							;\
   movl %edx, DVH							;\
   movl %ebx, DVL							;\
									;\
   xorl %edx, %edx							;\
   movl POLYSEG_V(%esi), %ebx						;\
   movl POLYSEG_U(%esi), %eax						;\
   shldl $16, %ebx, %edx						;\
   shll %cl, %eax							;\
   shll $16, %ebx							;\
									;\
   movl ADDR, %edi							;\
   addl $16, %ecx							;\
   decl %edi								;\
									;\
   andl VMASK, %edx							;\
   movl %eax, %esi							;\
   incl %edi								;\
   shrdl %cl, %edx, %esi						;\
   addl DU, %eax							;\
   addl TEXTURE, %esi


/* void _poly_scanline_atex8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex8)
   INIT_ATEX(/**/)

   addl DVL, %ebx
   adcl DVH, %edx

   movb (%esi), %ch
   movb %ch, FSEG(%edi)
   decl W
   jz atex_done

   _align_
atex_loop:
   andl VMASK, %edx
   movl %eax, %esi
   incl %edi
   shrdl %cl, %edx, %esi
   addl DU, %eax
   addl TEXTURE, %esi
   addl DVL, %ebx
   adcl DVH, %edx

   movb (%esi), %ch
   movb %ch, FSEG(%edi)
   decl W
   jg atex_loop


atex_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex8() */



/* void _poly_scanline_atex_mask8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask8)
   INIT_ATEX(/**/)

   addl DVL, %ebx
   adcl DVH, %edx

   _align_
atex_mask_loop:
   movb (%esi), %ch
   andl VMASK, %edx
   movl %eax, %esi
   incl %edi
   shrdl %cl, %edx, %esi
   addl DU, %eax
   addl TEXTURE, %esi
   addl DVL, %ebx
   adcl DVH, %edx

   orb  %ch, %ch
   jz atex_mask_skip

   movb %ch, FSEG(%edi)
atex_mask_skip:
   decl W
   jg atex_mask_loop


atex_mask_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex8() */




/* void _poly_scanline_atex_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit8)
   #define INIT_CODE                     \
      movl POLYSEG_C(%esi), %eax ;       \
      movl POLYSEG_DC(%esi), %ebx ;      \
      sarl $8, %eax ;                    \
      sarl $8, %ebx ;                    \
      jge atex_lit_round ;               \
      incl %ebx ;                        \
   atex_lit_round:                       \
      subl %ebx, %eax ;                  \
      movl %ebx, DC ;                    \
      movl %eax, C
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE

   movl %eax, TMP
   addl DVL, %ebx
   movl C, %eax
   adcl DVH, %edx

   addl DC, %eax
   movzbl (%esi), %esi		   /* read texel */
   movl %eax, C

   addl GLOBL(color_map), %esi
   andl $0x0000ff00, %eax
   movb (%eax, %esi), %ch	   /* lookup in lighting table */
   movl TMP, %eax

   movb %ch, FSEG(%edi)
   decl W
   jz atex_lit_done

   _align_
atex_lit_loop:
   andl VMASK, %edx
   movl %eax, %esi
   incl %edi
   shrdl %cl, %edx, %esi
   addl DU, %eax
   addl TEXTURE, %esi
   movl %eax, TMP
   addl DVL, %ebx
   movl C, %eax
   adcl DVH, %edx

   addl DC, %eax
   movzbl (%esi), %esi		   /* read texel */
   movl %eax, C

   addl GLOBL(color_map), %esi
   andl $0x0000ff00, %eax
   movb (%eax, %esi), %ch	   /* lookup in lighting table */
   movl TMP, %eax

   movb %ch, FSEG(%edi)
   decl W
   jg atex_lit_loop


atex_lit_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex_lit8() */



/* void _poly_scanline_atex_mask_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit8)
   #define INIT_CODE                     \
      movl POLYSEG_C(%esi), %eax ;       \
      movl POLYSEG_DC(%esi), %ebx ;      \
      sarl $8, %eax ;                    \
      sarl $8, %ebx ;                    \
      jge atex_mask_lit_round ;          \
      incl %ebx ;                        \
   atex_mask_lit_round:                  \
      subl %ebx, %eax ;                  \
      movl %ebx, DC ;                    \
      movl %eax, C
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE

   movl %eax, TMP
   addl DVL, %ebx
   movl C, %eax
   adcl DVH, %edx

   addl DC, %eax
   movzbl (%esi), %esi		   /* read texel */
   testl $0xffffffff, %esi
   jz atex_mask_lit_skip

   movl %eax, C
   addl GLOBL(color_map), %esi
   andl $0x0000ff00, %eax
   movb (%eax, %esi), %ch	   /* lookup in lighting table */
   movl TMP, %eax

   movb %ch, FSEG(%edi)
   decl W
   jz atex_mask_lit_done

   _align_
atex_mask_lit_loop:
   andl VMASK, %edx
   movl %eax, %esi
   incl %edi
   shrdl %cl, %edx, %esi
   addl DU, %eax
   addl TEXTURE, %esi
   movl %eax, TMP
   addl DVL, %ebx
   movl C, %eax
   adcl DVH, %edx

   addl DC, %eax
   movzbl (%esi), %esi		   /* read texel */
   testl $0xffffffff, %esi
   jz atex_mask_lit_skip

   movl %eax, C
   addl GLOBL(color_map), %esi
   andl $0x0000ff00, %eax
   movb (%eax, %esi), %ch	   /* lookup in lighting table */
   movl TMP, %eax

   movb %ch, FSEG(%edi)

   decl W
   jg atex_mask_lit_loop
   jmp atex_mask_lit_done

atex_mask_lit_skip:
   movl TMP, %eax
   decl W
   jg atex_mask_lit_loop

atex_mask_lit_done:
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_scanline_atex_mask_lit8() */



#undef DU
#undef DVL
#undef DVH
#undef TEXTURE
#undef VMASK
#undef INIT_ATEX
#undef C
#undef DC
#undef TMP

#endif /* COLOR8 */




#define TMP       -4(%ebp)
#define UMASK     -8(%ebp)
#define VMASK     -12(%ebp)
#define VSHIFT    -16(%ebp)
#define DU1       -20(%ebp)
#define DV1       -24(%ebp)
#define DZ1       -28(%ebp)
#define DU4       -32(%ebp)
#define DV4       -36(%ebp)
#define DZ4       -40(%ebp)
#define UDIFF     -44(%ebp)
#define VDIFF     -48(%ebp)
#define PREVU     -52(%ebp)
#define PREVV     -56(%ebp)
#define COUNT     -60(%ebp)
#define C         -64(%ebp)
#define DC        -68(%ebp)
#define TEXTURE   -72(%ebp)




/* helper for starting an fpu 1/z division */
#define START_FP_DIV()                                                       \
   fld1                                                                    ; \
   fdiv %st(3), %st(0)



/* helper for ending an fpu division, returning corrected u and v values */
#define END_FP_DIV(ureg, vreg)                                               \
   fld %st(0)                    /* duplicate the 1/z value */             ; \
									   ; \
   fmul %st(3), %st(0)           /* divide u by z */                       ; \
   fxch %st(1)                                                             ; \
									   ; \
   fmul %st(2), %st(0)           /* divide v by z */                       ; \
									   ; \
   fistpl TMP                    /* store v */                             ; \
   movl TMP, vreg                                                          ; \
									   ; \
   fistpl TMP                    /* store u */                             ; \
   movl TMP, ureg



/* helper for updating the u, v, and z values */
#define UPDATE_FP_POS(n)                                                     \
   fadds DV##n                   /* update v coord */                      ; \
   fxch %st(1)                   /* swap vuz stack to uvz */               ; \
									   ; \
   fadds DU##n                   /* update u coord */                      ; \
   fxch %st(2)                   /* swap uvz stack to zvu */               ; \
									   ; \
   fadds DZ##n                   /* update z value */                      ; \
   fxch %st(2)                   /* swap zvu stack to uvz */               ; \
   fxch %st(1)                   /* swap uvz stack back to vuz */




/* main body of the perspective-correct texture mapping routine */
#define DO_PTEX(name)                                                        \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $72, %esp                /* eighteen local variables */            ; \
									   ; \
   pushl %ebx                                                              ; \
   pushl %esi                                                              ; \
   pushl %edi                                                              ; \
									   ; \
   movl INFO, %esi               /* load registers */                      ; \
									   ; \
   flds POLYSEG_Z(%esi)          /* z at bottom of fpu stack */            ; \
   flds POLYSEG_FU(%esi)         /* followed by u */                       ; \
   flds POLYSEG_FV(%esi)         /* followed by v */                       ; \
									   ; \
   flds POLYSEG_DFU(%esi)        /* multiply diffs by four */              ; \
   flds POLYSEG_DFV(%esi)                                                  ; \
   flds POLYSEG_DZ(%esi)                                                   ; \
   fxch %st(2)                   /* u v z */                               ; \
   fadd %st(0), %st(0)           /* 2u v z */                              ; \
   fxch %st(1)                   /* v 2u z */                              ; \
   fadd %st(0), %st(0)           /* 2v 2u z */                             ; \
   fxch %st(2)                   /* z 2u 2v */                             ; \
   fadd %st(0), %st(0)           /* 2z 2u 2v */                            ; \
   fxch %st(1)                   /* 2u 2z 2v */                            ; \
   fadd %st(0), %st(0)           /* 4u 2z 2v */                            ; \
   fxch %st(2)                   /* 2v 2z 4u */                            ; \
   fadd %st(0), %st(0)           /* 4v 2z 4u */                            ; \
   fxch %st(1)                   /* 2z 4v 4u */                            ; \
   fadd %st(0), %st(0)           /* 4z 4v 4u */                            ; \
   fxch %st(2)                   /* 4u 4v 4z */                            ; \
   fstps DU4                                                               ; \
   fstps DV4                                                               ; \
   fstps DZ4                                                               ; \
									   ; \
   START_FP_DIV()                /* prime the 1/z calculation */           ; \
									   ; \
   movl POLYSEG_DFU(%esi), %eax  /* copy diff values onto the stack */     ; \
   movl POLYSEG_DFV(%esi), %ebx                                            ; \
   movl POLYSEG_DZ(%esi), %ecx                                             ; \
   movl %eax, DU1                                                          ; \
   movl %ebx, DV1                                                          ; \
   movl %ecx, DZ1                                                          ; \
									   ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                         ; \
   movl POLYSEG_VMASK(%esi), %edx                                          ; \
   movl POLYSEG_UMASK(%esi), %eax                                          ; \
									   ; \
   shll %cl, %edx                /* adjust v mask and shift value */       ; \
   negl %ecx                                                               ; \
   movl %eax, UMASK                                                        ; \
   addl $16, %ecx                                                          ; \
   movl %edx, VMASK                                                        ; \
   movl %ecx, VSHIFT                                                       ; \
									   ; \
   INIT()                                                                  ; \
									   ; \
   movl ADDR, %edi                                                         ; \
									   ; \
   _align_                                                                 ; \
name##_odd_byte_loop:                                                      ; \
   testl $3, %edi                /* deal with any odd pixels */            ; \
   jz name##_no_odd_bytes                                                  ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movb VSHIFT, %cl              /* shift and mask v coordinate */         ; \
   sarl %cl, %edx                                                          ; \
   andl VMASK, %edx                                                        ; \
									   ; \
   sarl $16, %ebx                /* shift and mask u coordinate */         ; \
   andl UMASK, %ebx                                                        ; \
   addl %ebx, %edx                                                         ; \
									   ; \
   SINGLE_PIXEL(1)               /* process the pixel */                   ; \
									   ; \
   decl W                                                                  ; \
   jg name##_odd_byte_loop                                                 ; \
									   ; \
   _align_                                                                 ; \
name##_no_odd_bytes:                                                       ; \
   movl W, %ecx                  /* inner loop expanded 4 times */         ; \
   movl %ecx, COUNT                                                        ; \
   shrl $2, %ecx                                                           ; \
   jg name##_prime_x4                                                      ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movl %ebx, PREVU              /* store initial u/v pos */               ; \
   movl %edx, PREVV                                                        ; \
									   ; \
   jmp name##_cleanup_leftovers                                            ; \
									   ; \
   _align_                                                                 ; \
name##_prime_x4:                                                           ; \
   movl %ecx, W                                                            ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(4)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movl %ebx, PREVU              /* store initial u/v pos */               ; \
   movl %edx, PREVV                                                        ; \
									   ; \
   jmp name##_start_loop                                                   ; \
									   ; \
   _align_                                                                 ; \
name##_last_time:                                                          ; \
   END_FP_DIV(%eax, %ecx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   jmp name##_loop_contents                                                ; \
									   ; \
   _align_                                                                 ; \
name##_loop:                                                               ; \
   END_FP_DIV(%eax, %ecx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(4)              /* update u/v/z values */                 ; \
									   ; \
name##_loop_contents:                                                      ; \
   START_FP_DIV()                /* start up the next divide */            ; \
									   ; \
   movl PREVU, %ebx              /* read the previous u/v pos */           ; \
   movl PREVV, %edx                                                        ; \
									   ; \
   movl %eax, PREVU              /* store u/v for the next cycle */        ; \
   movl %ecx, PREVV                                                        ; \
									   ; \
   subl %ebx, %eax               /* get u/v diffs for the four pixels */   ; \
   subl %edx, %ecx                                                         ; \
   sarl $2, %eax                                                           ; \
   sarl $2, %ecx                                                           ; \
   movl %eax, UDIFF                                                        ; \
   movl %ecx, VDIFF                                                        ; \
									   ; \
   PREPARE_FOUR_PIXELS()                                                   ; \
									   ; \
   /* pixel 1 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(0)                /* process pixel 1 */                     ; \
									   ; \
   addl UDIFF, %ebx                                                        ; \
   addl VDIFF, %edx                                                        ; \
									   ; \
   /* pixel 2 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(1)                /* process pixel 2 */                     ; \
									   ; \
   addl UDIFF, %ebx                                                        ; \
   addl VDIFF, %edx                                                        ; \
									   ; \
   /* pixel 3 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(2)                /* process pixel 3 */                     ; \
									   ; \
   addl UDIFF, %ebx                                                        ; \
   addl VDIFF, %edx                                                        ; \
									   ; \
   /* pixel 4 */                                                           ; \
   movl %edx, %eax               /* shift and mask v coordinate */         ; \
   movb VSHIFT, %cl                                                        ; \
   sarl %cl, %eax                                                          ; \
   andl VMASK, %eax                                                        ; \
									   ; \
   movl %ebx, %ecx               /* shift and mask u coordinate */         ; \
   sarl $16, %ecx                                                          ; \
   andl UMASK, %ecx                                                        ; \
   addl %ecx, %eax                                                         ; \
									   ; \
   FOUR_PIXELS(3)                /* process pixel 4 */                     ; \
									   ; \
   WRITE_FOUR_PIXELS()           /* write four pixels at a time */         ; \
									   ; \
name##_start_loop:                                                         ; \
   decl W                                                                  ; \
   jg name##_loop                                                          ; \
   jz name##_last_time                                                     ; \
									   ; \
name##_cleanup_leftovers:                                                  ; \
   movl COUNT, %ecx              /* deal with any leftover pixels */       ; \
   andl $3, %ecx                                                           ; \
   jz name##_done                                                          ; \
									   ; \
   movl %ecx, COUNT                                                        ; \
   movl PREVU, %ebx                                                        ; \
   movl PREVV, %edx                                                        ; \
									   ; \
   _align_                                                                 ; \
name##_cleanup_loop:                                                       ; \
   movb VSHIFT, %cl              /* shift and mask v coordinate */         ; \
   sarl %cl, %edx                                                          ; \
   andl VMASK, %edx                                                        ; \
									   ; \
   sarl $16, %ebx                /* shift and mask u coordinate */         ; \
   andl UMASK, %ebx                                                        ; \
   addl %ebx, %edx                                                         ; \
									   ; \
   SINGLE_PIXEL(2)               /* process the pixel */                   ; \
									   ; \
   decl COUNT                                                              ; \
   jz name##_done                                                          ; \
									   ; \
   END_FP_DIV(%ebx, %edx)        /* get corrected u/v position */          ; \
   UPDATE_FP_POS(1)              /* update u/v/z values */                 ; \
   START_FP_DIV()                /* start up the next divide */            ; \
   jmp name##_cleanup_loop                                                 ; \
									   ; \
name##_done:                                                               ; \
   fstp %st(0)                   /* pop fpu stack */                       ; \
   fstp %st(0)                                                             ; \
   fstp %st(0)                                                             ; \
   fstp %st(0)                                                             ; \
									   ; \
   popl %edi                                                               ; \
   popl %esi                                                               ; \
   popl %ebx                                                               ; \
   movl %ebp, %esp                                                         ; \
   popl %ebp




#ifdef ALLEGRO_COLOR8
/* void _poly_scanline_ptex8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex8)

   #define INIT()                                                            \
      movl POLYSEG_TEXTURE(%esi), %esi


   #define SINGLE_PIXEL(n)                                                   \
      movb (%esi, %edx), %al     /* read texel */                          ; \
      movb %al, FSEG(%edi)       /* write the pixel */                     ; \
      incl %edi


   #define PREPARE_FOUR_PIXELS()                                             \
      /* noop */


   #define FOUR_PIXELS(n)                                                    \
      movb (%esi, %eax), %al     /* read texel */                          ; \
      movb %al, n+TMP            /* store the pixel */


   #define WRITE_FOUR_PIXELS()                                               \
      movl TMP, %eax             /* write four pixels at a time */         ; \
      movl %eax, FSEG(%edi)                                                ; \
      addl $4, %edi


   DO_PTEX(ptex)


   #undef INIT
   #undef SINGLE_PIXEL
   #undef PREPARE_FOUR_PIXELS
   #undef FOUR_PIXELS
   #undef WRITE_FOUR_PIXELS

   ret                           /* end of _poly_scanline_ptex8() */




/* void _poly_scanline_ptex_mask8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask8)

   #define INIT()                                                            \
      movl POLYSEG_TEXTURE(%esi), %esi


   #define SINGLE_PIXEL(n)                                                   \
      movb (%esi, %edx), %al     /* read texel */                          ; \
      orb %al, %al                                                         ; \
      jz ptex_mask_skip_left_##n                                           ; \
      movb %al, FSEG(%edi)       /* write the pixel */                     ; \
   ptex_mask_skip_left_##n:                                                ; \
      incl %edi


   #define PREPARE_FOUR_PIXELS()                                             \
      /* noop */


   #define FOUR_PIXELS(n)                                                    \
      movb (%esi, %eax), %al     /* read texel */                          ; \
      orb %al, %al                                                         ; \
      jz ptex_mask_skip_##n                                                ; \
      movb %al, FSEG(%edi)       /* write the pixel */                     ; \
   ptex_mask_skip_##n:                                                     ; \
      incl %edi


   #define WRITE_FOUR_PIXELS()                                               \
      /* noop */


   DO_PTEX(ptex_mask)


   #undef INIT
   #undef SINGLE_PIXEL
   #undef PREPARE_FOUR_PIXELS
   #undef FOUR_PIXELS
   #undef WRITE_FOUR_PIXELS

   ret                           /* end of _poly_scanline_ptex_mask8() */



/* void _poly_scanline_ptex_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit8)

   #define INIT()                                                            \
      movl POLYSEG_C(%esi), %eax                                           ; \
      movl POLYSEG_DC(%esi), %ebx                                          ; \
      movl POLYSEG_TEXTURE(%esi), %ecx                                     ; \
      sarl $8, %eax                                                        ; \
      sarl $8, %ebx                                                        ; \
      movl %eax, C                                                         ; \
      movl %ebx, DC                                                        ; \
      movl %ecx, TEXTURE


   #define SINGLE_PIXEL(n)                                                   \
      movl TEXTURE, %esi         /* read texel */                          ; \
      movzbl (%esi, %edx), %eax                                            ; \
									   ; \
      movb 1+C, %ah              /* lookup in lighting table */            ; \
      movl GLOBL(color_map), %esi                                          ; \
      movl DC, %ecx                                                        ; \
      movb (%eax, %esi), %al                                               ; \
      addl %ecx, C                                                         ; \
									   ; \
      movb %al, FSEG(%edi)       /* write the pixel */                     ; \
      incl %edi


   #define PREPARE_FOUR_PIXELS()                                             \
      movl TEXTURE, %esi


   #define FOUR_PIXELS(n)                                                    \
      movl TEXTURE, %esi                                                   ; \
      movb (%esi, %eax), %al     /* read texel */                          ; \
      movb %al, n+TMP            /* store the pixel */


   #define WRITE_FOUR_PIXELS()                                               \
      movl GLOBL(color_map), %esi   /* light four pixels */                ; \
      xorl %ecx, %ecx                                                      ; \
      movl DC, %ebx                                                        ; \
      movl C, %edx                                                         ; \
									   ; \
      movb %dh, %ch              /* light pixel 1 */                       ; \
      movb TMP, %cl                                                        ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %al                                               ; \
									   ; \
      movb %dh, %ch              /* light pixel 2 */                       ; \
      movb 1+TMP, %cl                                                      ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %ah                                               ; \
									   ; \
      roll $16, %eax                                                       ; \
									   ; \
      movb %dh, %ch              /* light pixel 3 */                       ; \
      movb 2+TMP, %cl                                                      ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %al                                               ; \
									   ; \
      movb %dh, %ch              /* light pixel 4 */                       ; \
      movb 3+TMP, %cl                                                      ; \
      addl %ebx, %edx                                                      ; \
      movb (%ecx, %esi), %ah                                               ; \
									   ; \
      movl %edx, C                                                         ; \
      roll $16, %eax                                                       ; \
      movl %eax, FSEG(%edi)      /* write four pixels at a time */         ; \
      addl $4, %edi


   DO_PTEX(ptex_lit)


   #undef INIT
   #undef SINGLE_PIXEL
   #undef PREPARE_FOUR_PIXELS
   #undef FOUR_PIXELS
   #undef WRITE_FOUR_PIXELS

   ret                           /* end of _poly_scanline_ptex_lit8() */

#endif /* COLOR8 */
#undef TMP
#undef UMASK
#undef VMASK
#undef VSHIFT
#undef DU1
#undef DV1
#undef DZ1
#undef DU4
#undef DV4
#undef DZ4
#undef UDIFF
#undef VDIFF
#undef PREVU
#undef PREVV
#undef COUNT
#undef C
#undef DC
#undef TEXTURE

#undef START_FP_DIV
#undef END_FP_DIV
#undef UPDATE_FP_POS
#undef DO_PTEX



#define DB      -4(%ebp)
#define DG      -8(%ebp)
#define DR     -12(%ebp)

#define TMP4    -4(%ebp)
#define TMP     -8(%ebp)
#define SB     -16(%ebp)
#define SG     -24(%ebp)
#define SR     -32(%ebp)

/* first part of a grgb routine */
#define INIT_GRGB(depth, r_sft, g_sft, b_sft)                         \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $12, %esp                                                   ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
grgb_entry_##depth:                                                 ; \
   movl INFO, %esi               /* load registers */               ; \
								    ; \
   movl POLYSEG_DR(%esi), %ebx   /* move delta's on stack */        ; \
   movl POLYSEG_DG(%esi), %edi                                      ; \
   movl POLYSEG_DB(%esi), %edx                                      ; \
   movl %ebx, DR                                                    ; \
   movl %edi, DG                                                    ; \
   movl %edx, DB                                                    ; \
								    ; \
   movl POLYSEG_R(%esi), %ebx                                       ; \
   movl POLYSEG_G(%esi), %edi                                       ; \
   movl POLYSEG_B(%esi), %edx                                       ; \
								    ; \
   _align_                                                          ; \
grgb_loop_##depth:                                                  ; \
   movl %ebx, %eax              /* red */                           ; \
   movb GLOBL(_rgb_r_shift_##depth), %cl                            ; \
   shrl $r_sft, %eax                                                ; \
   addl DR, %ebx                                                    ; \
   shll %cl, %eax                                                   ; \
   movl %edi, %esi              /* green */                         ; \
   movb GLOBL(_rgb_g_shift_##depth), %cl                            ; \
   shrl $g_sft, %esi                                                ; \
   addl DG, %edi                                                    ; \
   shll %cl, %esi                                                   ; \
   orl %esi, %eax                                                   ; \
   movl %edx, %esi              /* blue */                          ; \
   movb GLOBL(_rgb_b_shift_##depth), %cl                            ; \
   shrl $b_sft, %esi                                                ; \
   addl DB, %edx                                                    ; \
   shll %cl, %esi                                                   ; \
   orl %esi, %eax                                                   ; \
   movl ADDR, %esi

/* end of grgb routine */
#define END_GRGB(depth)                                               \
   movl %esi, ADDR                                                  ; \
   decl W                                                           ; \
   jg grgb_loop_##depth                                             ; \
								    ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_grgb15(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb15)
   INIT_GRGB(15, 19, 19, 19)
   movw %ax, FSEG(%esi)          /* write the pixel */
   addl $2, %esi
   END_GRGB(15)
   ret                           /* end of _poly_scanline_grgb15() */



/* void _poly_scanline_grgb16(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb16)
   INIT_GRGB(16, 19, 18, 19)
   movw %ax, FSEG(%esi)          /* write the pixel */
   addl $2, %esi
   END_GRGB(16)
   ret                           /* end of _poly_scanline_grgb16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_grgb32(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb32)
   INIT_GRGB(32, 16, 16, 16)
   movl %eax, FSEG(%esi)         /* write the pixel */
   addl $4, %esi
   END_GRGB(32)
   ret                           /* end of _poly_scanline_grgb32() */

#endif /* COLOR32 */

#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_grgb24(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_scanline_grgb24)
   INIT_GRGB(24, 16, 16, 16)
   movw %ax, FSEG(%esi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%esi)
   addl $3, %esi
   END_GRGB(24)
   ret                           /* end of _poly_scanline_grgb24() */

#endif /* COLOR24 */

#undef DR
#undef DG
#undef DB
#undef TMP
#undef TMP2
#undef TMP4
#undef TMP6
#undef SR
#undef SG
#undef SB



#define VMASK      -8(%ebp)
#define VSHIFT    -16(%ebp)
#define DV        -20(%ebp)
#define DU        -24(%ebp)
#define ALPHA4    -28(%ebp)
#define ALPHA2    -30(%ebp)
#define ALPHA     -32(%ebp)
#define DALPHA4   -36(%ebp)
#define DALPHA    -40(%ebp)
#define UMASK     -44(%ebp)
#define READ_ADDR -48(%ebp)

/* first part of an affine texture mapping operation */
#define INIT_ATEX(extra)                                              \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $48, %esp                    /* local variables */          ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   movl INFO, %esi               /* load registers */               ; \
   extra                                                            ; \
								    ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                  ; \
   movl POLYSEG_VMASK(%esi), %eax                                   ; \
   shll %cl, %eax           /* adjust v mask and shift value */     ; \
   negl %ecx                                                        ; \
   addl $16, %ecx                                                   ; \
   movl %ecx, VSHIFT                                                ; \
   movl %eax, VMASK                                                 ; \
								    ; \
   movl POLYSEG_UMASK(%esi), %eax                                   ; \
   movl POLYSEG_DU(%esi), %ebx                                      ; \
   movl POLYSEG_DV(%esi), %edx                                      ; \
   movl %eax, UMASK                                                 ; \
   movl %ebx, DU                                                    ; \
   movl %edx, DV                                                    ; \
								    ; \
   movl POLYSEG_U(%esi), %ebx                                       ; \
   movl POLYSEG_V(%esi), %edx                                       ; \
   movl ADDR, %edi                                                  ; \
   movl POLYSEG_TEXTURE(%esi), %esi                                 ; \
								    ; \
   _align_                                                          ; \
1:                                                                  ; \
   movl %edx, %eax               /* get v */                        ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx               /* get u */                        ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax

/* end of atex routine */
#define END_ATEX()                                                  ; \
   addl DU, %ebx                                                    ; \
   addl DV, %edx                                                    ; \
   decl W                                                           ; \
   jg 1b                                                            ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_atex16(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex16)
   INIT_ATEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   addl $2, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex16() */

/* void _poly_scanline_atex_mask15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask15)
   INIT_ATEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
7: addl $2, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask15() */

/* void _poly_scanline_atex_mask16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask16)
   INIT_ATEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
7: addl $2, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask16() */
#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_atex32(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex32)
   INIT_ATEX(/**/)
   movl (%esi, %eax, 4), %eax    /* read texel */
   movl %eax, FSEG(%edi)         /* write the pixel */
   addl $4, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex32() */

/* void _poly_scanline_atex_mask32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask32)
   INIT_ATEX(/**/)
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   movl %eax, FSEG(%edi)         /* write solid pixels */
7: addl $4, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask32() */
#endif /* COLOR32 */

#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_atex24(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex24)
   INIT_ATEX(/**/)
   leal (%eax, %eax, 2), %ecx
   movw (%esi, %ecx), %ax        /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   movb 2(%esi, %ecx), %al       /* read texel */
   movb %al, FSEG 2(%edi)        /* write the pixel */
   addl $3, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex24() */

/* void _poly_scanline_atex_mask24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask24)
   INIT_ATEX(/**/)
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
7: addl $3, %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask24() */
#endif /* COLOR24 */



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_atex_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit15)
   #define INIT_CODE                     \
     movl POLYSEG_C(%esi), %eax ;        \
     movl POLYSEG_DC(%esi), %edx ;       \
     movl %eax, ALPHA ;                  \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movw (%esi, %eax, 2), %ax    /* read texel */
   pushl GLOBL(_blender_col_15)
   pushl %eax
   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)         /* write the pixel */
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_lit15() */

/* void _poly_scanline_atex_mask_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit15)
   #define INIT_CODE                     \
     movl POLYSEG_C(%esi), %eax ;        \
     movl POLYSEG_DC(%esi), %edx ;       \
     movl %eax, ALPHA ;                  \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax    /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_15)
   pushl %eax
   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)         /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_lit15() */



/* void _poly_scanline_atex_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit16)
   #define INIT_CODE                     \
     movl POLYSEG_C(%esi), %eax ;        \
     movl POLYSEG_DC(%esi), %edx ;       \
     movl %eax, ALPHA ;                  \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movw (%esi, %eax, 2), %ax     /* read texel */
   pushl GLOBL(_blender_col_16)
   pushl %eax
   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_lit16() */

/* void _poly_scanline_atex_mask_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit16)
   #define INIT_CODE                     \
     movl POLYSEG_C(%esi), %eax ;        \
     movl POLYSEG_DC(%esi), %edx ;       \
     movl %eax, ALPHA ;                  \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_16)
   pushl %eax
   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_lit16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_atex_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit32)
   #define INIT_CODE                     \
     movl POLYSEG_C(%esi), %eax ;        \
     movl POLYSEG_DC(%esi), %edx ;       \
     movl %eax, ALPHA ;                  \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movl (%esi, %eax, 4), %eax    /* read texel */
   pushl GLOBL(_blender_col_32)
   pushl %eax
   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   movl DALPHA, %eax
   addl $4, %edi
   addl %eax, ALPHA
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_lit32() */

/* void _poly_scanline_atex_mask_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit32)
   #define INIT_CODE                     \
     movl POLYSEG_C(%esi), %eax ;        \
     movl POLYSEG_DC(%esi), %edx ;       \
     movl %eax, ALPHA ;                  \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_32)
   pushl %eax
   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $4, %edi
   addl %eax, ALPHA
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_lit32() */

#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_atex_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_lit24)
   #define INIT_CODE                     \
     movl POLYSEG_C(%esi), %eax ;        \
     movl POLYSEG_DC(%esi), %edx ;       \
     movl %eax, ALPHA ;                  \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   leal (%eax, %eax, 2), %ecx
   movb 2(%esi, %ecx), %al        /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   pushl GLOBL(_blender_col_24)
   pushl %eax
   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   movl DALPHA, %eax
   addl $3, %edi
   addl %eax, ALPHA
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_lit24() */

/* void _poly_scanline_atex_mask_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_lit24)
   #define INIT_CODE                     \
     movl POLYSEG_C(%esi), %eax ;        \
     movl POLYSEG_DC(%esi), %edx ;       \
     movl %eax, ALPHA ;                  \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_24)
   pushl %eax
   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   popl %edx
7:
   movl DALPHA, %eax
   addl $3, %edi
   addl %eax, ALPHA
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_lit24() */

#endif /* COLOR24 */



#ifdef ALLEGRO_COLOR8
/* void _poly_scanline_atex_trans8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_trans8)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edi
   movzbl (%esi, %eax), %eax     /* read texel */
   movl READ_ADDR, %edi
   shll $8, %eax
   movb FSEG (%edi), %al		 /* read pixel */
   movl GLOBL(color_map), %ecx
   popl %edi
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
   incl READ_ADDR
   incl %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_trans8() */

/* void _poly_scanline_atex_mask_trans8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_trans8)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   orl %eax, %eax
   jz 7f
   shll $8, %eax
   pushl %edi
   movl READ_ADDR, %edi
   movb FSEG (%edi), %al
   popl %edi
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
7:
   incl READ_ADDR
   incl %edi
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_trans8() */
#endif /* COLOR8 */



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_atex_trans15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_trans15)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %edi
   movl READ_ADDR, %edi
   movw FSEG (%edi), %cx
   popl %edi
   movw (%esi, %eax, 2), %ax    /* read texel */
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)         /* write the pixel */
   addl $2, READ_ADDR
   addl $2, %edi
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_trans15() */

/* void _poly_scanline_atex_mask_trans15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_trans15)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax    /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   pushl %edi
   movl READ_ADDR, %edi
   movw FSEG (%edi), %cx
   popl %edi
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)         /* write the pixel */
   popl %edx
7:
   addl $2, %edi
   addl $2, READ_ADDR
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_trans15() */



/* void _poly_scanline_atex_trans16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_trans16)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %edi
   movl READ_ADDR, %edi
   movw FSEG (%edi), %cx
   popl %edi
   movw (%esi, %eax, 2), %ax     /* read texel */
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   addl $2, READ_ADDR
   addl $2, %edi
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_trans16() */

/* void _poly_scanline_atex_mask_trans16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_trans16)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   pushl %edi
   movl READ_ADDR, %edi
   movw FSEG (%edi), %cx
   popl %edi
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
7:
   addl $2, %edi
   addl $2, READ_ADDR
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_trans16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_atex_trans32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_trans32)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   pushl GLOBL(_blender_alpha)
   movl %edi, ALPHA
   movl READ_ADDR, %edi
   pushl FSEG (%edi)
   movl ALPHA, %edi
   pushl (%esi, %eax, 4)

   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   addl $4, READ_ADDR
   addl $4, %edi
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_trans32() */

/* void _poly_scanline_atex_mask_trans32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_trans32)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   pushl %edx
   pushl GLOBL(_blender_alpha)
   movl %edi, ALPHA
   movl READ_ADDR, %edi
   pushl FSEG (%edi)
   movl ALPHA, %edi
   pushl %eax

   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   popl %edx
7:
   addl $4, %edi
   addl $4, READ_ADDR
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_trans32() */

#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_atex_trans24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_trans24)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   pushl GLOBL(_blender_alpha)
   leal (%eax, %eax, 2), %ecx
   pushl %edi
   movl READ_ADDR, %edi
   movb FSEG 2(%edi), %dl
   movb 2(%esi, %ecx), %al        /* read texel */
   shll $16, %edx
   shll $16, %eax
   movw FSEG (%edi), %dx
   popl %edi
   movw (%esi, %ecx), %ax
   pushl %edx
   pushl %eax

   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   addl $3, READ_ADDR
   addl $3, %edi
   popl %edx
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_trans24() */

/* void _poly_scanline_atex_mask_trans24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_scanline_atex_mask_trans24)
   #define INIT_CODE                     \
     movl POLYSEG_RADDR(%esi), %eax ;    \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   pushl %edi
   movl READ_ADDR, %edi
   movb FSEG 2(%edi), %cl
   shll $16, %ecx
   movw FSEG (%edi), %cx
   popl %edi
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   popl %edx
7:
   addl $3, %edi
   addl $3, READ_ADDR
   END_ATEX()
   ret                           /* end of _poly_scanline_atex_mask_trans24() */

#endif /* COLOR24 */

#undef VMASK
#undef VSHIFT
#undef UMASK
#undef DU
#undef DV
#undef ALPHA
#undef ALPHA2
#undef ALPHA4
#undef DALPHA
#undef DALPHA4
#undef READ_ADDR



#define VMASK     -4(%ebp)
#define VSHIFT     -8(%ebp)
#define ALPHA4    -12(%ebp)
#define ALPHA     -16(%ebp)
#define DALPHA4   -20(%ebp)
#define DALPHA    -24(%ebp)
#define TMP4      -28(%ebp)
#define TMP2      -30(%ebp)
#define TMP       -32(%ebp)
#define BLEND4    -36(%ebp)
#define BLEND2    -38(%ebp)
#define BLEND     -40(%ebp)
#define U1        -44(%ebp)
#define V1        -48(%ebp)
#define DU        -52(%ebp)
#define DV        -56(%ebp)
#define DU4       -60(%ebp)
#define DV4       -64(%ebp)
#define DZ4       -68(%ebp)
#define UMASK     -72(%ebp)
#define COUNT     -76(%ebp)
#define SM        -80(%ebp)
#define SV        -84(%ebp)
#define SU        -88(%ebp)
#define SZ        -92(%ebp)
#define READ_ADDR -96(%ebp)

/* helper for starting an fpu 1/z division */
#define START_FP_DIV()                                                \
   fld1                                                             ; \
   fdiv %st(3), %st(0)

/* helper for ending an fpu division, returning corrected u and v values */
#define END_FP_DIV()                                                  \
   fld %st(0)                    /* duplicate the 1/z value */      ; \
   fmul %st(3), %st(0)           /* divide u by z */                ; \
   fxch %st(1)                                                      ; \
   fmul %st(2), %st(0)           /* divide v by z */

#define UPDATE_FP_POS_4()                                             \
   fadds DV4                     /* update v coord */               ; \
   fxch %st(1)                   /* swap vuz stack to uvz */        ; \
   fadds DU4                     /* update u coord */               ; \
   fxch %st(2)                   /* swap uvz stack to zvu */        ; \
   fadds DZ4                     /* update z value */               ; \
   fxch %st(2)                   /* swap zvu stack to uvz */        ; \
   fxch %st(1)                   /* swap uvz stack back to vuz */

/* main body of the perspective-correct texture mapping routine */
#define INIT_PTEX(extra)                                              \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $100, %esp               /* local variables */              ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   movl INFO, %esi               /* load registers */               ; \
								    ; \
   flds POLYSEG_Z(%esi)          /* z at bottom of fpu stack */     ; \
   flds POLYSEG_FU(%esi)         /* followed by u */                ; \
   flds POLYSEG_FV(%esi)         /* followed by v */                ; \
								    ; \
   flds POLYSEG_DFU(%esi)        /* multiply diffs by four */       ; \
   flds POLYSEG_DFV(%esi)                                           ; \
   flds POLYSEG_DZ(%esi)                                            ; \
   fxch %st(2)                   /* u v z */                        ; \
   fadd %st(0), %st(0)           /* 2u v z */                       ; \
   fxch %st(1)                   /* v 2u z */                       ; \
   fadd %st(0), %st(0)           /* 2v 2u z */                      ; \
   fxch %st(2)                   /* z 2u 2v */                      ; \
   fadd %st(0), %st(0)           /* 2z 2u 2v */                     ; \
   fxch %st(1)                   /* 2u 2z 2v */                     ; \
   fadd %st(0), %st(0)           /* 4u 2z 2v */                     ; \
   fxch %st(2)                   /* 2v 2z 4u */                     ; \
   fadd %st(0), %st(0)           /* 4v 2z 4u */                     ; \
   fxch %st(1)                   /* 2z 4v 4u */                     ; \
   fadd %st(0), %st(0)           /* 4z 4v 4u */                     ; \
   fxch %st(2)                   /* 4u 4v 4z */                     ; \
   fstps DU4                                                        ; \
   fstps DV4                                                        ; \
   fstps DZ4                                                        ; \
								    ; \
   START_FP_DIV()                                                   ; \
								    ; \
   extra                                                            ; \
								    ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                  ; \
   movl POLYSEG_VMASK(%esi), %eax                                   ; \
   movl POLYSEG_UMASK(%esi), %edx                                   ; \
   shll %cl, %eax           /* adjust v mask and shift value */     ; \
   negl %ecx                                                        ; \
   addl $16, %ecx                                                   ; \
   movl %ecx, VSHIFT                                                ; \
   movl %eax, VMASK                                                 ; \
   movl %edx, UMASK                                                 ; \
								    ; \
   END_FP_DIV()                                                     ; \
   fistpl V1                     /* store v */                      ; \
   fistpl U1                     /* store u */                      ; \
   UPDATE_FP_POS_4()                                                ; \
   START_FP_DIV()                                                   ; \
								    ; \
   movl ADDR, %edi                                                  ; \
   movl V1, %edx                                                    ; \
   movl U1, %ebx                                                    ; \
   movl POLYSEG_TEXTURE(%esi), %esi                                 ; \
   movl $0, COUNT                /* COUNT ranges from 3 to -1 */    ; \
   jmp 3f                                                           ; \
								    ; \
   _align_                                                          ; \
1:                  /* step 2: compute DU, DV for next 4 steps */   ; \
   movl $3, COUNT                                                   ; \
								    ; \
   END_FP_DIV()                 /* finish the divide */             ; \
   fistpl V1                                                        ; \
   fistpl U1                                                        ; \
   UPDATE_FP_POS_4()            /* 4 pixels away */                 ; \
   START_FP_DIV()                                                   ; \
								    ; \
   movl V1, %eax                                                    ; \
   movl U1, %ecx                                                    ; \
   subl %edx, %eax                                                  ; \
   subl %ebx, %ecx                                                  ; \
   sarl $2, %eax                                                    ; \
   sarl $2, %ecx                                                    ; \
   movl %eax, DV                                                    ; \
   movl %ecx, DU                                                    ; \
2:                              /* step 3 & 4: next U, V */         ; \
   addl DV, %edx                                                    ; \
   addl DU, %ebx                                                    ; \
3:                              /* step 1: just use U and V */      ; \
   movl %edx, %eax              /* get v */                         ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx              /* get u */                         ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax

#define END_PTEX()                                                    \
   decl W                                                           ; \
   jle 6f                                                           ; \
   decl COUNT                                                       ; \
   jg 2b                                                            ; \
   nop                                                              ; \
   jl 1b                                                            ; \
   movl V1, %edx                                                    ; \
   movl U1, %ebx                                                    ; \
   jmp 3b                                                           ; \
   _align_                                                          ; \
6:                                                                  ; \
   fstp %st(0)                   /* pop fpu stack */                ; \
   fstp %st(0)                                                      ; \
   fstp %st(0)                                                      ; \
   fstp %st(0)                                                      ; \
								    ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp





#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_ptex16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex16)
   INIT_PTEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   addl $2, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex16() */

/* void _poly_scanline_ptex_mask15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask15)
   INIT_PTEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
7: addl $2, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask15() */

/* void _poly_scanline_ptex_mask16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask16)
   INIT_PTEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
7: addl $2, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask16() */
#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_ptex32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex32)
   INIT_PTEX(/**/)
   movl (%esi, %eax, 4), %eax    /* read texel */
   movl %eax, FSEG(%edi)         /* write the pixel */
   addl $4, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex32() */

/* void _poly_scanline_ptex_mask32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask32)
   INIT_PTEX(/**/)
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   movl %eax, FSEG(%edi)         /* write solid pixels */
7: addl $4, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask32() */
#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_ptex24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex24)
   INIT_PTEX(/**/)
   leal (%eax, %eax, 2), %ecx
   movw (%esi, %ecx), %ax        /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   movb 2(%esi, %ecx), %al       /* read texel */
   movb %al, FSEG 2(%edi)        /* write the pixel */
   addl $3, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex24() */

/* void _poly_scanline_ptex_mask24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask24)
   INIT_PTEX(/**/)
   leal (%eax, %eax, 2), %ecx
   xorl %eax, %eax
   movb 2(%esi, %ecx), %al       /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
7: addl $3, %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask24() */
#endif /* COLOR24 */




#ifdef ALLEGRO_COLOR8
/* void _poly_scanline_ptex_mask_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit8)
   #define INIT_CODE                      \
     movl POLYSEG_C(%esi), %eax ;         \
     movl POLYSEG_DC(%esi), %edx ;        \
     movl %eax, ALPHA ;                   \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   orl %eax, %eax
   jz 7f
   movb 2+ALPHA, %ah
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
7:
   movl DALPHA, %eax
   incl %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit8() */
#endif /* COLOR8 */



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_ptex_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit15)
   #define INIT_CODE                      \
     movl POLYSEG_C(%esi), %eax ;         \
     movl POLYSEG_DC(%esi), %edx ;        \
     movl %eax, ALPHA ;                   \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movw (%esi, %eax, 2), %ax     /* read texel */
   pushl GLOBL(_blender_col_15)
   pushl %eax
   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_lit15() */

/* void _poly_scanline_ptex_mask_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit15)
   #define INIT_CODE                      \
     movl POLYSEG_C(%esi), %eax ;         \
     movl POLYSEG_DC(%esi), %edx ;        \
     movl %eax, ALPHA ;                   \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_15)
   pushl %eax
   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit15() */



/* void _poly_scanline_ptex_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit16)
   #define INIT_CODE                      \
     movl POLYSEG_C(%esi), %eax ;         \
     movl POLYSEG_DC(%esi), %edx ;        \
     movl %eax, ALPHA ;                   \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movw (%esi, %eax, 2), %ax     /* read texel */
   pushl GLOBL(_blender_col_16)
   pushl %eax
   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_lit16() */

/* void _poly_scanline_ptex_mask_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit16)
   #define INIT_CODE                      \
     movl POLYSEG_C(%esi), %eax ;         \
     movl POLYSEG_DC(%esi), %edx ;        \
     movl %eax, ALPHA ;                   \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_16)
   pushl %eax
   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $2, %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_ptex_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit32)
   #define INIT_CODE                      \
     movl POLYSEG_C(%esi), %eax ;         \
     movl POLYSEG_DC(%esi), %edx ;        \
     movl %eax, ALPHA ;                   \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   movl (%esi, %eax, 4), %eax    /* read texel */
   pushl GLOBL(_blender_col_32)
   pushl %eax
   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   movl DALPHA, %eax
   addl $4, %edi
   addl %eax, ALPHA
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_lit32() */

/* void _poly_scanline_ptex_mask_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit32)
   #define INIT_CODE                      \
     movl POLYSEG_C(%esi), %eax ;         \
     movl POLYSEG_DC(%esi), %edx ;        \
     movl %eax, ALPHA ;                   \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_32)
   pushl %eax
   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   popl %edx
7:
   movl DALPHA, %eax
   addl $4, %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit32() */

#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_ptex_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_lit24)
   #define INIT_CODE                      \
     movl POLYSEG_C(%esi), %eax ;         \
     movl POLYSEG_DC(%esi), %edx ;        \
     movl %eax, ALPHA ;                   \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   leal (%eax, %eax, 2), %ecx
   movb 2(%esi, %ecx), %al       /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   pushl GLOBL(_blender_col_24)
   pushl %eax
   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   movl DALPHA, %eax
   addl $3, %edi
   addl %eax, ALPHA
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_lit24() */

/* void _poly_scanline_ptex_mask_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_lit24)
   #define INIT_CODE                      \
     movl POLYSEG_C(%esi), %eax ;         \
     movl POLYSEG_DC(%esi), %edx ;        \
     movl %eax, ALPHA ;                   \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_24)
   pushl %eax
   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   popl %edx
7:
   movl DALPHA, %eax
   addl $3, %edi
   addl %eax, ALPHA
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_lit24() */

#endif /* COLOR24 */



#ifdef ALLEGRO_COLOR8
/* void _poly_scanline_ptex_trans8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_trans8)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   shll $8, %eax
   pushl %edi
   movl READ_ADDR, %edi
   movb FSEG (%edi), %al
   popl %edi
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
   incl READ_ADDR
   incl %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_trans8() */

/* void _poly_scanline_ptex_mask_trans8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_trans8)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   orl %eax, %eax
   jz 7f
   shll $8, %eax
   pushl %edi
   movl READ_ADDR, %edi
   movb FSEG (%edi), %al
   popl %edi
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
7:
   incl READ_ADDR
   incl %edi
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_trans8() */

#endif /* COLOR8 */



#ifdef ALLEGRO_COLOR16
/* void _poly_scanline_ptex_trans15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_trans15)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %edi
   movl READ_ADDR, %edi
   movw FSEG (%edi), %cx
   popl %edi
   movw (%esi, %eax, 2), %ax     /* read texel */
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   addl $2, READ_ADDR
   addl $2, %edi
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_trans15() */

/* void _poly_scanline_ptex_mask_trans15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_trans15)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 7f
   pushl %edi
   movl READ_ADDR, %edi
   movw FSEG (%edi), %cx
   popl %edi
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
7:
   addl $2, %edi
   addl $2, READ_ADDR
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_trans15() */



/* void _poly_scanline_ptex_trans16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_trans16)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %edi
   movl READ_ADDR, %edi
   movw FSEG (%edi), %cx
   popl %edi
   movw (%esi, %eax, 2), %ax     /* read texel */
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   addl $2, READ_ADDR
   addl $2, %edi
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_trans16() */

/* void _poly_scanline_ptex_mask_trans16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_trans16)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 7f
   pushl %edi
   movl READ_ADDR, %edi
   movw FSEG (%edi), %cx
   popl %edi
   pushl %edx
   pushl GLOBL(_blender_alpha)
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
7:
   addl $2, %edi
   addl $2, READ_ADDR
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_trans16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_scanline_ptex_trans32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_trans32)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   pushl GLOBL(_blender_alpha)
   movl %edi, TMP
   movl READ_ADDR, %edi
   pushl FSEG (%edi)
   movl TMP, %edi
   pushl (%esi, %eax, 4)

   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   addl $4, READ_ADDR
   addl $4, %edi
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_trans32() */

/* void _poly_scanline_ptex_mask_trans32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_trans32)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 7f
   pushl %edx
   pushl GLOBL(_blender_alpha)
   movl %edi, TMP
   movl READ_ADDR, %edi
   pushl FSEG (%edi)
   movl TMP, %edi
   pushl %eax

   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   popl %edx
7:
   addl $4, %edi
   addl $4, READ_ADDR
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_trans32() */

#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_scanline_ptex_trans24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_trans24)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   pushl %edx
   pushl GLOBL(_blender_alpha)
   leal (%eax, %eax, 2), %ecx
   pushl %edi
   movl READ_ADDR, %edi
   movb FSEG 2(%edi), %dl
   movb 2(%esi, %ecx), %al       /* read texel */
   shll $16, %edx
   shll $16, %eax
   movw FSEG (%edi), %dx
   popl %edi
   movw (%esi, %ecx), %ax
   pushl %edx
   pushl %eax

   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   addl $3, READ_ADDR
   addl $3, %edi
   popl %edx
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_trans24() */

/* void _poly_scanline_ptex_mask_trans24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_scanline_ptex_mask_trans24)
   #define INIT_CODE                      \
     movl POLYSEG_RADDR(%esi) ,%eax ;     \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 7f
   movl %edi, TMP
   movl READ_ADDR, %edi
   movb FSEG 2(%edi), %cl
   pushl %edx
   pushl GLOBL(_blender_alpha)
   shll $16, %ecx
   movw FSEG (%edi), %cx
   movl TMP, %edi
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   popl %edx
7:
   addl $3, %edi
   addl $3, READ_ADDR
   END_PTEX()
   ret                           /* end of _poly_scanline_ptex_mask_trans24() */

#endif /* COLOR24 */
