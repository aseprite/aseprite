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
 *      Z-buffered polygon scanline filler helpers (gouraud shading, tmapping, etc).
 *      Uses MMX instructions when possible, but these will be #ifdef'ed
 *      out if your assembler doesn't support them.
 *
 *      By Calin Andrian.
 *
 *      Updated to Allegro 3.9.34 by Bertrand Coconnier.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"


.text

/* all these functions share the same parameters */

#define ADDR      ARG1
#define W         ARG2
#define INFO      ARG3

#define ZBUF      -4(%ebp)


/* first part of a flat routine */
#define INIT_FLAT(bpp, extra)                                         \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   movl ADDR, %edi                                                  ; \
   movl W, %ecx                                                     ; \
   movl INFO, %esi                                                  ; \
								    ; \
   flds POLYSEG_DZ(%esi)                                            ; \
   flds POLYSEG_Z(%esi)                                             ; \
   movl POLYSEG_C(%esi), %edx                                       ; \
   movl POLYSEG_ZBADDR(%esi), %esi                                  ; \
   extra                                                            ; \
                                                                    ; \
   subl $bpp, %edi                                                  ; \
   subl $4, %esi                                                    ; \
                                                                    ; \
   _align_                                                          ; \
1:                                                                  ; \
   fcoms 4(%esi)                                                    ; \
   addl $bpp, %edi                                                  ; \
   fnstsw %ax                                                       ; \
   addl $4, %esi                                                    ; \
   andb $0x41, %ah                                                  ; \
   jnz 2f                /* with the andb above, it means jbe */

/* end of flat routine */
#define END_FLAT()                                                    \
   fsts (%esi)                                                      ; \
2:                                                                  ; \
   fadd %st(1), %st(0)                                              ; \
   decl %ecx                                                        ; \
   jg 1b                                                            ; \
   fstp %st(0)                                                      ; \
   fstp %st(0)                                                      ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



#ifdef ALLEGRO_COLOR8
/* void _poly_zbuf_flat8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills a single-color flat polygon scanline.
 */
FUNC(_poly_zbuf_flat8)
   INIT_FLAT(1, /**/)
   movb %dl, FSEG(%edi)
   END_FLAT()
   ret                           /* end of _poly_zbuf_flat8() */
#endif



#ifdef ALLEGRO_COLOR16
/* void _poly_zbuf_flat16(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills a single-color flat polygon scanline.
 */
FUNC(_poly_zbuf_flat16)
   INIT_FLAT(2, /**/)
   movw %dx, FSEG(%edi)
   END_FLAT()
   ret                           /* end of _poly_zbuf_flat16() */
#endif



#ifdef ALLEGRO_COLOR32
/* void _poly_zbuf_flat32(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills a single-color flat polygon scanline.
 */
FUNC(_poly_zbuf_flat32)
   INIT_FLAT(4, /**/)
   movl %edx, FSEG(%edi)
   END_FLAT()
   ret                           /* end of _poly_zbuf_flat32() */
#endif



#ifdef ALLEGRO_COLOR24
/* void _poly_zbuf_flat24(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills a single-color flat polygon scanline.
 */
FUNC(_poly_zbuf_flat24)
   #define INIT_CODE        \
     movl %edx, %ebx ;      \
     shrl $16, %ebx
   INIT_FLAT(3, INIT_CODE)
   #undef INIT_CODE
   movw %dx, FSEG(%edi)
   movb %bl, FSEG 2(%edi)
   END_FLAT()
   ret                           /* end of _poly_zbuf_flat24() */
#endif



#define DB      -8(%ebp)
#define DG     -12(%ebp)
#define DR     -16(%ebp)

#ifdef ALLEGRO_COLOR8
/* void _poly_zbuf_gcol8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills a single-color gouraud shaded polygon scanline.
 */
FUNC(_poly_zbuf_gcol8)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   movl ADDR, %edi
   movl INFO, %esi
   movl W, %ecx
   flds POLYSEG_DZ(%esi)
   flds POLYSEG_Z(%esi)
   movl POLYSEG_C(%esi), %edx
   movl POLYSEG_DC(%esi), %ebx
   movl POLYSEG_ZBADDR(%esi), %esi

   subl %ebx, %edx
   decl %edi
   subl $4, %esi

   _align_
