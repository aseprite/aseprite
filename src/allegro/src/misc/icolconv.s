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
 *      Asm routines for software color conversion.
 *      Suggestions to make it faster are welcome :)
 *
 *      By Isaac Cruz.
 *
 *      24-bit color support and non MMX routines by Eric Botcazou.
 *
 *      Support for rectangles of any width, 8-bit destination color
 *      and cross-conversion between 15-bit and 16-bit colors,
 *      additional MMX and color copy routines by Robert J. Ohannessian.
 *
 *      See readme.txt for copyright information.
 */


#include "src/i386/asmdefs.inc"


/* Better keep the stack pointer 16-byte aligned on modern CPUs */
#if !defined(__GNUC__) || (__GNUC__ < 3)
#define SLOTS_TO_BYTES(n) (n*4)
#else
#define SLOTS_TO_BYTES(n) (((n*4+15)>>4)*16)
#endif



.text


#ifdef ALLEGRO_MMX

/* it seems pand is broken in GAS 2.8.1 */
#define PAND(src, dst)   \
   .byte 0x0f, 0xdb    ; \
   .byte 0xc0 + 8*dst + src  /* mod field */

/* local variables */
#define RESERVE_LOCALS(n) subl $SLOTS_TO_BYTES(n), %esp
#define CLEANUP_LOCALS(n) addl $SLOTS_TO_BYTES(n), %esp
#define LOCAL1   0(%esp)
#define LOCAL2   4(%esp)
#define LOCAL3   8(%esp)
#define LOCAL4  12(%esp)

/* helper macros */
#define INIT_CONVERSION_1(mask_red, mask_green, mask_blue)                           \
      /* init register values */                                                   ; \
                                                                                   ; \
      movl mask_green, %esi                                                        ; \
      movd %esi, %mm3                                                              ; \
      punpckldq %mm3, %mm3                                                         ; \
      movl mask_red, %esi                                                          ; \
      movd %esi, %mm4                                                              ; \
      punpckldq %mm4, %mm4                                                         ; \
      movl mask_blue, %esi                                                         ; \
      movd %esi, %mm5                                                              ; \
      punpckldq %mm5, %mm5                                                         ; \
                                                                                   ; \
      movl ARG1, %esi                  /* esi = src_rect                 */        ; \
      movl GFXRECT_WIDTH(%esi), %ecx   /* ecx = src_rect->width          */        ; \
      movl GFXRECT_HEIGHT(%esi), %edx  /* edx = src_rect->height         */        ; \
      movl GFXRECT_PITCH(%esi), %eax   /* eax = src_rect->pitch          */        ; \
      movl GFXRECT_DATA(%esi), %esi    /* esi = src_rect->data           */        ; \
      shll $2, %ecx                    /* ecx = SCREEN_W * 4             */        ; \
      subl %ecx, %eax                  /* eax = (src_rect->pitch) - ecx  */        ; \
                                                                                   ; \
      movl ARG2, %edi                  /* edi = dest_rect                */        ; \
      shrl $1, %ecx                    /* ecx = SCREEN_W * 2             */        ; \
      movl GFXRECT_PITCH(%edi), %ebx   /* ebx = dest_rect->pitch         */        ; \
      movl GFXRECT_DATA(%edi), %edi    /* edi = dest_rect->data          */        ; \
      subl %ecx, %ebx                  /* ebx = (dest_rect->pitch) - ecx */        ; \
      shrl $1, %ecx                    /* ecx = SCREEN_W                 */        ; \
      movl %ecx, %ebp


#define INIT_CONVERSION_2(mask_red, mask_green, mask_blue)                           \
      /* init register values */                                                   ; \
                                                                                   ; \
      movl mask_green, %esi                                                        ; \
      movd %esi, %mm3                                                              ; \
      punpckldq %mm3, %mm3                                                         ; \
      movl mask_red, %esi                                                          ; \
      movd %esi, %mm4                                                              ; \
      punpckldq %mm4, %mm4                                                         ; \
      movl mask_blue, %esi                                                         ; \
      movd %esi, %mm5                                                              ; \
      punpckldq %mm5, %mm5                                                         ; \
                                                                                   ; \
      movl ARG1, %esi                  /* esi = src_rect                 */        ; \
      movl GFXRECT_WIDTH(%esi), %ecx   /* ecx = src_rect->width          */        ; \
      movl GFXRECT_HEIGHT(%esi), %edx  /* edx = src_rect->height         */        ; \
      movl GFXRECT_PITCH(%esi), %eax   /* eax = src_rect->pitch          */        ; \
      movl GFXRECT_DATA(%esi), %esi    /* esi = src_rect->data           */        ; \
      addl %ecx, %ecx                  /* ecx = SCREEN_W * 2             */        ; \
      subl %ecx, %eax                  /* eax = (src_rect->pitch) - ecx  */        ; \
                                                                                   ; \
      movl ARG2, %edi                  /* edi = dest_rect                */        ; \
      addl %ecx, %ecx                  /* ecx = SCREEN_W * 4             */        ; \
      movl GFXRECT_PITCH(%edi), %ebx   /* ebx = dest_rect->pitch         */        ; \
      movl GFXRECT_DATA(%edi), %edi    /* edi = dest_rect->data          */        ; \
      subl %ecx, %ebx                  /* ebx = (dest_rect->pitch) - ecx */        ; \
      shrl $2, %ecx                    /* ecx = SCREEN_W                 */        ; \
      movl %ecx, %ebp



#ifdef ALLEGRO_COLOR8

/* void _colorconv_blit_8_to_15 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT dest_rect)
 */
/* void _colorconv_blit_8_to_16 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT dest_rect)
 */
FUNC (_colorconv_blit_8_to_15)
FUNC (_colorconv_blit_8_to_16)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_8_to_16_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   RESERVE_LOCALS(4)

   /* init register values */

   movl ARG1, %esi                    /* esi = src_rect         */
   movl GFXRECT_WIDTH(%esi), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%esi), %eax    /* eax = src_rect->height */
   movl GFXRECT_PITCH(%esi), %ebx     /* ebx = src_rect->pitch  */
   movl GFXRECT_DATA(%esi), %esi      /* esi = src_rect->data   */
   movl %eax, LOCAL1                  /* LOCAL1 = SCREEN_H      */
   subl %ecx, %ebx
   movl %ebx, LOCAL2                  /* LOCAL2 = src_rect->pitch - SCREEN_W */

   movl ARG2, %edi                    /* edi = dest_rect        */
   addl %ecx, %ecx                    /* ecx = SCREEN_W * 2     */
   movl GFXRECT_PITCH(%edi), %edx     /* edx = dest_rect->pitch */
   movl GFXRECT_DATA(%edi), %edi      /* edi = dest_rect->data  */
   subl %ecx, %edx
   movl %edx, LOCAL3                  /* LOCAL3 = (dest_rect->pitch) - (SCREEN_W * 2) */
   shrl $1, %ecx                      /* ecx = SCREEN_W                               */
   movl GLOBL(_colorconv_indexed_palette), %ebp  /* ebp = _colorconv_indexed_palette  */
   movl %ecx, %ebx

   /* 8 bit to 16 bit conversion:
    we have:
    eax = free
    ebx = SCREEN_W
    ecx = SCREEN_W
    edx = free
    esi = src_rect->data
    edi = dest_rect->data
    ebp = _colorconv_indexed_palette
    LOCAL1 = SCREEN_H
    LOCAL2 = offset from the end of a line to the beginning of the next
    LOCAL3 = same as LOCAL2, but for the dest bitmap
   */

   _align_
   next_line_8_to_16:
      shrl $2, %ecx             /* work with packs of 4 pixels */

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      jz do_one_pixel_8_to_16  /* less than 4 pixels? Can't work with the main loop */
#endif

      _align_
      next_block_8_to_16:
         movl (%esi), %edx         /* edx = [4][3][2][1] */
         movzbl %dl, %eax
         movd (%ebp,%eax,4), %mm0  /* mm0 = xxxxxxxxxx xxxxx[ 1 ] */
         shrl $8, %edx
         movzbl %dl, %eax
         movd (%ebp,%eax,4), %mm1  /* mm1 = xxxxxxxxxx xxxxx[ 2 ] */
         punpcklwd %mm1, %mm0      /* mm0 = xxxxxxxxxx [ 2 ][ 1 ] */
         shrl $8, %edx
         movzbl %dl, %eax
         movd (%ebp,%eax,4), %mm2  /* mm2 = xxxxxxxxxx xxxxx[ 3 ] */
         shrl $8, %edx
         movl %edx, %eax
         movd (%ebp,%eax,4), %mm3  /* mm3 = xxxxxxxxxx xxxxx[ 4 ] */
         punpcklwd %mm3, %mm2      /* mm2 = xxxxxxxxxx [ 4 ][ 3 ] */
         addl $4, %esi
         punpckldq %mm2, %mm0      /* mm0 = [ 4 ][ 3 ] [ 2 ][ 1 ] */
         movq %mm0, (%edi)
         addl $8, %edi

         decl %ecx
         jnz next_block_8_to_16

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_8_to_16:
         movl %ebx, %ecx           /* restore width */
         andl $3, %ecx
         jz end_of_line_8_to_16    /* nothing to do? */

         shrl $1, %ecx
         jnc do_two_pixels_8_to_16

         movzbl (%esi), %edx        /* convert 1 pixel */
         movl (%ebp,%edx,4), %eax
         incl %esi
         addl $2, %edi
         movw %ax, -2(%edi)

      do_two_pixels_8_to_16:
         shrl $1, %ecx
         jnc end_of_line_8_to_16
         movzbl (%esi), %edx        /* convert 2 pixels */
         movzbl 1(%esi), %eax
         movl (%ebp,%edx,4), %edx
         movl (%ebp,%eax,4), %eax
         shll $16, %eax
         addl $2, %esi
         orl %eax, %edx
         addl $4, %edi
         movl %edx, -4(%edi)

   _align_
   end_of_line_8_to_16:
#endif

      movl LOCAL2, %edx
      addl %edx, %esi
      movl LOCAL3, %eax
      addl %eax, %edi
      movl LOCAL1, %edx
      movl %ebx, %ecx             /* restore width */
      decl %edx
      movl %edx, LOCAL1
      jnz next_line_8_to_16

   CLEANUP_LOCALS(4)

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret



/* void _colorconv_blit_8_to_32 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_8_to_32)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_8_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   RESERVE_LOCALS(4)

   /* init register values */

   movl ARG1, %esi                    /* esi = src_rect         */
   movl GFXRECT_WIDTH(%esi), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%esi), %eax    /* eax = src_rect->height */
   movl GFXRECT_PITCH(%esi), %ebx     /* ebx = src_rect->pitch  */
   movl GFXRECT_DATA(%esi), %esi      /* esi = src_rect->data   */
   movl %eax, LOCAL1                  /* LOCAL1 = SCREEN_H      */
   subl %ecx, %ebx
   movl %ebx, LOCAL2                  /* LOCAL2 = src_rect->pitch - SCREEN_W */

   movl ARG2, %edi                    /* edi = dest_rect        */
   shll $2, %ecx                      /* ecx = SCREEN_W * 4     */
   movl GFXRECT_PITCH(%edi), %edx     /* edx = dest_rect->pitch */
   movl GFXRECT_DATA(%edi), %edi      /* edi = dest_rect->data  */
   subl %ecx, %edx
   movl %edx, LOCAL3                  /* LOCAL3 = (dest_rect->pitch) - (SCREEN_W * 4) */
   shrl $2, %ecx                      /* ecx = SCREEN_W                               */
   movl GLOBL(_colorconv_indexed_palette), %ebp  /* ebp = _colorconv_indexed_palette  */
   movl %ecx, %ebx

   /* 8 bit to 32 bit conversion:
    we have:
    eax = free
    ebx = SCREEN_W
    ecx = SCREEN_W
    edx = free
    esi = src_rect->data
    edi = dest_rect->data
    ebp = _colorconv_indexed_palette
    LOCAL1 = SCREEN_H
    LOCAL2 = offset from the end of a line to the beginning of the next
    LOCAL3 = same as LOCAL2, but for the dest bitmap
   */

   _align_
   next_line_8_to_32:
      shrl $2, %ecx             /* work with packs of 4 pixels */

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      jz do_one_pixel_8_to_32  /* less than 4 pixels? Can't work with the main loop */
#endif

      _align_
      next_block_8_to_32:
         movl (%esi), %edx          /* edx = [4][3][2][1] */ 
         movzbl %dl, %eax
         movd (%ebp,%eax,4), %mm0   /* mm0 = xxxxxxxxx [   1   ] */
         shrl $8, %edx
         movzbl %dl, %eax
         movd (%ebp,%eax,4), %mm1   /* mm1 = xxxxxxxxx [   2   ] */
         punpckldq %mm1, %mm0       /* mm0 = [   2   ] [   1   ] */
         addl $4, %esi
         movq %mm0, (%edi)
         shrl $8, %edx
         movzbl %dl, %eax
         movd (%ebp,%eax,4), %mm0   /* mm0 = xxxxxxxxx [   3   ] */
         shrl $8, %edx
         movl %edx, %eax
         movd (%ebp,%eax,4), %mm1   /* mm1 = xxxxxxxxx [   4   ] */
         punpckldq %mm1, %mm0       /* mm0 = [   4   ] [   3   ] */
         movq %mm0, 8(%edi)
         addl $16, %edi

         decl %ecx
         jnz next_block_8_to_32

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_8_to_32:
         movl %ebx, %ecx           /* restore width */
         andl $3, %ecx
         jz end_of_line_8_to_32    /* nothing to do? */

         shrl $1, %ecx
         jnc do_two_pixels_8_to_32

         movzbl (%esi), %edx       /* convert 1 pixel */
         movl (%ebp,%edx,4), %edx
         incl %esi
         addl $4, %edi
         movl %edx, -4(%edi)

      do_two_pixels_8_to_32:
         shrl $1, %ecx
         jnc end_of_line_8_to_32

         movzbl (%esi), %edx       /* convert 2 pixels */
         movzbl 1(%esi), %eax
         movl (%ebp,%edx,4), %edx
         movl (%ebp,%eax,4), %eax
         addl $2, %esi
         movl %edx, (%edi)
         movl %eax, 4(%edi)
         addl $8, %edi

   _align_
   end_of_line_8_to_32:
#endif

      movl LOCAL2, %edx
      addl %edx, %esi
      movl LOCAL3, %eax
      addl %eax, %edi
      movl LOCAL1, %edx
      movl %ebx, %ecx          /* restore width */
      decl %edx
      movl %edx, LOCAL1
      jnz next_line_8_to_32

   CLEANUP_LOCALS(4)

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret

#endif  /* ALLEGRO_COLOR8 */



#ifdef ALLEGRO_COLOR16

/* void _colorconv_blit_15_to_16 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_15_to_16)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_15_to_16_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   shll $1, %ecx
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ecx, %eax

   movl ARG2, %ebx                    /* ebx = dest_rect        */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ecx, %ebx
   shrl $1, %ecx

   /* 15 bit to 16 bit conversion:
    we have:
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
    ecx = SCREEN_W
    edx = SCREEN_H
    esi = src_rect->data
    edi = dest_rect->data
   */

   movd %ecx, %mm7              /* save width for later */
   
   movl $0x7FE07FE0, %ecx
   movd %ecx, %mm6
   movl $0x00200020, %ecx       /* addition to green component */
   punpckldq %mm6, %mm6         /* mm6 = reg-green mask */
   movd %ecx, %mm4
   movl $0x001F001F, %ecx
   punpckldq %mm4, %mm4         /* mm4 = green add mask */
   movd %ecx, %mm5
   punpckldq %mm5, %mm5         /* mm5 = blue mask */

   movd %mm7, %ecx

   _align_
   next_line_15_to_16:
      shrl $3, %ecx             /* work with packs of 8 pixels */
      orl %ecx, %ecx
      jz do_one_pixel_15_to_16  /* less than 8 pixels? Can't work with the main loop */

      _align_
      next_block_15_to_16:
         movq (%esi), %mm0         /* read 8 pixels */
         movq 8(%esi), %mm1        /* mm1 = [rgb7][rgb6][rgb5][rgb4] */
         movq %mm0, %mm2           /* mm0 = [rgb3][rgb2][rgb1][rgb0] */
         movq %mm1, %mm3
         pand %mm6, %mm0           /* isolate red-green */
         pand %mm6, %mm1
         pand %mm5, %mm2           /* isolate blue */
         pand %mm5, %mm3
         psllq $1, %mm0            /* shift red-green by 1 bit to the left */
         addl $16, %esi
         psllq $1, %mm1
         addl $16, %edi
         por %mm4, %mm0            /* set missing bit to 1 */
         por %mm4, %mm1
         por %mm2, %mm0            /* recombine components */
         por %mm3, %mm1
         movq %mm0, -16(%edi)      /* write result */
         movq %mm1, -8(%edi)

         decl %ecx
         jnz next_block_15_to_16

      do_one_pixel_15_to_16:
         movd %mm7, %ecx          /* anything left to do? */
         andl $7, %ecx
         jz end_of_line_15_to_16

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
         shrl $1, %ecx            /* do one pixel */
         jnc do_two_pixels_15_to_16

         movzwl (%esi), %ecx      /* read one pixel */
         addl $2, %esi
         movd %ecx, %mm0
         movd %ecx, %mm2
         pand %mm6, %mm0
         pand %mm5, %mm2
         psllq $1, %mm0
         addl $2, %edi
         por %mm4, %mm0
         por %mm2, %mm0
         movd %mm0, %ecx
         movw %cx, -2(%edi)
         movd %mm7, %ecx
         shrl $1, %ecx

      do_two_pixels_15_to_16:
         shrl $1, %ecx
         jnc do_four_pixels_15_to_16

         movd (%esi), %mm0         /* read two pixels */
         addl $4, %esi
         movq %mm0, %mm2
         pand %mm6, %mm0
         pand %mm5, %mm2
         psllq $1, %mm0
         addl $4, %edi
         por %mm4, %mm0
         por %mm2, %mm0
         movd %mm0, -4(%edi)

      _align_
      do_four_pixels_15_to_16:
         shrl $1, %ecx
         jnc end_of_line_15_to_16
#endif

         movq (%esi), %mm0        /* read four pixels */
         addl $8, %esi
         movq %mm0, %mm2
         pand %mm6, %mm0
         pand %mm5, %mm2
         psllq $1, %mm0
         por %mm4, %mm0
         por %mm2, %mm0
         addl $8, %edi
         movq %mm0, -8(%edi)

   _align_
   end_of_line_15_to_16:
      addl %eax, %esi
      movd %mm7, %ecx           /* restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_15_to_16

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret



/* void _colorconv_blit_15_to_32 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_15_to_32)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_15_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_2 ($0x7c00, $0x03e0, $0x001f);

   /* 15 bit to 32 bit conversion:
    we have:    
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
    ecx = SCREEN_W
    edx = SCREEN_H
    esi = src_rect->data
    edi = dest_rect->data
    ebp = SCREEN_W
   */

   _align_
   next_line_15_to_32:
      shrl $1, %ecx             /* work with packs of 2 pixels */

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      jz do_one_pixel_15_to_32  /* 1 pixel? Can't use dual-pixel code */
#endif

      _align_
      next_block_15_to_32:
         movd (%esi), %mm0    /* mm0 = 0000 0000  [rgb1][rgb2] */
         punpcklwd %mm0, %mm0 /* mm0 = xxxx [rgb1] xxxx [rgb2]  (x don't matter) */
         movq %mm0, %mm1
         movq %mm0, %mm2
         PAND (5, 0)        /* pand %mm5, %mm0 */
         pslld $3, %mm0
         PAND (3, 1)        /* pand %mm3, %mm1 */
         pslld $6, %mm1
         por %mm1, %mm0
         addl $4, %esi
         PAND (4, 2)        /* pand %mm4, %mm2 */
         pslld $9, %mm2
         por %mm2, %mm0
         movq %mm0, (%edi)
         addl $8, %edi

         decl %ecx
         jnz next_block_15_to_32

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_15_to_32:
         movl %ebp, %ecx    /* restore width */
         shrl $1, %ecx
         jnc end_of_line_15_to_32

         movd (%esi), %mm0
         punpcklwd %mm0, %mm0
         movq %mm0, %mm1
         movq %mm0, %mm2
         PAND (5, 0)        /* pand %mm5, %mm0 */
         pslld $3, %mm0
         PAND (3, 1)        /* pand %mm3, %mm1 */
         pslld $6, %mm1
         por %mm1, %mm0
         addl $2, %esi
         PAND (4, 2)        /* pand %mm4, %mm2 */
         pslld $9, %mm2
         por %mm2, %mm0
         movd %mm0, (%edi)
         addl $4, %edi

   _align_
   end_of_line_15_to_32:
#endif

      addl %eax, %esi
      movl %ebp, %ecx         /* restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_15_to_32

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret



/* void _colorconv_blit_16_to_15 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_16_to_15)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_16_to_15_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   shll $1, %ecx
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ecx, %eax

   movl ARG2, %ebx                    /* ebx = dest_rect        */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ecx, %ebx
   shrl $1, %ecx

   /* 16 bit to 15 bit conversion:
    we have:
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
    ecx = SCREEN_W
    edx = SCREEN_H
    esi = src_rect->data
    edi = dest_rect->data
   */

   movd %ecx, %mm7              /* save width for later */

   movl $0xFFC0FFC0, %ecx
   movd %ecx, %mm6
   punpckldq %mm6, %mm6         /* mm6 = reg-green mask */
   movl $0x001F001F, %ecx
   movd %ecx, %mm5
   punpckldq %mm5, %mm5         /* mm4 = blue mask */
   
   movd %mm7, %ecx

   _align_
   next_line_16_to_15:
      shrl $3, %ecx             /* work with packs of 8 pixels */
      orl %ecx, %ecx
      jz do_one_pixel_16_to_15  /* less than 8 pixels? Can't work with the main loop */

      _align_
      next_block_16_to_15:
         movq (%esi), %mm0         /* read 8 pixels */
         movq 8(%esi), %mm1        /* mm1 = [rgb7][rgb6][rgb5][rgb4] */
         addl $16, %esi
         addl $16, %edi
         movq %mm0, %mm2           /* mm0 = [rgb3][rgb2][rgb1][rgb0] */
         movq %mm1, %mm3
         pand %mm6, %mm0           /* isolate red-green */
         pand %mm6, %mm1
         pand %mm5, %mm2           /* isolate blue */
         psrlq $1, %mm0            /* shift red-green by 1 bit to the right */
         pand %mm5, %mm3
         psrlq $1, %mm1
         por %mm2, %mm0            /* recombine components */
         por %mm3, %mm1
         movq %mm0, -16(%edi)      /* write result */
         movq %mm1, -8(%edi)

         decl %ecx
         jnz next_block_16_to_15

      do_one_pixel_16_to_15:
         movd %mm7, %ecx          /* anything left to do? */
         andl $7, %ecx
         jz end_of_line_16_to_15

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
         shrl $1, %ecx            /* do one pixel */
         jnc do_two_pixels_16_to_15

         movzwl (%esi), %ecx      /* read one pixel */
         addl $2, %esi
         movd %ecx, %mm0
         movd %ecx, %mm2
         pand %mm6, %mm0
         pand %mm5, %mm2
         psrlq $1, %mm0
         por %mm2, %mm0
         movd %mm0, %ecx
         addl $2, %edi
         movw %cx, -2(%edi)
         movd %mm7, %ecx
         shrl $1, %ecx

      do_two_pixels_16_to_15:
         shrl $1, %ecx
         jnc do_four_pixels_16_to_15

         movd (%esi), %mm0      /* read two pixels */
         addl $4, %esi
         movq %mm0, %mm2
         pand %mm6, %mm0
         pand %mm5, %mm2
         psrlq $1, %mm0
         por %mm2, %mm0
         addl $4, %edi
         movd %mm0, -4(%edi)

      _align_
      do_four_pixels_16_to_15:
         shrl $1, %ecx
         jnc end_of_line_16_to_15
#endif

         movq (%esi), %mm0      /* read four pixels */
         addl $8, %esi
         movq %mm0, %mm2
         pand %mm6, %mm0
         pand %mm5, %mm2
         psrlq $1, %mm0
         por %mm2, %mm0
         addl $8, %edi
         movd %mm0, -8(%edi)

   _align_
   end_of_line_16_to_15:
      addl %eax, %esi
      movd %mm7, %ecx           /* restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_16_to_15

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret



/* void _colorconv_blit_16_to_32 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_16_to_32)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_16_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_2 ($0xf800, $0x07e0, $0x001f);

   /* 16 bit to 32 bit conversion:
    we have:
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
    ecx = SCREEN_W
    edx = SCREEN_H
    esi = src_rect->data
    edi = dest_rect->data
    ebp = SCREEN_W
   */

   _align_
   next_line_16_to_32:
      shrl $1, %ecx             /* work with packs of 2 pixels */

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      jz do_one_pixel_16_to_32  /* 1 pixel? Can't use dual-pixel code */
#endif

      _align_
      next_block_16_to_32:
         movd (%esi), %mm0    /* mm0 = 0000 0000  [rgb1][rgb2] */
         punpcklwd %mm0, %mm0 /* mm0 = xxxx [rgb1] xxxx [rgb2]  (x don't matter) */
         movq %mm0, %mm1
         movq %mm0, %mm2
         PAND (5, 0)        /* pand %mm5, %mm0 */
         pslld $3, %mm0
         PAND (3, 1)        /* pand %mm3, %mm1 */
         pslld $5, %mm1
         por %mm1, %mm0
         addl $4, %esi
         PAND (4, 2)        /* pand %mm4, %mm2 */
         pslld $8, %mm2
         por %mm2, %mm0
         movq %mm0, (%edi)
         addl $8, %edi

         decl %ecx
         jnz next_block_16_to_32

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_16_to_32:
         movl %ebp, %ecx    /* restore width */
         shrl $1, %ecx
         jnc end_of_line_16_to_32

         movd (%esi), %mm0
         punpcklwd %mm0, %mm0
         movq %mm0, %mm1
         movq %mm0, %mm2
         PAND (5, 0)        /* pand %mm5, %mm0 */
         pslld $3, %mm0
         PAND (3, 1)        /* pand %mm3, %mm1 */
         pslld $5, %mm1
         por %mm1, %mm0
         addl $2, %esi
         PAND (4, 2)        /* pand %mm4, %mm2 */
         pslld $8, %mm2
         por %mm2, %mm0
         movd %mm0, (%edi)
         addl $4, %edi

   _align_
   end_of_line_16_to_32:
#endif

      addl %eax, %esi
      movl %ebp, %ecx         /* restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_16_to_32

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret

#endif  /* ALLEGRO_COLOR16 */



#ifdef ALLEGRO_COLOR24

/* void _colorconv_blit_24_to_32 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_24_to_32)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_24_to_32_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   leal (%ecx, %ecx, 2), %ebx         /* ebx = SCREEN_W * 3     */
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ebx, %eax

   movl ARG2, %ebx                    /* ebx = dest_rect        */
   shll $2, %ecx                      /* ecx = SCREEN_W * 4     */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ecx, %ebx
   shrl $2, %ecx                      /* ecx = SCREEN_W         */
   movd %ecx, %mm7

   /* 24 bit to 32 bit conversion:
    we have:
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
    ecx = SCREEN_W
    edx = SCREEN_H
    esi = src_rect->data
    edi = dest_rect->data
   */

   _align_
   next_line_24_to_32:
      shrl $2, %ecx             /* work with packs of 4 pixels */

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      jz do_one_pixel_24_to_32  /* less than 4 pixels? Can't work with the main loop */
#endif

      _align_
      next_block_24_to_32:
         movq (%esi), %mm0         /* mm0 = [GB2][RGB1][RGB0] */
         movd 8(%esi), %mm1        /* mm1 = [..0..][RGB3][R2] */
         movq %mm0, %mm2
         movq %mm0, %mm3
         movq %mm1, %mm4
         psllq $16, %mm2
         psllq $40, %mm0
         psrlq $40, %mm2
         psrlq $40, %mm0           /* mm0 = [....0....][RGB0] */
         psllq $32, %mm2           /* mm2 = [..][RGB1][..0..] */
         psrlq $8, %mm1
         psrlq $48, %mm3           /* mm3 = [.....0....][GB2] */
         psllq $56, %mm4
         psllq $32, %mm1           /* mm1 = [.RGB3][....0...] */
         psrlq $40, %mm4           /* mm4 = [....0...][R2][0] */
         por %mm3, %mm1
         por %mm2, %mm0            /* mm0 = [.RGB1][.RGB0]    */
         por %mm4, %mm1            /* mm1 = [.RGB3][.RGB2]    */
         movq %mm0, (%edi)
         movq %mm1, 8(%edi)
         addl $12, %esi
         addl $16, %edi

         decl %ecx
         jnz next_block_24_to_32

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   do_one_pixel_24_to_32:
      movd %mm7, %ecx           /* restore width */
      andl $3, %ecx
      jz end_of_line_24_to_32   /* nothing to do? */

      shrl $1, %ecx
      jnc do_two_pixels_24_to_32

      xorl %ecx, %ecx           /* partial registar stalls ahead, 6 cycles penalty on the 686 */
      movzwl (%esi), %ebp
      movb  2(%esi), %cl
      movw  %bp, (%edi)
      movw  %cx, 2(%edi)
      addl $3, %esi
      addl $4, %edi
      movd %mm7, %ecx           /* restore width */
      shrl $1, %ecx

   do_two_pixels_24_to_32:
      shrl $1, %ecx
      jnc end_of_line_24_to_32

      movd (%esi), %mm0         /* read 2 pixels */
      movzwl 4(%esi), %ecx
      movd %ecx, %mm1
      movq %mm0, %mm2
      pslld $8, %mm1
      addl $6, %esi
      pslld $8, %mm0
      addl $8, %edi
      psrld $24, %mm2
      psrld $8, %mm0
      por %mm2, %mm1
      psllq $32, %mm1
      por %mm1, %mm0
      movq %mm0, -8(%edi)

   _align_
   end_of_line_24_to_32:
#endif

      addl %eax, %esi
      movd %mm7, %ecx           /* restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_24_to_32

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret

#endif  /* ALLEGRO_COLOR24 */



#ifdef ALLEGRO_COLOR32

/* void _colorconv_blit_32_to_15 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_32_to_15)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_32_to_15_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_1 ($0xf80000, $0x00f800, $0x0000f8);

   /* 32 bit to 15 bit conversion:
    we have:
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
    ecx = SCREEN_W
    edx = SCREEN_H
    esi = src_rect->data
    edi = dest_rect->data
    ebp = SCREEN_W
   */

   _align_
   next_line_32_to_15:
      shrl $1, %ecx             /* work with packs of 2 pixels */

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      jz do_one_pixel_32_to_15  /* 1 pixel? Can't use dual-pixel code */
#endif

      _align_
      next_block_32_to_15:
         movq (%esi), %mm0
         movq %mm0, %mm1
         movq %mm0, %mm2
         PAND (5, 0)        /* pand %mm5, %mm0 */
         psrld $3, %mm0
         PAND (3, 1)        /* pand %mm3, %mm1 */
         psrld $6, %mm1
         por %mm1, %mm0
         addl $8, %esi
         PAND (4, 2)        /* pand %mm4, %mm2 */
         psrld $9, %mm2
         por %mm2, %mm0
         movq %mm0, %mm6
         psrlq $16, %mm0
         por %mm0, %mm6
         movd %mm6, (%edi)
         addl $4, %edi

         decl %ecx
         jnz next_block_32_to_15

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_32_to_15:
         movl %ebp, %ecx    /* restore width */
         shrl $1, %ecx
         jnc end_of_line_32_to_15

         movd (%esi), %mm0
         movq %mm0, %mm1
         movq %mm0, %mm2
         PAND (5, 0)        /* pand %mm5, %mm0 */
         psrld $3, %mm0
         PAND (3, 1)        /* pand %mm3, %mm1 */
         psrld $6, %mm1
         por %mm1, %mm0
         addl $4, %esi
         PAND (4, 2)        /* pand %mm4, %mm2 */
         psrld $9, %mm2
         por %mm2, %mm0
         movd %mm0, %ecx
         movw %cx, (%edi)
         addl $2, %edi

   _align_
   end_of_line_32_to_15:
#endif

      addl %eax, %esi
      movl %ebp, %ecx         /* restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_32_to_15

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret



/* void _colorconv_blit_32_to_16 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_32_to_16)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_32_to_16_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   INIT_CONVERSION_1 ($0xf80000, $0x00fc00, $0x0000f8);

   /* 32 bit to 16 bit conversion:
    we have:
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
    ecx = SCREEN_W
    edx = SCREEN_H
    esi = src_rect->data
    edi = dest_rect->data
    ebp = SCREEN_W
   */

   _align_
   next_line_32_to_16:
      shrl $1, %ecx             /* work with packs of 2 pixels */

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      jz do_one_pixel_32_to_16  /* 1 pixel? Can't use dual-pixel code */
#endif

      _align_
      next_block_32_to_16:
         movq (%esi), %mm0
         movq %mm0, %mm1
         nop
         movq %mm0, %mm2
         PAND (5, 0)        /* pand %mm5, %mm0 */
         psrld $3, %mm0
         PAND (3, 1)        /* pand %mm3, %mm1 */
         psrld $5, %mm1
         por %mm1, %mm0
         addl $8, %esi
         PAND (4, 2)        /* pand %mm4, %mm2 */
         psrld $8, %mm2
         nop
         nop
         por %mm2, %mm0
         movq %mm0, %mm6
         psrlq $16, %mm0
         por %mm0, %mm6
         movd %mm6, (%edi)
         addl $4, %edi

         decl %ecx
         jnz next_block_32_to_16

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_32_to_16:
         movl %ebp, %ecx      /* restore width */
         shrl $1, %ecx
         jnc end_of_line_32_to_16

         movd (%esi), %mm0
         movq %mm0, %mm1
         movq %mm0, %mm2
         PAND (5, 0)          /* pand %mm5, %mm0 - get Blue component */
         PAND (3, 1)          /* pand %mm3, %mm1 - get Red component */
         psrld $3, %mm0       /* adjust Red, Green and Blue to correct positions */
         PAND (4, 2)          /* pand %mm4, %mm2 - get Green component */
         psrld $5, %mm1
         psrld $8, %mm2
         por %mm1, %mm0       /* combine Red and Blue */
         addl $4, %esi
         por %mm2, %mm0       /* and green */
         movq %mm0, %mm6      /* make the pixels fit in the first 32 bits */
         psrlq $16, %mm0
         por %mm0, %mm6
         movd %mm6, %ecx
         movw %cx, (%edi)
         addl $2, %edi

   _align_
   end_of_line_32_to_16:
#endif

      addl %eax, %esi
      movl %ebp, %ecx         /* restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_32_to_16

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret



/* void _colorconv_blit_32_to_24 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_32_to_24)
   movl GLOBL(cpu_capabilities), %eax     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %eax
   jz _colorconv_blit_32_to_24_no_mmx

   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */
   movl $0xFFFFFF, %eax               /* get RGB mask           */
   movd %eax, %mm5                    /* low RGB mask in mm5    */
   movd %eax, %mm6 
   psllq $32, %mm6                    /* high RGB mask in mm6   */

   movl ARG1, %eax                    /* eax = src_rect         */
   movl GFXRECT_WIDTH(%eax), %ecx     /* ecx = src_rect->width  */
   movl GFXRECT_HEIGHT(%eax), %edx    /* edx = src_rect->height */
   shll $2, %ecx                      /* ecx = SCREEN_W * 4     */
   movl GFXRECT_DATA(%eax), %esi      /* esi = src_rect->data   */
   movl GFXRECT_PITCH(%eax), %eax     /* eax = src_rect->pitch  */
   subl %ecx, %eax

   movl ARG2, %ebx                    /* ebx = dest_rect        */
   shrl $2, %ecx                      /* ecx = SCREEN_W         */
   leal (%ecx, %ecx, 2), %ebp         /* ebp = SCREEN_W * 3     */
   movl GFXRECT_DATA(%ebx), %edi      /* edi = dest_rect->data  */
   movl GFXRECT_PITCH(%ebx), %ebx     /* ebx = dest_rect->pitch */
   subl %ebp, %ebx
   movd %ecx, %mm7

   /* 32 bit to 24 bit conversion:
    we have:
    eax = offset from the end of a line to the beginning of the next
    ebx = same as eax, but for the dest bitmap
    ecx = SCREEN_W
    edx = SCREEN_H
    esi = src_rect->data
    edi = dest_rect->data
   */

   _align_
   next_line_32_to_24:
      shrl $2, %ecx             /* work with packs of 4 pixels */

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      jz do_one_pixel_32_to_24  /* less than 4 pixels? Can't work with the main loop */
#endif

      _align_
      next_block_32_to_24:
         /* i686: 14 cycles/4 pixels, i586: 11 cycles/4 pixels  */
         movq (%esi), %mm0         /* mm0 = [ARGBARGB](1)(0)    */
         addl $12, %edi
         movq 8(%esi), %mm2        /* mm2 = [ARGBARGB](3)(2)    */
         addl $16, %esi

         movq %mm0, %mm1           /* mm1 = [ARGBARGB](1)(0)    */
         movq %mm2, %mm3           /* mm3 = [ARGBARGB](3)(2)    */
         
         pand %mm6, %mm1           /* mm1 = [.RGB....](1)       */
         pand %mm5, %mm2           /* mm2 = [.....RGB](2)       */

         psrlq $8, %mm1            /* mm1 = [..RGB...](1)       */
         pand %mm6, %mm3           /* mm3 = [.RGB....](3)       */
         
         movq %mm2, %mm4           /* mm4 = [.....RGB](2)       */
         pand %mm5, %mm0           /* mm0 = [.....RGB](0)       */
         
         psrlq $16, %mm2           /* mm2 = [.......R](2)       */
         por %mm1, %mm0            /* mm0 = [..RGBRGB](1)(0)    */

         psrlq $24, %mm3           /* mm3 = [....RGB.](3)       */
         decl %ecx

         psllq $48, %mm4           /* mm4 = [GB......](2)       */
         por %mm3, %mm2            /* mm2 = [....RGBR](3)(2)    */

         por %mm4, %mm0            /* mm0 = [GBRGBRGB](2)(1)(0) */
         movq %mm0, -12(%edi)
         movd %mm2, -4(%edi)

         jnz next_block_32_to_24

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_32_to_24:
         movd %mm7, %ecx           /* restore width */
         andl $3, %ecx
         jz end_of_line_32_to_24   /* nothing to do? */

         shrl $1, %ecx
         jnc do_two_pixels_32_to_24

         movl (%esi), %ecx
         addl $4, %esi
         movw %cx, (%edi)
         shrl $16, %ecx
         addl $3, %edi
         movb %cl, -1(%edi)

         movd %mm7, %ecx
         shrl $1, %ecx             /* restore width */

      do_two_pixels_32_to_24:
         shrl $1, %ecx
         jnc end_of_line_32_to_24

         /* 4 cycles/2 pixels */
         movq (%esi), %mm0         /* mm0 = [.RGB.RGB](1)(0) */

         movq %mm0, %mm1           /* mm1 = [.RGB.RGB](1)(0) */

         pand %mm5, %mm0           /* mm0 = [.....RGB](0)    */
         pand %mm6, %mm1           /* mm1 = [.RGB....](1)    */

         psrlq $8, %mm1            /* mm1 = [..RGB...](1)    */

         por %mm1, %mm0            /* mm0 = [..RGBRGB](1)(0) */

         movd %mm0, (%edi)
         psrlq $32, %mm0
         movd %mm0, %ecx
         movw %cx, 2(%edi)

         addl $8, %esi
         addl $6, %edi

   _align_
   end_of_line_32_to_24:
#endif

      addl %eax, %esi
      movd %mm7, %ecx           /* restore width */
      addl %ebx, %edi
      decl %edx
      jnz next_line_32_to_24

   emms
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret

#endif  /* ALLEGRO_COLOR32 */

#endif  /* ALLEGRO_MMX */



/********************************************************************************************/
/* pure 386 asm routines                                                                    */
/*  optimized for Intel Pentium                                                             */
/********************************************************************************************/

#define RESERVE_MYLOCALS(n) subl $SLOTS_TO_BYTES(n), %esp
#define CLEANUP_MYLOCALS(n) addl $SLOTS_TO_BYTES(n), %esp

/* create the default stack frame (4 slots) */
#define CREATE_STACK_FRAME  \
   pushl %ebp             ; \
   movl %esp, %ebp        ; \
   pushl %ebx             ; \
   pushl %esi             ; \
   pushl %edi             ; \
   RESERVE_MYLOCALS(4)

#define DESTROY_STACK_FRAME \
   CLEANUP_MYLOCALS(4)    ; \
   popl %edi              ; \
   popl %esi              ; \
   popl %ebx              ; \
   popl %ebp

#define MYLOCAL1    0(%esp)
#define MYLOCAL2    4(%esp)
#define MYLOCAL3    8(%esp)
#define MYLOCAL4   12(%esp)

/* create the big stack frame (5 slots) */
#define CREATE_BIG_STACK_FRAME  \
   pushl %ebp             ; \
   movl %esp, %ebp        ; \
   pushl %ebx             ; \
   pushl %esi             ; \
   pushl %edi             ; \
   RESERVE_MYLOCALS(5)

#define DESTROY_BIG_STACK_FRAME \
   CLEANUP_MYLOCALS(5)    ; \
   popl %edi              ; \
   popl %esi              ; \
   popl %ebx              ; \
   popl %ebp

#define SPILL_SLOT   16(%esp)

/* initialize the registers */
#define SIZE_1
#define SIZE_2 addl %ebx, %ebx
#define SIZE_3 leal (%ebx,%ebx,2), %ebx
#define SIZE_4 shll $2, %ebx
#define LOOP_RATIO_1
#define LOOP_RATIO_2 shrl $1, %edi
#define LOOP_RATIO_4 shrl $2, %edi

#define INIT_REGISTERS_NO_MMX(src_mul_code, dest_mul_code, width_ratio_code)        \
   movl ARG1, %eax                  /* eax      = src_rect                    */  ; \
   movl GFXRECT_WIDTH(%eax), %ebx   /* ebx      = src_rect->width             */  ; \
   movl GFXRECT_HEIGHT(%eax), %edx  /* edx      = src_rect->height            */  ; \
   movl GFXRECT_PITCH(%eax), %ecx   /* ecx      = src_rect->pitch             */  ; \
   movl %ebx, %edi                  /* edi      = width                       */  ; \
   src_mul_code                     /* ebx      = width*x                     */  ; \
   movl GFXRECT_DATA(%eax), %esi    /* esi      = src_rect->data              */  ; \
   subl %ebx, %ecx                                                                ; \
   movl %edi, %ebx                                                                ; \
   width_ratio_code                                                               ; \
   movl ARG2, %eax                  /* eax      = dest_rect                   */  ; \
   movl %edi, MYLOCAL1              /* MYLOCAL1 = width/y                     */  ; \
   movl %ecx, MYLOCAL2              /* MYLOCAL2 = src_rect->pitch - width*x   */  ; \
   dest_mul_code                    /* ebx      = width*y                     */  ; \
   movl GFXRECT_PITCH(%eax), %ecx   /* ecx      = dest_rect->pitch            */  ; \
   subl %ebx, %ecx                                                                ; \
   movl GFXRECT_DATA(%eax), %edi    /* edi      = dest_rect->data             */  ; \
   movl %ecx, MYLOCAL3              /* MYLOCAL3 = dest_rect->pitch - width*y  */  ; \
   movl %edx, MYLOCAL4              /* MYLOCAL4 = src_rect->height            */

  /* registers state after initialization:
    eax: free 
    ebx: free
    ecx: free (for the inner loop counter)
    edx: free
    esi: (char *) source surface pointer
    edi: (char *) destination surface pointer
    ebp: free (for the lookup table base pointer)
    MYLOCAL1: (const int) width/ratio
    MYLOCAL2: (const int) offset from the end of a line to the beginning of next
    MYLOCAL3: (const int) same as MYLOCAL2, but for the dest bitmap
    MYLOCAL4: (int) height
   */


#define CONV_TRUE_TO_8_NO_MMX(name, bytes_ppixel)                                 \
   _align_                                                                      ; \
   next_line_##name:                                                            ; \
      movl MYLOCAL1, %ecx                                                       ; \
                                                                                ; \
      _align_                                                                   ; \
      next_block_##name:                                                        ; \
         xorl %edx, %edx                                                        ; \
         movb (%esi), %al          /* read 1 pixel */                           ; \
         movb 1(%esi), %bl                                                      ; \
         movb 2(%esi), %dl                                                      ; \
         shrb $4, %al                                                           ; \
         addl $bytes_ppixel, %esi                                               ; \
         shll $4, %edx                                                          ; \
         andb $0xf0, %bl                                                        ; \
         orb  %bl, %al             /* combine to get 4.4.4 */                   ; \
         incl %edi                                                              ; \
         movb %al, %dl                                                          ; \
         movb (%ebp, %edx), %dl    /* look it up */                             ; \
         movb %dl, -1(%edi)        /* write 1 pixel */                          ; \
         decl %ecx                                                              ; \
         jnz next_block_##name                                                  ; \
                                                                                ; \
      addl MYLOCAL2, %esi                                                       ; \
      addl MYLOCAL3, %edi                                                       ; \
      decl MYLOCAL4                                                             ; \
      jnz next_line_##name


#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH

#define CONV_TRUE_TO_15_NO_MMX(name, bytes_ppixel)                                 \
   _align_                                                                       ; \
   next_line_##name:                                                             ; \
      movl MYLOCAL1, %ecx                                                        ; \
                                                                                 ; \
      _align_                                                                    ; \
      /* 100% Pentium pairable loop */                                           ; \
      /* 11 cycles = 10 cycles/2 pixels + 1 cycle loop */                        ; \
      next_block_##name:                                                         ; \
         movb bytes_ppixel(%esi), %al     /* al = b8 pixel2                  */  ; \
         addl $4, %edi                    /* 2 pixels written                */  ; \
         shrb $3, %al                     /* al = b5 pixel2                  */  ; \
         movb bytes_ppixel+1(%esi), %bh   /* ebx = g8 pixel2 << 8            */  ; \
         shll $16, %ebx                   /* ebx = g8 pixel2 << 24           */  ; \
         movb bytes_ppixel+2(%esi), %ah   /* eax = r8b5 pixel2               */  ; \
         shrb $1, %ah                     /* eax = r7b5 pixel2               */  ; \
         movb (%esi), %dl                 /* dl = b8 pixel1                  */  ; \
         shrb $3, %dl                     /* dl = b5 pixel1                  */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel2 | g8 pixel1     */  ; \
         shll $16, %eax                   /* eax = r7b5 pixel2 << 16         */  ; \
         movb 2(%esi), %dh                /* edx = r8b5 pixel1               */  ; \
         shrb $1, %dh                     /* edx = r7b5 pixel1               */  ; \
         addl $bytes_ppixel*2, %esi       /* 2 pixels read                   */  ; \
         shrl $6, %ebx                    /* ebx = g5 pixel2 | g5 pixel1     */  ; \
         orl  %edx, %eax                  /* eax = r7b5 pixel2 | r7b5 pixel1 */  ; \
         andl $0x7c1f7c1f, %eax           /* eax = r5b5 pixel2 | r5b5 pixel1 */  ; \
         andl $0x03e003e0, %ebx           /* clean g5 pixel2 | g5 pixel1     */  ; \
         orl  %ebx, %eax                  /* eax = pixel2 | pixel1           */  ; \
         decl %ecx                                                               ; \
         movl %eax, -4(%edi)              /* write pixel1..pixel2            */  ; \
         jnz next_block_##name                                                   ; \
                                                                                 ; \
      addl MYLOCAL2, %esi                                                        ; \
      addl MYLOCAL3, %edi                                                        ; \
      decl MYLOCAL4                                                              ; \
      jnz next_line_##name


#define CONV_TRUE_TO_16_NO_MMX(name, bytes_ppixel)                                 \
   _align_                                                                       ; \
   next_line_##name:                                                             ; \
      movl MYLOCAL1, %ecx                                                        ; \
                                                                                 ; \
      _align_                                                                    ; \
      /* 100% Pentium pairable loop */                                           ; \
      /* 10 cycles = 9 cycles/2 pixels + 1 cycle loop */                         ; \
      next_block_##name:                                                         ; \
         movb bytes_ppixel(%esi), %al     /* al = b8 pixel2                  */  ; \
         addl $4, %edi                    /* 2 pixels written                */  ; \
         shrb $3, %al                     /* al = b5 pixel2                  */  ; \
         movb bytes_ppixel+1(%esi), %bh   /* ebx = g8 pixel2 << 8            */  ; \
         shll $16, %ebx                   /* ebx = g8 pixel2 << 24           */  ; \
         movb (%esi), %dl                 /* dl = b8 pixel1                  */  ; \
         shrb $3, %dl                     /* dl = b5 pixel1                  */  ; \
         movb bytes_ppixel+2(%esi), %ah   /* eax = r8b5 pixel2               */  ; \
         shll $16, %eax                   /* eax = r8b5 pixel2 << 16         */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel2 | g8 pixel1     */  ; \
         shrl $5, %ebx                    /* ebx = g6 pixel2 | g6 pixel1     */  ; \
         movb 2(%esi), %dh                /* edx = r8b5 pixel1               */  ; \
         orl  %edx, %eax                  /* eax = r8b5 pixel2 | r8b5 pixel1 */  ; \
         addl $bytes_ppixel*2, %esi       /* 2 pixels read                   */  ; \
         andl $0xf81ff81f, %eax           /* eax = r5b5 pixel2 | r5b5 pixel1 */  ; \
         andl $0x07e007e0, %ebx           /* clean g6 pixel2 | g6 pixel1     */  ; \
         orl  %ebx, %eax                  /* eax = pixel2 | pixel1           */  ; \
         decl %ecx                                                               ; \
         movl %eax, -4(%edi)              /* write pixel1..pixel2            */  ; \
         jnz next_block_##name                                                   ; \
                                                                                 ; \
      addl MYLOCAL2, %esi                                                        ; \
      addl MYLOCAL3, %edi                                                        ; \
      decl MYLOCAL4                                                              ; \
      jnz next_line_##name

#else