1:
   fcoms 4(%esi)
   addl %ebx, %edx
   addl $4, %esi
   fnstsw %ax
   incl %edi
   andb $0x41, %ah
   jnz 2f

   movl %edx, %eax
   shrl $16, %eax
   movb %al, FSEG(%edi)

   fsts (%esi)
2:
   fadd %st(1), %st(0)
   decl %ecx
   jg 1b
   fstp %st(0)
   fstp %st(0)
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_zbuf_gcol8() */



/* void _poly_zbuf_grgb8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_zbuf_grgb8)
   pushl %ebp
   movl %esp, %ebp
   subl $16, %esp
   pushl %ebx
   pushl %esi
   pushl %edi
   movl INFO, %esi

   flds POLYSEG_DZ(%esi)
   flds POLYSEG_Z(%esi)
   movl POLYSEG_ZBADDR(%esi), %ecx
   movl POLYSEG_DR(%esi), %eax
   movl %ecx, ZBUF
   movl POLYSEG_DG(%esi), %ebx
   movl POLYSEG_DB(%esi), %ecx

   movl %eax, DR
   movl %ebx, DG
   movl %ecx, DB

   movl POLYSEG_R(%esi), %ebx
   movl POLYSEG_G(%esi), %ecx
   movl POLYSEG_B(%esi), %edx

   movl ADDR, %edi
   subl DR, %ebx
   subl DG, %ecx
   subl DB, %edx
   decl %edi

   _align_
1:
   movl ZBUF, %esi
   addl DR, %ebx
   fcoms (%esi)
   addl DG, %ecx
   addl DB, %edx
   fnstsw %ax
   incl %edi
   andb $0x41, %ah
   jnz 2f

   movl %ecx, %eax               /* green */
   movl %ebx, %esi               /* red */
   shrl $19, %eax                /* green */
   shrl $19, %esi                /* red */
   shll $5, %eax                 /* green */
   shll $10, %esi                /* red */
   orl %eax, %esi                /* green */
   movl %edx, %eax               /* blue */
   shrl $19, %eax                /* blue */
   orl %eax, %esi                /* blue */

   movl GLOBL(rgb_map), %eax     /* table lookup */
   movb (%eax, %esi), %al
   movb %al, FSEG(%edi)          /* write the pixel */

   movl ZBUF, %esi
   fsts (%esi)
2:
   fadd %st(1), %st(0)
   addl $4, ZBUF
   decl W
   jg 1b

   fstp %st(0)
   fstp %st(0)
   popl %edi
   popl %esi
   popl %ebx
   movl %ebp, %esp
   popl %ebp
   ret                           /* end of _poly_zbuf_grgb8() */
#endif



/* first part of a grgb routine */
#define INIT_GRGB(depth, r_sft, g_sft, b_sft)                         \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $16, %esp                                                   ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   movl INFO, %esi               /* load registers */               ; \
								    ; \
   flds POLYSEG_DZ(%esi)                                            ; \
   flds POLYSEG_Z(%esi)                                             ; \
   movl POLYSEG_ZBADDR(%esi), %edx                                  ; \
   movl POLYSEG_DR(%esi), %ebx   /* move delta's on stack */        ; \
   movl %edx, ZBUF                                                  ; \
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
   subl DR, %ebx                                                    ; \
   subl DG, %edi                                                    ; \
   subl DB, %edx                                                    ; \
                                                                    ; \
   _align_                                                          ; \
1:                                                                  ; \
   movl ZBUF, %esi                                                  ; \
   addl DR, %ebx                                                    ; \
   fcoms (%esi)                                                     ; \
   addl DG, %edi                                                    ; \
   addl DB, %edx                                                    ; \
   fnstsw %ax                                                       ; \
   andb $0x41, %ah                                                  ; \
   jnz 2f                                                           ; \
                                                                    ; \
   movl %ebx, %eax              /* red */                           ; \
   movb GLOBL(_rgb_r_shift_##depth), %cl                            ; \
   shrl $r_sft, %eax                                                ; \
   shll %cl, %eax                                                   ; \
   movl %edi, %esi              /* green */                         ; \
   movb GLOBL(_rgb_g_shift_##depth), %cl                            ; \
   shrl $g_sft, %esi                                                ; \
   shll %cl, %esi                                                   ; \
   orl %esi, %eax                                                   ; \
   movl %edx, %esi              /* blue */                          ; \
   movb GLOBL(_rgb_b_shift_##depth), %cl                            ; \
   shrl $b_sft, %esi                                                ; \
   shll %cl, %esi                                                   ; \
   orl %esi, %eax                                                   ; \
   movl ADDR, %esi

/* end of grgb routine */
#define END_GRGB(bpp)                                                 \
   movl ZBUF, %esi                                                  ; \
   fsts (%esi)                                                      ; \
2:                                                                  ; \
   fadd %st(1), %st(0)                                              ; \
   addl $4, ZBUF                                                    ; \
   addl $bpp, ADDR                                                  ; \
   decl W                                                           ; \
   jg 1b                                                            ; \
								    ; \
   fstp %st(0)                                                      ; \
   fstp %st(0)                                                      ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



#ifdef ALLEGRO_COLOR16
/* void _poly_zbuf_grgb15(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_zbuf_grgb15)
   INIT_GRGB(15, 19, 19, 19)
   movw %ax, FSEG(%esi)          /* write the pixel */
   END_GRGB(2)
   ret                           /* end of _poly_zbuf_grgb15() */



/* void _poly_zbuf_grgb16(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_zbuf_grgb16)
   INIT_GRGB(16, 19, 18, 19)
   movw %ax, FSEG(%esi)          /* write the pixel */
   END_GRGB(2)
   ret                           /* end of _poly_zbuf_grgb16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_zbuf_grgb32(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_zbuf_grgb32)
   INIT_GRGB(32, 16, 16, 16)
   movl %eax, FSEG(%esi)         /* write the pixel */
   END_GRGB(4)
   ret                           /* end of _poly_zbuf_grgb32() */

#endif /* COLOR32 */

#ifdef ALLEGRO_COLOR24
/* void _poly_zbuf_grgb24(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an RGB gouraud shaded polygon scanline.
 */
FUNC(_poly_zbuf_grgb24)
   INIT_GRGB(24, 16, 16, 16)
   movw %ax, FSEG(%esi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%esi)
   END_GRGB(3)
   ret                           /* end of _poly_zbuf_grgb24() */

#endif /* COLOR24 */

#undef DR
#undef DG
#undef DB



#define VMASK      -8(%ebp)
#define VSHIFT    -12(%ebp)
#define DV        -16(%ebp)
#define DU        -20(%ebp)
#define ALPHA2    -22(%ebp)
#define ALPHA     -24(%ebp)
#define DALPHA    -28(%ebp)
#define UMASK     -32(%ebp)
#define READ_ADDR -36(%ebp)

/* first part of an affine texture mapping operation */
#define INIT_ATEX(extra)                                              \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $36, %esp                    /* local variables */          ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   movl INFO, %esi               /* load registers */               ; \
   extra                                                            ; \
								    ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                  ; \
   movl POLYSEG_VMASK(%esi), %eax                                   ; \
   movl POLYSEG_ZBADDR(%esi), %ebx                                  ; \
   shll %cl, %eax           /* adjust v mask and shift value */     ; \
   negl %ecx                                                        ; \
   movl %ebx, ZBUF                                                  ; \
   addl $16, %ecx                                                   ; \
   movl %eax, VMASK                                                 ; \
   movl %ecx, VSHIFT                                                ; \
								    ; \
   movl POLYSEG_UMASK(%esi), %eax                                   ; \
   movl POLYSEG_DU(%esi), %ebx                                      ; \
   movl POLYSEG_DV(%esi), %edx                                      ; \
   movl %eax, UMASK                                                 ; \
   movl %ebx, DU                                                    ; \
   movl %edx, DV                                                    ; \
								    ; \
   flds POLYSEG_DZ(%esi)                                            ; \
   flds POLYSEG_Z(%esi)                                             ; \
   movl POLYSEG_U(%esi), %ebx                                       ; \
   movl POLYSEG_V(%esi), %edx                                       ; \
   movl ADDR, %edi                                                  ; \
   movl POLYSEG_TEXTURE(%esi), %esi                                 ; \
								    ; \
   subl DU, %ebx                                                    ; \
   subl DV, %edx                                                    ; \
                                                                    ; \
   _align_                                                         ; \
1:                                                                  ; \
   movl ZBUF, %ecx                                                  ; \
   addl DU, %ebx                                                    ; \
   fcoms (%ecx)                                                     ; \
   addl DV, %edx                                                    ; \
   fnstsw %ax                                                       ; \
   andb $0x41, %ah                                                  ; \
   jnz 2f                                                           ; \
                                                                    ; \
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
#define END_ATEX(extra)                                               \
   movl ZBUF, %ecx                                                  ; \
   fsts (%ecx)                                                      ; \
2:                                                                  ; \
   fadd %st(1), %st(0)                                              ; \
   addl $4, ZBUF                                                    ; \
   extra                                                            ; \
   decl W                                                           ; \
   jg 1b                                                            ; \
								    ; \
   fstp %st(0)                                                      ; \
   fstp %st(0)                                                      ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



#ifdef ALLEGRO_COLOR8
/* void _poly_zbuf_atex8(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex8)
   INIT_ATEX(/**/)
   movb (%esi, %eax), %al        /* read texel */
   movb %al, FSEG(%edi)          /* write the pixel */
   END_ATEX(incl %edi)
   ret                           /* end of _poly_zbuf_atex8() */

/* void _poly_zbuf_atex_mask8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask8)
   INIT_ATEX(/**/)
   movb (%esi, %eax), %al        /* read texel */
   orb %al, %al
   jz 2f
   movb %al, FSEG(%edi)          /* write solid pixels */
   END_ATEX(incl %edi)
   ret                           /* end of _poly_zbuf_atex_mask8() */
#endif



#ifdef ALLEGRO_COLOR16
/* void _poly_zbuf_atex16(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex16)
   INIT_ATEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   #define END_CODE                 \
     addl $2, %edi
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex16() */

/* void _poly_zbuf_atex_mask15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask15)
   INIT_ATEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 2f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   #define END_CODE                 \
     addl $2, %edi
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask15() */

/* void _poly_zbuf_atex_mask16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask16)
   INIT_ATEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 2f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   #define END_CODE                 \
     addl $2, %edi
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask16() */
#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_zbuf_atex32(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex32)
   INIT_ATEX(/**/)
   movl (%esi, %eax, 4), %eax    /* read texel */
   movl %eax, FSEG(%edi)         /* write the pixel */
   #define END_CODE                 \
     addl $4, %edi
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex32() */

/* void _poly_zbuf_atex_mask32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask32)
   INIT_ATEX(/**/)
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 2f
   movl %eax, FSEG(%edi)         /* write solid pixels */
   #define END_CODE                 \
     addl $4, %edi
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask32() */
#endif /* COLOR32 */