#define CONV_TRUE_TO_15_NO_MMX(name, bytes_ppixel)                                 \
   _align_                                                                       ; \
   next_line_##name:                                                             ; \
      movl MYLOCAL1, %ecx                                                        ; \
                                                                                 ; \
      shrl $1, %ecx                                                              ; \
      jz do_one_pixel_##name                                                     ; \
                                                                                 ; \
      _align_                                                                    ; \
      /* 100% Pentium pairable loop */                                           ; \
      /* 11 cycles = 10 cycles/2 pixels + 1 cycle loop */                        ; \
      next_block_##name:                                                         ; \
         movb bytes_ppixel(%esi), %al     /* al = b8 pixel2                  */  ; \
         addl $4, %edi                    /* 2 pixels written                */  ; \
         shrb $3, %al                     /* al = b5 pixel2                  */  ; \
         movb bytes_ppixel+1(%esi), %bh   /* ebx = g8 pixel2 << 8            */  ; \
         shll $16, %ebx                   /* ebx = g8 pixel2 << 24           */  ; \
         movb bytes_ppixel+2(%esi), %ah   /* eax = r8b5 pixel2               */  ; \
         shrb $1, %ah                     /* eax = r7b5 pixel2               */  ; \
         movb (%esi), %dl                 /* dl = b8 pixel1                  */  ; \
         shrb $3, %dl                     /* dl = b5 pixel1                  */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel2 | g8 pixel1     */  ; \
         shll $16, %eax                   /* eax = r7b5 pixel2 << 16         */  ; \
         movb 2(%esi), %dh                /* edx = r8b5 pixel1               */  ; \
         shrb $1, %dh                     /* edx = r7b5 pixel1               */  ; \
         addl $bytes_ppixel*2, %esi       /* 2 pixels read                   */  ; \
         shrl $6, %ebx                    /* ebx = g5 pixel2 | g5 pixel1     */  ; \
         orl  %edx, %eax                  /* eax = r7b5 pixel2 | r7b5 pixel1 */  ; \
         andl $0x7c1f7c1f, %eax           /* eax = r5b5 pixel2 | r5b5 pixel1 */  ; \
         andl $0x03e003e0, %ebx           /* clean g5 pixel2 | g5 pixel1     */  ; \
         orl  %ebx, %eax                  /* eax = pixel2 | pixel1           */  ; \
         decl %ecx                                                               ; \
         movl %eax, -4(%edi)              /* write pixel1..pixel2            */  ; \
         jnz next_block_##name                                                   ; \
                                                                                 ; \
      do_one_pixel_##name:                                                       ; \
         movl MYLOCAL1, %ecx                                                     ; \
         shrl $1, %ecx                                                           ; \
         jnc end_of_line_##name                                                  ; \
                                                                                 ; \
         movb (%esi), %cl                 /* cl = b8 pixel1                  */  ; \
         addl $2, %edi                                                           ; \
         shrb $3, %cl                     /* cl = b5 pixel1                  */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel1                 */  ; \
         shrl $6, %ebx                    /* ebx = g5 pixel1                 */  ; \
         movb 2(%esi), %ch                /* ecx = r8b5 pixel1               */  ; \
         shrb $1, %ch                     /* ecx = r7b5 pixel1               */  ; \
         addl $bytes_ppixel, %esi         /* 1 pixel read                    */  ; \
         andl $0x7c1f, %ecx               /* ecx = r5b5 pixel1               */  ; \
         andl $0x03e0, %ebx               /* clean g5 pixel1                 */  ; \
         orl  %ebx, %ecx                  /* edx = pixel1                    */  ; \
         movw %cx, -2(%edi)               /* write pixel1                    */  ; \
                                                                                 ; \
   _align_                                                                       ; \
   end_of_line_##name:                                                           ; \
      addl MYLOCAL2, %esi                                                        ; \
      addl MYLOCAL3, %edi                                                        ; \
      decl MYLOCAL4                                                              ; \
      jnz next_line_##name


#define CONV_TRUE_TO_16_NO_MMX(name, bytes_ppixel)                                 \
   _align_                                                                       ; \
   next_line_##name:                                                             ; \
      movl MYLOCAL1, %ecx                                                        ; \
                                                                                 ; \
      shrl $1, %ecx                                                              ; \
      jz do_one_pixel_##name                                                     ; \
                                                                                 ; \
      _align_                                                                    ; \
      /* 100% Pentium pairable loop */                                           ; \
      /* 10 cycles = 9 cycles/2 pixels + 1 cycle loop */                         ; \
      next_block_##name:                                                         ; \
         movb bytes_ppixel(%esi), %al     /* al = b8 pixel2                  */  ; \
         addl $4, %edi                    /* 2 pixels written                */  ; \
         shrb $3, %al                     /* al = b5 pixel2                  */  ; \
         movb bytes_ppixel+1(%esi), %bh   /* ebx = g8 pixel2 << 8            */  ; \
         shll $16, %ebx                   /* ebx = g8 pixel2 << 24           */  ; \
         movb (%esi), %dl                 /* dl = b8 pixel1                  */  ; \
         shrb $3, %dl                     /* dl = b5 pixel1                  */  ; \
         movb bytes_ppixel+2(%esi), %ah   /* eax = r8b5 pixel2               */  ; \
         shll $16, %eax                   /* eax = r8b5 pixel2 << 16         */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel2 | g8 pixel1     */  ; \
         shrl $5, %ebx                    /* ebx = g6 pixel2 | g6 pixel1     */  ; \
         movb 2(%esi), %dh                /* edx = r8b5 pixel1               */  ; \
         orl  %edx, %eax                  /* eax = r8b5 pixel2 | r8b5 pixel1 */  ; \
         addl $bytes_ppixel*2, %esi       /* 2 pixels read                   */  ; \
         andl $0xf81ff81f, %eax           /* eax = r5b5 pixel2 | r5b5 pixel1 */  ; \
         andl $0x07e007e0, %ebx           /* clean g6 pixel2 | g6 pixel1     */  ; \
         orl  %ebx, %eax                  /* eax = pixel2 | pixel1           */  ; \
         decl %ecx                                                               ; \
         movl %eax, -4(%edi)              /* write pixel1..pixel2            */  ; \
         jnz next_block_##name                                                   ; \
                                                                                 ; \
      do_one_pixel_##name:                                                       ; \
         movl MYLOCAL1, %ecx                                                     ; \
         shrl $1, %ecx                                                           ; \
         jnc end_of_line_##name                                                  ; \
                                                                                 ; \
         movb (%esi), %cl                 /* cl = b8 pixel1                  */  ; \
         addl $2, %edi                                                           ; \
         shrb $3, %cl                     /* cl = b5 pixel1                  */  ; \
         movb 1(%esi), %bh                /* ebx = g8 pixel1                 */  ; \
         shrl $5, %ebx                    /* ebx = g6 pixel1                 */  ; \
         movb 2(%esi), %ch                /* ecx = r8b5 pixel1               */  ; \
         addl $bytes_ppixel, %esi         /* 1 pixel read                    */  ; \
         andl $0xf81f, %ecx               /* ecx = r5b5 pixel1               */  ; \
         andl $0x07e0, %ebx               /* clean g6 pixel1                 */  ; \
         orl  %ebx, %ecx                  /* edx = pixel1                    */  ; \
         movw %cx, -2(%edi)               /* write pixel1                    */  ; \
                                                                                 ; \
   _align_                                                                       ; \
   end_of_line_##name:                                                           ; \
      addl MYLOCAL2, %esi                                                        ; \
      addl MYLOCAL3, %edi                                                        ; \
      decl MYLOCAL4                                                              ; \
      jnz next_line_##name

#endif  /* ALLEGRO_COLORCONV_ALIGNED_WIDTH */



#ifdef ALLEGRO_COLOR8

/* void _colorconv_blit_8_to_8 (struct GRAPHICS_RECT *src_rect,
 *                              struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_8_to_8)
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_1, LOOP_RATIO_4)
#else
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_1, LOOP_RATIO_1)
#endif

   movl GLOBL(_colorconv_rgb_map), %ebp

   _align_
   next_line_8_to_8_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $2, %ecx                /* work in packs of 4 pixels */
      jz do_one_pixel_8_to_8_no_mmx
#endif

      _align_
      next_block_8_to_8_no_mmx:
         movl (%esi), %eax         /* read 4 pixels */
         xorl %ebx, %ebx
         xorl %edx, %edx
         addl $4, %esi
         addl $4, %edi
         movb %al, %bl             /* pick out 2x bottom 8 bits */
         movb %ah, %dl
         shrl $16, %eax
         movb (%ebp, %ebx), %bl    /* lookup the new palette entries */
         movb (%ebp, %edx), %bh
         xorl %edx, %edx
         movb %ah, %dl             /* repeat for the top 16 bits */
         andl $0xff, %eax
         movb (%ebp, %eax), %al
         movb (%ebp, %edx), %ah
         shll $16, %eax
         orl %ebx, %eax            /* put everything together */
         movl %eax, -4(%edi)       /* write 4 pixels */
         decl %ecx
         jnz next_block_8_to_8_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_8_to_8_no_mmx:
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_8_to_8_no_mmx

         shrl $1, %ecx
         jnc do_two_pixels_8_to_8_no_mmx

         xorl %eax, %eax
         movb (%esi), %al          /* read 1 pixel */
         incl %edi
         incl %esi
         movb (%ebp, %eax), %al    /* lookup the new palette entry */
         movb %al, -1(%edi)        /* write 1 pixel */

      do_two_pixels_8_to_8_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_8_to_8_no_mmx

         xorl %eax, %eax
         xorl %ebx, %ebx
         movb (%esi), %al          /* read 2 pixels */
         movb 1(%esi), %bl
         addl $2, %edi
         addl $2, %esi
         movb (%ebp, %eax), %al    /* lookup the new palette entry */
         movb (%ebp, %ebx), %bl
         movb %al, -2(%edi)        /* write 2 pixels */
         movb %bl, -1(%edi)

   _align_
   end_of_line_8_to_8_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_8_to_8_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_8_to_15 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT *dest_rect)
 */
/* void _colorconv_blit_8_to_16 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_8_to_16_no_mmx:
#else
FUNC (_colorconv_blit_8_to_15)
FUNC (_colorconv_blit_8_to_16)
#endif
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_2, LOOP_RATIO_4)
#else
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_2, LOOP_RATIO_1)
#endif

   movl GLOBL(_colorconv_indexed_palette), %ebp
   xorl %eax, %eax  /* init first line */

   _align_
   next_line_8_to_16_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $2, %ecx
      jz do_one_pixel_8_to_16_no_mmx
#endif

      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_16_no_mmx:
         xorl %ebx, %ebx
         movb (%esi), %al             /* al = pixel1            */
         xorl %edx, %edx
         movb 1(%esi), %bl            /* bl = pixel2            */
         movb 2(%esi), %dl            /* dl = pixel3            */
         movl (%ebp,%eax,4), %eax     /* lookup: ax = pixel1    */
         movl 1024(%ebp,%ebx,4), %ebx /* lookup: bx = pixel2    */
         addl $4, %esi                /* 4 pixels read          */
         orl  %ebx, %eax              /* eax = pixel2..pixel1   */
         xorl %ebx, %ebx
         movl %eax, (%edi)            /* write pixel1, pixel2   */
         movb -1(%esi), %bl           /* bl = pixel4            */
         movl (%ebp,%edx,4), %edx     /* lookup: dx = pixel3    */
         xorl %eax, %eax
         movl 1024(%ebp,%ebx,4), %ebx /* lookup: bx = pixel4    */
         addl $8, %edi                /* 4 pixels written       */
         orl  %ebx, %edx              /* edx = pixel4..pixel3   */
         decl %ecx
         movl %edx, -4(%edi)          /* write pixel3, pixel4   */
         jnz next_block_8_to_16_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_8_to_16_no_mmx:
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_8_to_16_no_mmx

         shrl $1, %ecx
         jnc do_two_pixels_8_to_16_no_mmx

         xorl %eax, %eax
         incl %esi
         movb -1(%esi), %al
         movl (%ebp, %eax, 4), %ebx
         addl $2, %edi
         xorl %eax, %eax
         movw %bx, -2(%edi)

      do_two_pixels_8_to_16_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_8_to_16_no_mmx

         xorl %ebx, %ebx
         movb (%esi), %al
         movb 1(%esi), %bl
         addl $2, %esi
         movl (%ebp, %eax, 4), %eax
         movl 1024(%ebp, %ebx, 4), %ebx
         addl $4, %edi
         orl %eax, %ebx
         xorl %eax, %eax
         movl %ebx, -4(%edi)

    _align_
    end_of_line_8_to_16_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_8_to_16_no_mmx
 
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_8_to_24 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_8_to_24)
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_3, LOOP_RATIO_4)
#else
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_3, LOOP_RATIO_1)
#endif

   movl GLOBL(_colorconv_indexed_palette), %ebp
   xorl %eax, %eax  /* init first line */

   _align_
   next_line_8_to_24_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $2, %ecx
      jz do_one_pixel_8_to_24_no_mmx