#ifdef ALLEGRO_COLOR24
/* void _poly_zbuf_atex24(unsigned long addr, int w, POLYGON_SEGMENT *info);
 *  Fills an affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex24)
   INIT_ATEX(/**/)
   leal (%eax, %eax, 2), %ecx
   movw (%esi, %ecx), %ax        /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   movb 2(%esi, %ecx), %al       /* read texel */
   movb %al, FSEG 2(%edi)         /* write the pixel */
   #define END_CODE                 \
     addl $3, %edi
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex24() */

/* void _poly_zbuf_atex_mask24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask24)
   INIT_ATEX(/**/)
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 2f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   #define END_CODE                 \
     addl $3, %edi
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask24() */
#endif /* COLOR24 */



#ifdef ALLEGRO_COLOR8
/* void _poly_zbuf_atex_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_lit8)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   movb ALPHA2, %ah
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     incl %edi ;                    \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_lit8() */

/* void _poly_zbuf_atex_mask_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_lit8)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   orl %eax, %eax
   jz 2f
   movb ALPHA2, %ah
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     incl %edi ;                    \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_lit8() */
#endif



#ifdef ALLEGRO_COLOR16
/* void _poly_zbuf_atex_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_lit15)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
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
   popl %edx
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     addl $2, %edi ;                \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_lit15() */

/* void _poly_zbuf_atex_mask_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_lit15)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax    /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 2f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_15)
   pushl %eax

   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)         /* write the pixel */
   popl %edx
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     addl $2, %edi ;                \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_lit15() */



/* void _poly_zbuf_atex_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_lit16)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
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
   popl %edx
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     addl $2, %edi ;                \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_lit16() */

/* void _poly_zbuf_atex_mask_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_lit16)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 2f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_16)
   pushl %eax

   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     addl $2, %edi ;                \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_lit16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_zbuf_atex_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_lit32)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
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
   popl %edx
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     addl $4, %edi ;                \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_lit32() */