#endif

      _align_
      /* 100% Pentium pairable loop */
      /* 12 cycles = 11 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_24_no_mmx:
         xorl %ebx, %ebx
         movb 3(%esi), %al               /* al = pixel4                     */
         movb 2(%esi), %bl               /* bl = pixel3                     */
         xorl %edx, %edx
         movl 3072(%ebp,%eax,4), %eax    /* lookup: eax = pixel4 << 8       */
         movb 1(%esi), %dl               /* dl = pixel 2                    */
         movl 2048(%ebp,%ebx,4), %ebx    /* lookup: ebx = g8b800r8 pixel3   */
         addl $12, %edi                  /* 4 pixels written                */
         movl 1024(%ebp,%edx,4), %edx    /* lookup: edx = b800r8g8 pixel2   */
         movb %bl, %al                   /* eax = pixel4 << 8 | r8 pixel3   */
         movl %eax, -4(%edi)             /* write r8 pixel3..pixel4         */
         xorl %eax, %eax
         movb %dl, %bl                   /* ebx = g8b8 pixel3 | 00g8 pixel2 */
         movb (%esi), %al                /* al = pixel1                     */
         movb %dh, %bh                   /* ebx = g8b8 pixel3 | r8g8 pixel2 */
         andl $0xff000000, %edx          /* edx = b8 pixel2 << 24           */
         movl %ebx, -8(%edi)             /* write g8r8 pixel2..b8g8 pixel3  */
         movl (%ebp,%eax,4), %eax        /* lookup: eax = pixel1            */
         orl  %eax, %edx                 /* edx = b8 pixel2 << 24 | pixel1  */
         xorl %eax, %eax
         movl %edx, -12(%edi)            /* write pixel1..b8 pixel2         */
         addl $4, %esi                   /* 4 pixels read                   */
         decl %ecx
         jnz next_block_8_to_24_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_8_to_24_no_mmx:
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_8_to_24_no_mmx

         shrl $1, %ecx
         jnc do_two_pixels_8_to_24_no_mmx

         xorl %eax, %eax
         movb (%esi), %al                /* al = pixel1                     */
         incl %esi
         movl (%ebp,%eax,4), %eax        /* lookup: eax = pixel1            */
         movw %ax, (%edi)
         shrl $16, %eax
         addl $3, %edi
         movb %al, -1(%edi)
         xorl %eax, %eax

       do_two_pixels_8_to_24_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_8_to_24_no_mmx

         xorl %ebx, %ebx
         movb (%esi), %al                /* al = pixel1                     */
         movb 1(%esi), %bl               /* bl = pixel2                     */
         addl $2, %esi
         movl (%ebp,%eax,4), %eax        /* lookup: eax = pixel1            */
         movl (%ebp,%ebx,4), %ebx        /* lookup: ebx = pixel2            */
         movl %eax, (%edi)               /* write pixel1                    */
         movw %bx, 3(%edi)
         shrl $16, %ebx
         addl $6, %edi
         movb %bl, -1(%edi)
         xorl %eax, %eax

   _align_
   end_of_line_8_to_24_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_8_to_24_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_8_to_32 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_8_to_32_no_mmx:
#else
FUNC (_colorconv_blit_8_to_32)
#endif
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_4, LOOP_RATIO_4)
#else
   INIT_REGISTERS_NO_MMX(SIZE_1, SIZE_4, LOOP_RATIO_1)
#endif

   xorl %eax, %eax  /* init first line */
   movl GLOBL(_colorconv_indexed_palette), %ebp

   _align_
   next_line_8_to_32_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $2, %ecx
      jz do_one_pixel_8_to_32_no_mmx
#endif

      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/4 pixels + 1 cycle loop */
      next_block_8_to_32_no_mmx:
         movb (%esi), %al           /* al = pixel1          */
         xorl %ebx, %ebx
         movb 1(%esi), %bl          /* bl = pixel2          */
         xorl %edx, %edx
         movl (%ebp,%eax,4), %eax   /* lookup: eax = pixel1 */
         movb 2(%esi), %dl          /* dl = pixel3          */
         movl %eax, (%edi)          /* write pixel1         */
         movl (%ebp,%ebx,4), %ebx   /* lookup: ebx = pixel2 */
         xorl %eax, %eax
         movl (%ebp,%edx,4), %edx   /* lookup: edx = pixel3 */
         movl %ebx, 4(%edi)         /* write pixel2         */
         movb 3(%esi), %al          /* al = pixel4          */
         movl %edx, 8(%edi)         /* write pixel3         */
         addl $16, %edi             /* 4 pixels written     */
         movl (%ebp,%eax,4), %eax   /* lookup: eax = pixel4 */
         addl $4, %esi              /* 4 pixels read        */
         movl %eax, -4(%edi)        /* write pixel4         */
         xorl %eax, %eax
         decl %ecx
         jnz next_block_8_to_32_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_8_to_32_no_mmx:
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_8_to_32_no_mmx

         shrl $1, %ecx
         jnc do_two_pixels_8_to_32_no_mmx

         movb (%esi), %al           /* read one pixel */
         incl %esi
         movl (%ebp,%eax,4), %ebx   /* lookup: ebx = pixel */
         addl $4, %edi
         xorl %eax, %eax
         movl %ebx, -4(%edi)        /* write pixel */

      do_two_pixels_8_to_32_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_8_to_32_no_mmx

         movb (%esi), %al           /* read one pixel */
         xorl %ebx, %ebx
         addl $2, %esi
         movb -1(%esi), %bl         /* read another pixel */         
         movl (%ebp,%eax,4), %ecx   /* lookup: ecx = pixel */
         movl (%ebp,%ebx,4), %ebx   /* lookup: ebx = pixel */
         addl $8, %edi
         xorl %eax, %eax
         movl %ecx, -8(%edi)        /* write pixel */
         movl %ebx, -4(%edi)        /* write pixel */

   _align_
   end_of_line_8_to_32_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_8_to_32_no_mmx

   DESTROY_STACK_FRAME
   ret

#endif  /* ALLEGRO_COLOR8 */



#ifdef ALLEGRO_COLOR16

/* void _colorconv_blit_15_to_8 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_15_to_8)
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_1, LOOP_RATIO_2)
#else
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_1, LOOP_RATIO_1)
#endif

   movl GLOBL(_colorconv_rgb_map), %ebp

   _align_
   next_line_15_to_8_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $1, %ecx                /* work in packs of 2 pixels */
      jz do_one_pixel_15_to_8_no_mmx
#endif

      _align_
      next_block_15_to_8_no_mmx:
         movl (%esi), %eax         /* read 2 pixels */
         addl $4, %esi
         addl $2, %edi
         movl %eax, %ebx           /* get bottom 16 bits */
         movl %eax, %edx
         andl $0x781e, %ebx
         andl $0x03c0, %edx
         shrb $1, %bl              /* shift to correct positions */
         shrb $3, %bh
         shrl $2, %edx
         shrl $16, %eax
         orl %edx, %ebx            /* combine to get a 4.4.4 number */
         movl %eax, %edx
         movb (%ebp, %ebx), %bl    /* look it up */
         andl $0x781f, %eax
         andl $0x03c0, %edx
         shrb $1, %al              /* shift to correct positions */
         shrb $3, %ah
         shrl $2, %edx
         orl %edx, %eax            /* combine to get a 4.4.4 number */
         movb (%ebp, %eax), %bh    /* look it up */
         movw %bx, -2(%edi)        /* write 2 pixels */
         decl %ecx
         jnz next_block_15_to_8_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_15_to_8_no_mmx:
         movl MYLOCAL1, %ecx
         shrl $1, %ecx
         jnc end_of_line_15_to_8_no_mmx

         xorl %eax, %eax
         movw (%esi), %ax          /* read 1 pixel */
         addl $2, %esi
         incl %edi
         movl %eax, %ebx
         andl $0x781e, %ebx
         andl $0x03c0, %eax
         shrb $1, %bl              /* shift to correct positions */
         shrb $3, %bh
         shrl $2, %eax
         orl %eax, %ebx            /* combine to get a 4.4.4 number */
         movb (%ebp, %ebx), %bl    /* look it up */
         movb %bl, -1(%edi)        /* write 1 pixel */

   _align_
   end_of_line_15_to_8_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_15_to_8_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_15_to_16 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_15_to_16_no_mmx:
#else
FUNC (_colorconv_blit_15_to_16)
#endif
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_2, LOOP_RATIO_4)
#else
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_2, LOOP_RATIO_1)
#endif

   _align_
   next_line_15_to_16_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $2, %ecx
      jz do_one_pixel_15_to_16_no_mmx
#endif

      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/4 pixels + 1 cycle loop */
      next_block_15_to_16_no_mmx:
         movl (%esi), %eax           /* eax = pixel2 | pixel1  */
         addl $8, %edi               /* 4 pixels written       */
         movl %eax, %ebx             /* ebx = pixel2 | pixel1  */
         andl $0x7fe07fe0, %eax      /* eax = r5g5b0 | r5g5b0  */
         shll $1, %eax               /* eax = r5g6b0 | r5g6b0  */
         andl $0x001f001f, %ebx      /* ebx = r0g0b5 | r0g0b5  */
         orl  %ebx, %eax             /* eax = r5g6b5 | r5g6b5  */
         movl 4(%esi), %edx          /* edx = pixel4 | pixel3  */
         movl %edx, %ebx             /* ebx = pixel4 | pixel3  */
         andl $0x7fe07fe0, %edx      /* edx = r5g5b0 | r5g5b0  */
         shll $1, %edx               /* edx = r5g6b0 | r5g6b0  */
         andl $0x001f001f, %ebx      /* ebx = r0g0b5 | r0g0b5  */
         orl  %ebx, %edx             /* edx = r5g6b5 | r5g6b5  */
         orl  $0x00200020, %eax      /* green gamma correction */
         movl %eax, -8(%edi)         /* write pixel1..pixel2   */
         orl  $0x00200020, %edx      /* green gamma correction */
         movl %edx, -4(%edi)         /* write pixel3..pixel4   */
         addl $8, %esi               /* 4 pixels read          */
         decl %ecx
         jnz next_block_15_to_16_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_15_to_16_no_mmx:
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_15_to_16_no_mmx

         shrl $1, %ecx
         jnc do_two_pixels_15_to_16_no_mmx

         xorl %eax, %eax
         movw (%esi), %ax
         addl $2, %edi
         movl %eax, %ebx
         andl $0x7fe0, %eax
         andl $0x001f, %ebx
         shll $1, %eax
         addl $2, %esi
         orl $0x0020, %eax
         orl %ebx, %eax
         movw %ax, -2(%edi)

      do_two_pixels_15_to_16_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_15_to_16_no_mmx

         movl (%esi), %eax
         addl $4, %edi
         movl %eax, %ebx
         andl $0x7fe07fe0, %eax
         andl $0x001f001f, %ebx
         shll $1, %eax
         addl $4, %esi
         orl $0x00200020, %eax
         orl %ebx, %eax
         movl %eax, -4(%edi)

   _align_
   end_of_line_15_to_16_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_15_to_16_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_15_to_24 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
/* void _colorconv_blit_16_to_24 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_15_to_24)
FUNC (_colorconv_blit_16_to_24)
   CREATE_BIG_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_3, LOOP_RATIO_4)
#else
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_3, LOOP_RATIO_1)
#endif

   movl GLOBL(_colorconv_rgb_scale_5x35), %ebp

   next_line_16_to_24_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $2, %ecx
      jz do_one_pixel_16_to_24_no_mmx
#endif

      _align_
      /* 100% Pentium pairable loop */
      /* 22 cycles = 20 cycles/4 pixels + 1 cycle stack + 1 cycle loop */
      next_block_16_to_24_no_mmx:
         movl %ecx, SPILL_SLOT          /* spill %ecx to the stack          */
         xorl %ebx, %ebx
         xorl %eax, %eax
         movb 7(%esi), %bl              /* bl = high byte pixel4            */
         xorl %edx, %edx
         movb 6(%esi), %al              /* al = low byte pixel4             */
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel4      */
         movb 4(%esi), %dl              /* dl = low byte pixel3             */
         movl 1024(%ebp,%eax,4), %eax   /* lookup: eax = r0g8b8 pixel4      */
         xorl %ecx, %ecx
         addl %ebx, %eax                /* eax = r8g8b8 pixel4              */
         movb 5(%esi), %cl              /* cl = high byte pixel3            */
         shll $8, %eax                  /* eax = r8g8b8 pixel4 << 8         */
         movl 5120(%ebp,%edx,4), %edx   /* lookup: edx = g8b800r0 pixel3    */
         movl 4096(%ebp,%ecx,4), %ecx   /* lookup: ecx = g8b000r8 pixel3    */
         xorl %ebx, %ebx
         addl %ecx, %edx                /* edx = g8b800r8 pixel3            */
         movb %cl, %al                  /* eax = pixel4 << 8 | r8 pixel3    */
         movl %eax, 8(%edi)             /* write r8 pixel3..pixel4          */
         movb 3(%esi), %bl              /* bl = high byte pixel2            */
         xorl %eax, %eax
         xorl %ecx, %ecx
         movb 2(%esi), %al              /* al = low byte pixel2             */
         movl 2048(%ebp,%ebx,4), %ebx   /* lookup: ebx = b000r8g8 pixel2    */
         movb 1(%esi), %cl              /* cl = high byte pixel1            */
         addl $12, %edi                 /* 4 pixels written                 */
         movl 3072(%ebp,%eax,4), %eax   /* lookup: eax = b800r0g8 pixel2    */
         addl $8, %esi                  /* 4 pixels read                    */
         addl %ebx, %eax                /* eax = b800r8g8 pixel2            */
         movl (%ebp,%ecx,4), %ecx       /* lookup: ecx = r8g8b0 pixel1      */
         movb %al, %dl                  /* edx = g8b8 pixel3 | 00g8 pixel2  */
         xorl %ebx, %ebx
         movb %ah, %dh                  /* edx = g8b8 pixel3 | r8g8 pixel2  */
         movb -8(%esi), %bl             /* bl = low byte pixel1             */
         movl %edx, -8(%edi)            /* write g8r8 pixel2..b8g8 pixel3   */
         andl $0xff000000, %eax         /* eax = b8 pixel2 << 24            */
         movl 1024(%ebp,%ebx,4), %ebx   /* lookup: ebx = r0g8b8 pixel1      */
         /* nop */
         addl %ecx, %ebx                /* ebx = r8g8b8 pixel1              */
         movl SPILL_SLOT, %ecx          /* restore %ecx                     */
         orl  %ebx, %eax                /* eax = b8 pixel2 << 24 | pixel1   */
         decl %ecx
         movl %eax, -12(%edi)           /* write pixel1..b8 pixel2          */
         jnz next_block_16_to_24_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_16_to_24_no_mmx:
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_16_to_24_no_mmx

         shrl $1, %ecx
         jnc do_two_pixels_16_to_24_no_mmx

         xorl %eax, %eax
         xorl %ebx, %ebx
         movb 1(%esi), %al              /* al = high byte pixel1            */
         addl $3, %edi                  /* 1 pixel written                  */
         addl $2, %esi                  /* 1 pixel  read                    */
         movl (%ebp,%eax,4), %eax       /* lookup: eax = r8g8b0 pixel1      */
         movb -2(%esi), %bl             /* bl = low byte pixel1             */
         movl 1024(%ebp,%ebx,4), %ebx   /* lookup: ebx = r0g8b8 pixel1      */
         addl %eax, %ebx                /* ebx = r8g8b8 pixel1              */
         movw %bx, -3(%edi)
         shrl $16, %ebx
         movb %bl, -1(%edi)

       do_two_pixels_16_to_24_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_16_to_24_no_mmx

         xorl %eax, %eax
         xorl %ebx, %ebx
         movb 1(%esi), %al              /* al = high byte pixel1            */
         addl $6, %edi                  /* 1 pixel written                  */
         addl $4, %esi                  /* 1 pixel  read                    */
         movl (%ebp,%eax,4), %eax       /* lookup: eax = r8g8b0 pixel1      */
         movb -4(%esi), %bl             /* bl = low byte pixel1             */
         movl 1024(%ebp,%ebx,4), %ebx   /* lookup: ebx = r0g8b8 pixel1      */
         addl %eax, %ebx                /* ebx = r8g8b8 pixel1              */
         xorl %eax, %eax
         movl %ebx, -6(%edi)            /* write pixel1                     */
         movb -1(%esi), %al             /* al = high byte pixel2            */
         xorl %ebx, %ebx
         movl (%ebp,%eax,4), %eax       /* lookup: eax = r8g8b0 pixel2      */
         movb -2(%esi), %bl             /* bl = low byte pixel2             */
         movl 1024(%ebp,%ebx,4), %ebx   /* lookup: ebx = r0g8b8 pixel2      */
         addl %eax, %ebx                /* ebx = r8g8b8 pixel2              */
         movw %bx, -3(%edi)             /* write pixel2                     */
         shrl $16, %ebx
         movb %bl, -1(%edi)

   _align_
   end_of_line_16_to_24_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_16_to_24_no_mmx

   DESTROY_BIG_STACK_FRAME
   ret



/* void _colorconv_blit_15_to_32 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
/* void _colorconv_blit_16_to_32 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_15_to_32_no_mmx:
_colorconv_blit_16_to_32_no_mmx:
#else
FUNC (_colorconv_blit_15_to_32)
FUNC (_colorconv_blit_16_to_32)
#endif
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_4, LOOP_RATIO_2)
#else
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_4, LOOP_RATIO_1)
#endif

   movl GLOBL(_colorconv_rgb_scale_5x35), %ebp
   xorl %eax, %eax  /* init first line */

   _align_
   next_line_16_to_32_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $1, %ecx
      jz do_one_pixel_16_to_32_no_mmx
#endif

      _align_
      /* 100% Pentium pairable loop */
      /* 10 cycles = 9 cycles/2 pixels + 1 cycle loop */
      next_block_16_to_32_no_mmx:
         xorl %ebx, %ebx
         movb (%esi), %al               /* al = low byte pixel1        */
         xorl %edx, %edx
         movb 1(%esi), %bl              /* bl = high byte pixel1       */
         movl 1024(%ebp,%eax,4), %eax   /* lookup: eax = r0g8b8 pixel1 */
         movb 2(%esi), %dl              /* dl = low byte pixel2        */
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel1 */
         addl $8, %edi                  /* 2 pixels written            */
         addl %ebx, %eax                /* eax = r8g8b8 pixel1         */
         xorl %ebx, %ebx
         movl 1024(%ebp,%edx,4), %edx   /* lookup: edx = r0g8b8 pixel2 */
         movb 3(%esi), %bl              /* bl = high byte pixel2       */
         movl %eax, -8(%edi)            /* write pixel1                */
         xorl %eax, %eax
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel2 */
         addl $4, %esi                  /* 4 pixels read               */
         addl %ebx, %edx                /* edx = r8g8b8 pixel2         */
         decl %ecx
         movl %edx, -4(%edi)            /* write pixel2                */
         jnz next_block_16_to_32_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_16_to_32_no_mmx:
         movl MYLOCAL1, %ecx            /* restore width */
         shrl $1, %ecx
         jnc end_of_line_16_to_32_no_mmx

         xorl %ebx, %ebx
         movb (%esi), %al               /* al = low byte pixel1        */
         addl $4, %edi                  /* 2 pixels written            */
         movb 1(%esi), %bl              /* bl = high byte pixel1       */
         movl 1024(%ebp,%eax,4), %eax   /* lookup: eax = r0g8b8 pixel1 */
         movl (%ebp,%ebx,4), %ebx       /* lookup: ebx = r8g8b0 pixel1 */
         addl $2, %esi
         addl %eax, %ebx                /* ebx = r8g8b8 pixel1         */
         xorl %eax, %eax
         movl %ebx, -4(%edi)            /* write pixel1                */

      _align_
      end_of_line_16_to_32_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_16_to_32_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_16_to_8 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_16_to_8)
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_1, LOOP_RATIO_2)
#else
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_1, LOOP_RATIO_1)
#endif

   movl GLOBL(_colorconv_rgb_map), %ebp

   _align_
   next_line_16_to_8_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $1, %ecx                /* work in packs of 2 pixels */
      jz do_one_pixel_16_to_8_no_mmx
#endif

      _align_
      next_block_16_to_8_no_mmx:
         movl (%esi), %eax         /* read 2 pixels */
         addl $4, %esi
         addl $2, %edi
         movl %eax, %ebx           /* get bottom 16 bits */
         movl %eax, %edx
         andl $0xf01e, %ebx
         andl $0x0780, %edx
         shrb $1, %bl              /* shift to correct positions */
         shrb $4, %bh
         shrl $3, %edx
         shrl $16, %eax
         orl %edx, %ebx            /* combine to get a 4.4.4 number */
         movl %eax, %edx
         movb (%ebp, %ebx), %bl    /* look it up */         
         andl $0xf01e, %eax
         andl $0x0780, %edx
         shrb $1, %al              /* shift to correct positions */
         shrb $4, %ah
         shrl $3, %edx
         orl %edx, %eax            /* combine to get a 4.4.4 number */
         movb (%ebp, %eax), %bh    /* look it up */
         movw %bx, -2(%edi)        /* write 2 pixels */
         decl %ecx
         jnz next_block_16_to_8_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_16_to_8_no_mmx:
         movl MYLOCAL1, %ecx
         shrl $1, %ecx
         jnc end_of_line_16_to_8_no_mmx

         xorl %eax, %eax
         movw (%esi), %ax          /* read 1 pixel */
         addl $2, %esi
         incl %edi
         movl %eax, %ebx
         andl $0xf01e, %ebx
         andl $0x0780, %eax
         shrb $1, %bl              /* shift to correct positions */
         shrb $4, %bh
         shrl $3, %eax
         orl %eax, %ebx            /* combine to get a 4.4.4 number */
         movb (%ebp, %ebx), %bl    /* look it up */
         movb %bl, -1(%edi)        /* write 1 pixel */

   _align_
   end_of_line_16_to_8_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_16_to_8_no_mmx

   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_16_to_15 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_16_to_15_no_mmx:
#else
FUNC (_colorconv_blit_16_to_15)
#endif
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_2, LOOP_RATIO_4)
#else
   INIT_REGISTERS_NO_MMX(SIZE_2, SIZE_2, LOOP_RATIO_1)
#endif

   _align_
   next_line_16_to_15_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $2, %ecx
      jz do_one_pixel_16_to_15_no_mmx
#endif

      _align_
      /* 100% Pentium pairable loop */
      /* 9 cycles = 8 cycles/4 pixels + 1 cycle loop */
      next_block_16_to_15_no_mmx:
         movl (%esi), %eax         /* eax = pixel2 | pixel1 */
         addl $8, %edi             /* 4 pixels written      */
         movl %eax, %ebx           /* ebx = pixel2 | pixel1 */
         andl $0xffc0ffc0, %eax    /* eax = r5g6b0 | r5g6b0 */
         shrl $1, %eax             /* eax = r5g5b0 | r5g5b0 */
         andl $0x001f001f, %ebx    /* ebx = r0g0b5 | r0g0b5 */
         orl %ebx, %eax            /* eax = r5g5b5 | r5g5b5 */
         movl 4(%esi), %edx        /* edx = pixel4 | pixel3 */
         movl %edx, %ebx           /* ebx = pixel4 | pixel3 */
         andl $0xffc0ffc0, %edx    /* edx = r5g6b0 | r5g6b0 */
         shrl $1, %edx             /* edx = r5g5b0 | r5g5b0 */
         andl $0x001f001f, %ebx    /* ebx = r0g0b5 | r0g0b5 */
         movl %eax, -8(%edi)       /* write pixel1..pixel2  */
         orl %ebx, %edx            /* edx = r5g5b5 | r5g5b5 */
         movl %edx, -4(%edi)       /* write pixel3..pixel4  */
         addl $8, %esi             /* 4 pixels read         */
         decl %ecx
         jnz next_block_16_to_15_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_16_to_15_no_mmx:
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_16_to_15_no_mmx
         
         shrl $1, %ecx
         jnc do_two_pixels_16_to_15_no_mmx

         xorl %eax, %eax
         movw (%esi), %ax
         addl $2, %edi
         movl %eax, %ebx
         andl $0xffc0ffc0, %eax
         andl $0x001f001f, %ebx
         shrl $1, %eax
         addl $2, %esi
         orl %ebx, %eax
         movw %ax, -2(%edi)

      do_two_pixels_16_to_15_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_16_to_15_no_mmx

         movl (%esi), %eax
         addl $4, %edi
         movl %eax, %ebx
         andl $0xffc0ffc0, %eax
         andl $0x001f001f, %ebx
         shrl $1, %eax
         addl $4, %esi
         orl %ebx, %eax
         movl %eax, -4(%edi)

   _align_
   end_of_line_16_to_15_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_16_to_15_no_mmx

   DESTROY_STACK_FRAME
   ret

#endif  /* ALLEGRO_COLOR16 */



#ifdef ALLEGRO_COLOR24