/* void _poly_zbuf_atex_mask_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_lit32)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 2f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_32)
   pushl %eax

   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   popl %edx
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     addl $4, %edi ;                \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_lit32() */

#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_zbuf_atex_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_lit24)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
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
   popl %edx
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     addl $3, %edi ;                \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_lit24() */

/* void _poly_zbuf_atex_mask_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_lit24)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 2f
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
   #define END_CODE                 \
     movl DALPHA, %eax ;            \
     addl $3, %edi ;                \
     addl %eax, ALPHA
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_lit24() */

#endif /* COLOR24 */




#ifdef ALLEGRO_COLOR8
/* void _poly_zbuf_atex_trans8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_trans8)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
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
   #define END_CODE                 \
     incl %edi ;                    \
     incl READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_trans8() */

/* void _poly_zbuf_atex_mask_trans8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_trans8)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   orl %eax, %eax
   jz 2f
   shll $8, %eax
   pushl %edi
   movl READ_ADDR, %edi
   movb FSEG (%edi), %al
   popl %edi
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
   #define END_CODE                 \
     incl %edi ;                    \
     incl READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_trans8() */
#endif



#ifdef ALLEGRO_COLOR16
/* void _poly_zbuf_atex_trans15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_trans15)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
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
   popl %edx
   #define END_CODE                 \
     addl $2, %edi ;                \
     addl $2, READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_trans15() */

/* void _poly_zbuf_atex_mask_trans15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_trans15)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax    /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 2f
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
   #define END_CODE                 \
     addl $2, %edi ;                \
     addl $2, READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_trans15() */



/* void _poly_zbuf_atex_trans16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_trans16)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
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
   popl %edx
   #define END_CODE                 \
     addl $2, %edi ;                \
     addl $2, READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_trans16() */

/* void _poly_zbuf_atex_mask_trans16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_trans16)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 2f
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
   #define END_CODE                 \
     addl $2, %edi ;                \
     addl $2, READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_trans16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_zbuf_atex_trans32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_trans32)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
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
   popl %edx
   #define END_CODE                 \
     addl $4, %edi ;                \
     addl $4, READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_trans32() */

/* void _poly_zbuf_atex_mask_trans32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_trans32)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 2f
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
   #define END_CODE                 \
     addl $4, %edi ;                \
     addl $4, READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_trans32() */

#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_zbuf_atex_trans24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_trans24)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
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
   popl %edx
   #define END_CODE                 \
     addl $3, %edi ;                \
     addl $3, READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_trans24() */

/* void _poly_zbuf_atex_mask_trans24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans affine texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_atex_mask_trans24)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_ATEX(INIT_CODE)
   #undef INIT_CODE
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 2f
   movl %edi, ALPHA
   movl READ_ADDR, %edi
   movb FSEG 2(%edi), %cl
   pushl %edx
   movl ALPHA, %edi
   pushl GLOBL(_blender_alpha)
   shll $16, %ecx
   movw FSEG (%edi), %cx
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   popl %edx
   #define END_CODE                 \
     addl $3, %edi ;                \
     addl $3, READ_ADDR
   END_ATEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_atex_mask_trans24() */

#endif /* COLOR24 */



#undef VMASK
#undef VSHIFT
#undef UMASK
#undef DU
#undef DV
#undef ALPHA
#undef ALPHA2
#undef DALPHA
#undef READ_ADDR



#define VMASK      -8(%ebp)
#define VSHIFT    -12(%ebp)
#define ALPHA     -16(%ebp)
#define DALPHA    -20(%ebp)
#define U1        -24(%ebp)
#define V1        -28(%ebp)
#define DU        -32(%ebp)
#define DV        -36(%ebp)
#define DZ        -40(%ebp)
#define DU4       -44(%ebp)
#define DV4       -48(%ebp)
#define DZ4       -52(%ebp)
#define UMASK     -56(%ebp)
#define COUNT     -60(%ebp)
#define READ_ADDR -64(%ebp)

/* helper for starting an fpu 1/z division */
#define START_FP_DIV()                                                \
   fld1                                                             ; \
   fdiv %st(1), %st(0)

/* helper for ending an fpu division, returning corrected u and v values */
#define END_FP_DIV()                                                  \
   fst %st(1)                    /* duplicate the 1/z value */      ; \
   fmul %st(3), %st(0)           /* divide u by z */                ; \
   fxch %st(1)                                                      ; \
   fmul %st(2), %st(0)           /* divide v by z */

#define UPDATE_FP_POS_4()                                             \
   fadds DV4                     /* update v coord */               ; \
   fxch %st(1)                   /* swap vuz stack to uvz */        ; \
   fadds DU4                     /* update u coord */               ; \
   fxch %st(1)                   /* swap uvz stack back to vuz */   ; \
   fld %st(2)                    /* zvuz */                         ; \
   fadds DZ4                     /* update z value */

/* main body of the perspective-correct texture mapping routine */
#define INIT_PTEX(extra)                                              \
   pushl %ebp                                                       ; \
   movl %esp, %ebp                                                  ; \
   subl $64, %esp                /* local variables */              ; \
   pushl %ebx                                                       ; \
   pushl %esi                                                       ; \
   pushl %edi                                                       ; \
								    ; \
   movl INFO, %esi               /* load registers */               ; \
								    ; \
   flds POLYSEG_DFU(%esi)        /* multiply diffs by four */       ; \
   flds POLYSEG_DFV(%esi)                                           ; \
   flds POLYSEG_DZ(%esi)                                            ; \
   fsts DZ                                                          ; \
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
   flds POLYSEG_Z(%esi)          /* z at bottom of fpu stack */     ; \
   flds POLYSEG_FU(%esi)         /* followed by u */                ; \
   flds POLYSEG_FV(%esi)         /* followed by v */                ; \
   fld %st(2)                    /* zvuz */                         ; \
								    ; \
   START_FP_DIV()                                                   ; \
								    ; \
   extra                                                            ; \
								    ; \
   movl POLYSEG_ZBADDR(%esi), %edx                                  ; \
   movl POLYSEG_VSHIFT(%esi), %ecx                                  ; \
   movl %edx, ZBUF                                                  ; \
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
   _align_                                                         ; \
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
4:                              /* step 3 & 4: next U, V */         ; \
   addl DV, %edx                                                    ; \
   addl DU, %ebx                                                    ; \
3:                              /* step 1: just use U and V */      ; \
   movl ZBUF, %ecx                                                  ; \
   fxch %st(4)                                                      ; \
   fcoms (%ecx)                                                     ; \
   fnstsw %ax                                                       ; \
   andb $0x41, %ah                                                  ; \
   jnz 2f                                                           ; \
                                                                    ; \
   movl %edx, %eax              /* get v */                         ; \
   movb VSHIFT, %cl                                                 ; \
   sarl %cl, %eax                                                   ; \
   andl VMASK, %eax                                                 ; \
								    ; \
   movl %ebx, %ecx              /* get u */                         ; \
   sarl $16, %ecx                                                   ; \
   andl UMASK, %ecx                                                 ; \
   addl %ecx, %eax

#define END_PTEX(extra)                                               \
   movl ZBUF, %ecx                                                  ; \
   fsts (%ecx)                                                      ; \
2:                                                                  ; \
   fadds DZ                                                         ; \
   fxch %st(4)                                                      ; \
   addl $4, ZBUF                                                    ; \
                                                                    ; \
   extra                                                            ; \
   decl W                                                           ; \
   jle 6f                                                           ; \
   decl COUNT                                                       ; \
   jg 4b                                                            ; \
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
   fstp %st(0)                                                      ; \
								    ; \
   popl %edi                                                        ; \
   popl %esi                                                        ; \
   popl %ebx                                                        ; \
   movl %ebp, %esp                                                  ; \
   popl %ebp



#ifdef ALLEGRO_COLOR8
/* void _poly_zbuf_ptex8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex8)
   INIT_PTEX(/**/)
   movb (%esi, %eax), %al        /* read texel */
   movb %al, FSEG(%edi)          /* write the pixel */
   END_PTEX(incl %edi)
   ret                           /* end of _poly_zbuf_ptex8() */

/* void _poly_zbuf_ptex_mask8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask8)
   INIT_PTEX(/**/)
   movb (%esi, %eax), %al        /* read texel */
   orb %al, %al
   jz 2f
   movb %al, FSEG(%edi)          /* write solid pixels */
   END_PTEX(incl %edi)
   ret                           /* end of _poly_zbuf_ptex_mask8() */