/* void _colorconv_blit_24_to_8 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_24_to_8)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_1, LOOP_RATIO_1)
   movl GLOBL(_colorconv_rgb_map), %ebp
   CONV_TRUE_TO_8_NO_MMX(24_to_8_no_mmx, 3)
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_24_to_15 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_24_to_15)
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_2, LOOP_RATIO_2)
#else
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_2, LOOP_RATIO_1)
#endif

   CONV_TRUE_TO_15_NO_MMX(24_to_15_no_mmx, 3)
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_24_to_16 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_24_to_16)
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_2, LOOP_RATIO_2)
#else
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_2, LOOP_RATIO_1)
#endif

   CONV_TRUE_TO_16_NO_MMX(24_to_16_no_mmx, 3)
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_24_to_32 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_24_to_32_no_mmx:
#else
FUNC (_colorconv_blit_24_to_32)
#endif
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_4, LOOP_RATIO_4)
#else
   INIT_REGISTERS_NO_MMX(SIZE_3, SIZE_4, LOOP_RATIO_1)
#endif

   _align_
   next_line_24_to_32_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $2, %ecx
      jz do_one_pixel_24_to_32_no_mmx
#endif

      _align_
      /* 100% Pentium pairable loop */
      /* 9 cycles = 8 cycles/4 pixels + 1 cycle loop */
      next_block_24_to_32_no_mmx:
         movl 4(%esi), %ebx        /* ebx = r8g8 pixel2         */
         movl (%esi), %eax         /* eax = pixel1              */
         shll $8, %ebx             /* ebx = r8g8b0 pixel2       */
         movl 8(%esi), %edx        /* edx = pixel4 | r8 pixel 3 */
         movl %eax, (%edi)         /* write pixel1              */
         movb 3(%esi), %bl         /* ebx = pixel2              */
         movl %edx, %eax           /* eax = r8 pixel3           */
         movl %ebx, 4(%edi)        /* write pixel2              */
         shll $16, %eax            /* eax = r8g0b0 pixel3       */
         addl $16, %edi            /* 4 pixels written          */
         shrl $8, %edx             /* edx = pixel4              */
         movb 6(%esi), %al         /* eax = r8g0b8 pixel3       */
         movl %edx, -4(%edi)       /* write pixel4              */
         movb 7(%esi), %ah         /* eax = r8g8b8 pixel3       */
         movl %eax, -8(%edi)       /* write pixel3              */
         addl $12, %esi            /* 4 pixels read             */
         decl %ecx
         jnz next_block_24_to_32_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_24_to_32_no_mmx:
         movl MYLOCAL1, %ecx      /* restore width */
         andl $3, %ecx
         jz end_of_line_24_to_32_no_mmx

         shrl $1, %ecx
         jnc do_two_pixels_24_to_32_no_mmx

         xorl %eax, %eax
         xorl %ebx, %ebx
         movw (%esi), %ax       /* read one pixel */
         movb 2(%esi), %bl
         addl $3, %esi
         shll $16, %ebx
         orl %ebx, %eax
         movl %eax, (%edi)         /* write */
         addl $4, %edi

      do_two_pixels_24_to_32_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_24_to_32_no_mmx

         xorl %ebx, %ebx
         movl (%esi), %eax         /* read 2 pixels */
         movw 4(%esi), %bx
         movl %eax, %ecx
         shll $8, %ebx
         shrl $24, %ecx
         addl $6, %esi
         orl %ecx, %ebx
         movl %eax, (%edi)         /* write */
         movl %ebx, 4(%edi)
         addl $8, %edi

   _align_
   end_of_line_24_to_32_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_24_to_32_no_mmx

   DESTROY_STACK_FRAME
   ret

#endif  /* ALLEGRO_COLOR24 */



#ifdef ALLEGRO_COLOR32

/* void _colorconv_blit_32_to_8 (struct GRAPHICS_RECT *src_rect,
 *                               struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorconv_blit_32_to_8)
   CREATE_STACK_FRAME
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_1, LOOP_RATIO_1)
   movl GLOBL(_colorconv_rgb_map), %ebp
   CONV_TRUE_TO_8_NO_MMX(32_to_8_no_mmx, 4)
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_32_to_15 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_32_to_15_no_mmx:
#else
FUNC (_colorconv_blit_32_to_15)
#endif
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_2, LOOP_RATIO_2)
#else
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_2, LOOP_RATIO_1)
#endif

   CONV_TRUE_TO_15_NO_MMX(32_to_15_no_mmx, 4)
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_32_to_16 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_32_to_16_no_mmx:
#else
FUNC (_colorconv_blit_32_to_16)
#endif
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_2, LOOP_RATIO_2)
#else
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_2, LOOP_RATIO_1)
#endif

   CONV_TRUE_TO_16_NO_MMX(32_to_16_no_mmx, 4)
   DESTROY_STACK_FRAME
   ret



/* void _colorconv_blit_32_to_24 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
#ifdef ALLEGRO_MMX
_align_
_colorconv_blit_32_to_24_no_mmx:
#else
FUNC (_colorconv_blit_32_to_24)
#endif
   CREATE_STACK_FRAME

#ifdef ALLEGRO_COLORCONV_ALIGNED_WIDTH
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_3, LOOP_RATIO_4)
#else
   INIT_REGISTERS_NO_MMX(SIZE_4, SIZE_3, LOOP_RATIO_1)
#endif

   movl $0xFFFFFF, %ebp

   _align_
   next_line_32_to_24_no_mmx:
      movl MYLOCAL1, %ecx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      shrl $2, %ecx
      jz do_one_pixel_32_to_24_no_mmx
#endif

      _align_
      /* 100% Pentium pairable loop */
      /* 12 cycles = 11 cycles/4 pixels + 1 cycle loop */
      next_block_32_to_24_no_mmx:
         movl 4(%esi), %ebx     /* ebx = [ARGB](2)     */
         addl $12, %edi         /* 4 pixels written                */
         movl (%esi), %eax      /* eax = [ARGB](1)     */
         movl %ebx, %edx        /* edx = [ARGB](2)     */
         shll $24, %edx         /* edx = [B...](2)     */
         andl %ebp, %ebx        /* ebx = [.RGB](2)     */
         shrl $8, %ebx          /* ebx = [..RG](2)     */
         andl %ebp, %eax        /* eax = [.RGB](1)     */
         orl %edx, %eax         /* eax = [BRGB](2)(1)  */
         movl 8(%esi), %edx     /* edx = [ARGB](3)     */
         movl %eax, -12(%edi)   /* write [BRGB](2)(1)  */
         movl %edx, %eax        /* eax = [ARGB](3)     */
         shll $16, %edx         /* edx = [GB..](3)     */
         andl %ebp, %eax        /* eax = [.RGB](3)     */
         shrl $16, %eax         /* eax = [...R](3)     */
         orl %edx, %ebx         /* ebx = [GBRG](3)(2)  */
         movl 12(%esi), %edx    /* edx = [ARGB](4)     */
         movl %ebx, -8(%edi)    /* write [GBRG](3)(2)  */
         shll $8, %edx          /* edx = [RGB.](4)     */
         addl $16, %esi         /* 4 pixels read                   */
         orl %edx, %eax         /* eax = [RGBR](4)(3)  */
         decl %ecx
         movl %eax, -4(%edi)    /* write [RGBR](4)(3)  */
         jnz next_block_32_to_24_no_mmx

#ifndef ALLEGRO_COLORCONV_ALIGNED_WIDTH
      do_one_pixel_32_to_24_no_mmx:
         movl MYLOCAL1, %ecx
         andl $3, %ecx
         jz end_of_line_32_to_24_no_mmx

         shrl $1, %ecx
         jnc do_two_pixels_32_to_24_no_mmx

         movl (%esi), %eax      /* read one pixel */
         addl $4, %esi
         movw %ax, (%edi)       /* write bottom 24 bits */
         shrl $16, %eax
         addl $3, %edi
         movb %al, -1(%edi)

      do_two_pixels_32_to_24_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_32_to_24_no_mmx

         movl (%esi), %eax      /* read two pixels */
         movl 4(%esi), %ebx
         addl $8, %esi
         movl %ebx, %ecx
         andl %ebp, %eax
         shll $24, %ebx
         orl %ebx, %eax
         shrl $8, %ecx
         movl %eax, (%edi)      /* write bottom 48 bits */
         addl $6, %edi
         movw %cx, -2(%edi)

   _align_
   end_of_line_32_to_24_no_mmx:
#endif

      addl MYLOCAL2, %esi
      addl MYLOCAL3, %edi
      decl MYLOCAL4
      jnz next_line_32_to_24_no_mmx

   DESTROY_STACK_FRAME
   ret

#endif  /* ALLEGRO_COLOR32 */



#ifndef ALLEGRO_NO_COLORCOPY

/********************************************************************************************/
/* color copy routines                                                                      */
/*  386 and MMX support                                                                     */
/********************************************************************************************/


/* void _colorcopy (struct GRAPHICS_RECT *src_rect, struct GRAPHICS_RECT *dest_rect, int bpp)
 */
FUNC (_colorcopy)
   
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %esi
   pushl %edi

   /* init register values */

   movl ARG3, %ebx
   movl ARG1, %eax                  /* eax = src_rect                 */
   movl GFXRECT_WIDTH(%eax), %eax
   mull %ebx

   movl %eax, %ecx                  /* ecx = src_rect->width * bpp    */
   movl ARG1, %eax
   movl GFXRECT_HEIGHT(%eax), %edx  /* edx = src_rect->height         */
   movl GFXRECT_DATA(%eax), %esi    /* esi = src_rect->data           */
   movl GFXRECT_PITCH(%eax), %eax   /* eax = src_rect->pitch          */
   subl %ecx, %eax                  /* eax = (src_rect->pitch) - ecx  */

   movl ARG2, %ebx                  /* ebx = dest_rect                */
   movl GFXRECT_DATA(%ebx), %edi    /* edi = dest_rect->data          */
   movl GFXRECT_PITCH(%ebx), %ebx   /* ebx = dest_rect->pitch         */
   subl %ecx, %ebx                  /* ebx = (dest_rect->pitch) - ecx */

   movl %ecx, %ebp                  /* save for later */

#ifdef ALLEGRO_MMX
   movl GLOBL(cpu_capabilities), %ecx     /* if MMX is enabled (or not disabled :) */
   andl $CPU_MMX, %ecx
   jz next_line_no_mmx

   movl %ebp, %ecx
   shrl $5, %ecx                    /* we work with 32 pixels at a time */
   movd %ecx, %mm6

   _align_
   next_line:
      movd %mm6, %ecx
      orl %ecx, %ecx
      jz do_one_byte

      _align_
      next_block:
         movq (%esi), %mm0           /* read */
         movq 8(%esi), %mm1
         addl $32, %esi
         movq -16(%esi), %mm2
         movq -8(%esi), %mm3
         movq %mm0, (%edi)           /* write */
         movq %mm1, 8(%edi)
         addl $32, %edi
         movq %mm2, -16(%edi)
         movq %mm3, -8(%edi)
         decl %ecx
         jnz next_block

      do_one_byte:
         movl %ebp, %ecx
         andl $31, %ecx
         jz end_of_line

         shrl $1, %ecx
         jnc do_two_bytes

         movsb      

      do_two_bytes:
         shrl $1, %ecx
         jnc do_four_bytes

         movsb
         movsb

      _align_
      do_four_bytes:
         shrl $1, %ecx
         jnc do_eight_bytes

         movsl

      _align_
      do_eight_bytes:
         shrl $1, %ecx
         jnc do_sixteen_bytes

         movq (%esi), %mm0
         addl $8, %esi
         movq %mm0, (%edi)
         addl $8, %edi

      _align_
      do_sixteen_bytes:
         shrl $1, %ecx
         jnc end_of_line

         movq (%esi), %mm0
         movq 8(%esi), %mm1
         addl $16, %esi
         movq %mm0, (%edi)
         movq %mm1, 8(%edi)
         addl $16, %edi

   _align_
   end_of_line:
      addl %eax, %esi
      addl %ebx, %edi
      decl %edx
      jnz next_line

   emms
   jmp end_of_function
#endif

   _align_
   next_line_no_mmx:
      movl %ebp, %ecx
      shrl $2, %ecx
      orl %ecx, %ecx
      jz do_one_byte_no_mmx

      rep; movsl

      do_one_byte_no_mmx:
         movl %ebp, %ecx
         andl $3, %ecx
         jz end_of_line_no_mmx

         shrl $1, %ecx
         jnc do_two_bytes_no_mmx

         movsb

      do_two_bytes_no_mmx:
         shrl $1, %ecx
         jnc end_of_line_no_mmx

         movsb
         movsb

   _align_
   end_of_line_no_mmx:
      addl %eax, %esi
      addl %ebx, %edi
      decl %edx
      jnz next_line_no_mmx

end_of_function:
   popl %edi
   popl %esi
   popl %ebx
   popl %ebp

   ret



#ifdef ALLEGRO_COLOR16

/* void _colorcopy_blit_15_to_15 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
/* void _colorcopy_blit_16_to_16 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorcopy_blit_15_to_15)
FUNC (_colorcopy_blit_16_to_16)

   pushl %ebp
   movl %esp, %ebp

   pushl $2
   pushl ARG2
   pushl ARG1

   call GLOBL(_colorcopy)
   addl $12, %esp

   popl %ebp
   ret

#endif  /* ALLEGRO_COLOR16 */



#ifdef ALLEGRO_COLOR24

/* void _colorcopy_blit_24_to_24 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorcopy_blit_24_to_24)

   pushl %ebp
   movl %esp, %ebp

   pushl $3
   pushl ARG2
   pushl ARG1

   call GLOBL(_colorcopy)
   addl $12, %esp

   popl %ebp
   ret

#endif  /* ALLEGRO_COLOR24 */



#ifdef ALLEGRO_COLOR32

/* void _colorcopy_blit_32_to_32 (struct GRAPHICS_RECT *src_rect,
 *                                struct GRAPHICS_RECT *dest_rect)
 */
FUNC (_colorcopy_blit_32_to_32)

   pushl %ebp
   movl %esp, %ebp

   pushl $4
   pushl ARG2
   pushl ARG1

   call GLOBL(_colorcopy)
   addl $12, %esp

   popl %ebp
   ret

#endif  /* ALLEGRO_COLOR32 */

#endif  /* ALLEGRO_NO_COLORCOPY */