#endif



#ifdef ALLEGRO_COLOR16
/* void _poly_zbuf_ptex16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex16)
   INIT_PTEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   #define END_CODE          \
     addl $2, %edi
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex16() */

/* void _poly_zbuf_ptex_mask15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask15)
   INIT_PTEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 2f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   #define END_CODE          \
     addl $2, %edi
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask15() */

/* void _poly_zbuf_ptex_mask16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask16)
   INIT_PTEX(/**/)
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 2f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   #define END_CODE          \
     addl $2, %edi
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask16() */
#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_zbuf_ptex32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex32)
   INIT_PTEX(/**/)
   movl (%esi, %eax, 4), %eax    /* read texel */
   movl %eax, FSEG(%edi)         /* write the pixel */
   #define END_CODE          \
     addl $4, %edi
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex32() */

/* void _poly_zbuf_ptex_mask32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask32)
   INIT_PTEX(/**/)
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 2f
   movl %eax, FSEG(%edi)         /* write solid pixels */
   #define END_CODE          \
     addl $4, %edi
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask32() */
#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_zbuf_ptex24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex24)
   INIT_PTEX(/**/)
   leal (%eax, %eax, 2), %ecx
   movw (%esi, %ecx), %ax        /* read texel */
   movw %ax, FSEG(%edi)          /* write the pixel */
   movb 2(%esi, %ecx), %al       /* read texel */
   movb %al, FSEG 2(%edi)         /* write the pixel */
   #define END_CODE          \
     addl $3, %edi
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex24() */

/* void _poly_zbuf_ptex_mask24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a masked perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask24)
   INIT_PTEX(/**/)
   leal (%eax, %eax, 2), %ecx
   xorl %eax, %eax
   movb 2(%esi, %ecx), %al       /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 2f
   movw %ax, FSEG(%edi)          /* write solid pixels */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   #define END_CODE          \
     addl $3, %edi
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask24() */
#endif /* COLOR24 */



#ifdef ALLEGRO_COLOR8
/* void _poly_zbuf_ptex_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_lit8)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   movb 2+ALPHA, %ah
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     incl %edi ;             \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_lit8() */

/* void _poly_zbuf_ptex_mask_lit8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_lit8)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   orl %eax, %eax
   jz 2f
   movb 2+ALPHA, %ah
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     incl %edi ;             \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_lit8() */
#endif



#ifdef ALLEGRO_COLOR16
/* void _poly_zbuf_ptex_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_lit15)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
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
   popl %edx
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     addl $2, %edi ;         \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_lit15() */

/* void _poly_zbuf_ptex_mask_lit15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_lit15)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 2f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_15)
   pushl %eax

   call *GLOBL(_blender_func15)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     addl $2, %edi ;         \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_lit15() */



/* void _poly_zbuf_ptex_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_lit16)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
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
   popl %edx
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     addl $2, %edi ;         \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_lit16() */

/* void _poly_zbuf_ptex_mask_lit16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_lit16)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 2f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_16)
   pushl %eax

   call *GLOBL(_blender_func16)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   popl %edx
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     addl $2, %edi ;         \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_lit16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_zbuf_ptex_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_lit32)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
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
   popl %edx
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     addl $4, %edi ;         \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_lit32() */

/* void _poly_zbuf_ptex_mask_lit32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_lit32)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 2f
   pushl %edx
   movzbl 2+ALPHA, %edx
   pushl %edx
   pushl GLOBL(_blender_col_32)
   pushl %eax

   call *GLOBL(_blender_func32)
   addl $12, %esp

   movl %eax, FSEG(%edi)         /* write the pixel */
   popl %edx
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     addl $4, %edi ;         \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_lit32() */

#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_zbuf_ptex_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_lit24)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
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

   call *GLOBL(_blender_func32)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   popl %edx
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     addl $3, %edi ;         \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_lit24() */

/* void _poly_zbuf_ptex_mask_lit24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a lit perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_lit24)
   #define INIT_CODE                 \
     movl POLYSEG_C(%esi), %eax ;    \
     movl POLYSEG_DC(%esi), %edx ;   \
     movl %eax, ALPHA ;              \
     movl %edx, DALPHA
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 2f
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
   #define END_CODE          \
     movl DALPHA, %eax ;     \
     addl $3, %edi ;         \
     addl %eax, ALPHA
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_lit24() */

#endif /* COLOR24 */



#ifdef ALLEGRO_COLOR8
/* void _poly_zbuf_ptex_trans8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_trans8)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
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
   #define END_CODE          \
     incl %edi ;             \
     incl READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_trans8() */

/* void _poly_zbuf_ptex_mask_trans8(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_trans8)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movzbl (%esi, %eax), %eax     /* read texel */
   orl %eax, %eax
   jz 2f
   shll $8, %eax
   pushl %edi
   movl READ_ADDR, %edi
   movb FSEG (%edi), %al
   popl %edi
   movl GLOBL(color_map), %ecx
   movb (%ecx, %eax), %al
   movb %al, FSEG(%edi)
   #define END_CODE          \
     incl %edi ;             \
     incl READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_trans8() */
#endif


#ifdef ALLEGRO_COLOR16
/* void _poly_zbuf_ptex_trans15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_trans15)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
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
   popl %edx
   #define END_CODE          \
     addl $2, %edi ;         \
     addl $2, READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_trans15() */

/* void _poly_zbuf_ptex_mask_trans15(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_trans15)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax    /* read texel */
   cmpw $MASK_COLOR_15, %ax
   jz 2f
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
   #define END_CODE          \
     addl $2, %edi ;         \
     addl $2, READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_trans15() */



/* void _poly_zbuf_ptex_trans16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_trans16)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
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
   popl %edx
   #define END_CODE          \
     addl $2, %edi ;         \
     addl $2, READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_trans16() */

/* void _poly_zbuf_ptex_mask_trans16(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_trans16)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movw (%esi, %eax, 2), %ax     /* read texel */
   cmpw $MASK_COLOR_16, %ax
   jz 2f
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
   #define END_CODE          \
     addl $2, %edi ;         \
     addl $2, READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_trans16() */

#endif /* COLOR16 */



#ifdef ALLEGRO_COLOR32
/* void _poly_zbuf_ptex_trans32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_trans32)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
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
   popl %edx
   #define END_CODE          \
     addl $4, %edi ;         \
     addl $4, READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_trans32() */

/* void _poly_zbuf_ptex_mask_trans32(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_trans32)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   movl (%esi, %eax, 4), %eax    /* read texel */
   cmpl $MASK_COLOR_32, %eax
   jz 2f
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
   #define END_CODE          \
     addl $4, %edi ;         \
     addl $4, READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_trans32() */

#endif /* COLOR32 */



#ifdef ALLEGRO_COLOR24
/* void _poly_zbuf_ptex_trans24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_trans24)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
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
   popl %edx
   #define END_CODE          \
     addl $3, %edi ;         \
     addl $3, READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_trans24() */

/* void _poly_zbuf_ptex_mask_trans24(ulong addr, int w, POLYGON_SEGMENT *info);
 *  Fills a trans perspective correct texture mapped polygon scanline.
 */
FUNC(_poly_zbuf_ptex_mask_trans24)
   #define INIT_CODE                  \
     movl POLYSEG_RADDR(%esi), %eax ; \
     movl %eax, READ_ADDR
   INIT_PTEX(INIT_CODE)
   #undef INIT_CODE
   leal (%eax, %eax, 2), %ecx
   movzbl 2(%esi, %ecx), %eax    /* read texel */
   shll $16, %eax
   movw (%esi, %ecx), %ax
   cmpl $MASK_COLOR_24, %eax
   jz 2f
   movl %edi, ALPHA
   movl READ_ADDR, %edi
   movb FSEG 2(%edi), %cl
   pushl %edx
   pushl GLOBL(_blender_alpha)
   shll $16, %ecx
   movw FSEG (%edi), %cx
   movl ALPHA, %edi
   pushl %ecx
   pushl %eax

   call *GLOBL(_blender_func24)
   addl $12, %esp

   movw %ax, FSEG(%edi)          /* write the pixel */
   shrl $16, %eax
   movb %al, FSEG 2(%edi)
   popl %edx
   #define END_CODE          \
     addl $3, %edi ;         \
     addl $3, READ_ADDR
   END_PTEX(END_CODE)
   #undef END_CODE
   ret                           /* end of _poly_zbuf_ptex_mask_trans24() */

#endif /* COLOR24 */
