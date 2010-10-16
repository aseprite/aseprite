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
 *      32 bit sprite drawing (written for speed, not readability :-)
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "sprite.inc"

#ifdef ALLEGRO_COLOR32

.text



/* void _linear_draw_sprite32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite onto a linear bitmap at the specified x, y position, 
 *  using a masked drawing mode where zero pixels are not output.
 */
FUNC(_linear_draw_sprite32)

   START_SPRITE_DRAW(sprite)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   shrl $2, %eax
   subl S_W, %eax                /* - w */
   shll $2, %eax 
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   _align_
   SPRITE_LOOP(draw_sprite) 
   movl (%esi), %ebx             /* read a pixel */ 
   cmpl $MASK_COLOR_32, %ebx     /* test */ 
   jz draw_sprite_skip 
   movl %ebx, %es:(%eax)         /* write */ 
   draw_sprite_skip: 
   addl $4, %esi 
   addl $4, %eax 
   SPRITE_END_X(draw_sprite) 
   SPRITE_END_Y(draw_sprite) 

   _align_
sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite32() */

FUNC(_linear_draw_sprite32_end)
   ret




/* void _linear_draw_256_sprite32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a 256 color sprite onto a linear bitmap at the specified x, y 
 *  position, using a masked drawing mode where zero pixels are not output.
 */
FUNC(_linear_draw_256_sprite32)

   START_SPRITE_DRAW(sprite256)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   pushl %edx                    /* get color expansion lookup table */
   pushl $32
   call *GLOBL(_palette_expansion_table)
   addl $4, %esp
   popl %edx
   movl %eax, %edi

   _align_
   SPRITE_LOOP(draw_sprite256) 
   movzbl (%esi), %ebx           /* read a pixel */ 
   orb %bl, %bl                  /* test */
   jz draw_256_sprite_skip
   movl (%edi, %ebx, 4), %ebx    /* lookup in palette table */
   movl %ebx, %es:(%eax)         /* write */
draw_256_sprite_skip:
   incl %esi
   addl $4, %eax
   SPRITE_END_X(draw_sprite256)
   SPRITE_END_Y(draw_sprite256)

   _align_
sprite256_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_256_sprite32() */




/* void _linear_draw_sprite_v_flip32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping vertically.
 */
FUNC(_linear_draw_sprite_v_flip32)

   START_SPRITE_DRAW(sprite_v_flip)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   shrl $2, %eax
   addl S_W, %eax                /* + w */
   shll $2, %eax
   negl %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   _align_
   SPRITE_LOOP(v_flip) 
   movl (%esi), %ebx             /* read pixel */
   cmpl $MASK_COLOR_32, %ebx     /* test */
   je sprite_v_flip_skip 
   movl %ebx, %es:(%eax)         /* write */
sprite_v_flip_skip: 
   addl $4, %esi 
   addl $4, %eax 
   SPRITE_END_X(v_flip)
   SPRITE_END_Y(v_flip)

sprite_v_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_v_flip32() */




/* void _linear_draw_sprite_h_flip32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping horizontally.
 */
FUNC(_linear_draw_sprite_h_flip32)

   START_SPRITE_DRAW(sprite_h_flip)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   shrl $2, %eax
   addl S_W, %eax                /* + w */
   shll $2, %eax 
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   leal (%esi, %ecx, 4), %esi
   subl S_LGAP, %esi 
   subl $4, %esi                 /* esi = sprite data ptr */

   _align_
   SPRITE_LOOP(h_flip) 
   movl (%esi), %ebx             /* read pixel */
   cmpl $MASK_COLOR_32, %ebx     /* test  */
   je sprite_h_flip_skip 
   movl %ebx, %es:(%eax)         /* write */
sprite_h_flip_skip: 
   subl $4, %esi 
   addl $4, %eax 
   SPRITE_END_X(h_flip)
   SPRITE_END_Y(h_flip)

sprite_h_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_h_flip32() */




/* void _linear_draw_sprite_vh_flip32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping both vertically and horizontally.
 */
FUNC(_linear_draw_sprite_vh_flip32)

   START_SPRITE_DRAW(sprite_vh_flip)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   shrl $2, %eax
   subl S_W, %eax                /* - w */
   shll $2, %eax
   negl %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   leal (%esi, %ecx, 4), %esi
   subl S_LGAP, %esi 
   subl $4, %esi                 /* esi = sprite data ptr */

   _align_
   SPRITE_LOOP(vh_flip) 
   movl (%esi), %ebx             /* read pixel */
   cmpl $MASK_COLOR_32, %ebx     /* test  */
   je sprite_vh_flip_skip 
   movl %ebx, %es:(%eax)         /* write */
sprite_vh_flip_skip: 
   subl $4, %esi 
   addl $4, %eax 
   SPRITE_END_X(vh_flip)
   SPRITE_END_Y(vh_flip)

sprite_vh_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_vh_flip32() */




/* void _linear_draw_trans_sprite32(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a translucent sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_sprite32)

   START_SPRITE_DRAW(trans_sprite)

   movl BMP_VTABLE(%esi), %eax   /* check the color depth */
   cmpl $8, VTABLE_COLOR_DEPTH(%eax)
   je trans_sprite_256

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   shrl $2, %eax
   subl S_W, %eax                /* - w */
   shll $2, %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   TT_SPRITE_LOOP(trans, %edi) 
   movl (%esi), %eax             /* read source pixel */
   cmpl $MASK_COLOR_32, %eax
   jz trans_sprite_skip
   pushl GLOBL(_blender_alpha)
   pushl %es:(%ebx, %edi)        /* read memory pixel */
   pushl %eax
   call *GLOBL(_blender_func32)  /* blend */
   addl $12, %esp
   movl %eax, %es:(%ebx)         /* write the result */
trans_sprite_skip:
   addl $4, %ebx
   addl $4, %esi
   T_SPRITE_END_X(trans)
   /* no cleanup at end of line */
   SPRITE_END_Y(trans)

trans_sprite_done:
   END_SPRITE_DRAW()
   ret


   /* special version for drawing 8 bit alpha channels onto 32 bit RGBA */
   _align_
trans_sprite_256:
   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   TT_SPRITE_LOOP(trans_256, %edi) 
   movzbl (%esi), %eax           /* read source pixel */
   orl %eax, %eax
   jz trans_sprite_256_skip
   pushl GLOBL(_blender_alpha)
   pushl %es:(%ebx, %edi)        /* read memory pixel */
   pushl %eax
   call *GLOBL(_blender_func32)  /* blend */
   addl $12, %esp
   movl %eax, %es:(%ebx)         /* write the result */
trans_sprite_256_skip:
   addl $4, %ebx 
   incl %esi 
   T_SPRITE_END_X(trans_256)
   /* no cleanup at end of line */ 
   SPRITE_END_Y(trans_256)

   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_trans_sprite32() */




/* void _linear_draw_lit_sprite32(BITMAP *bmp, BITMAP *sprite, int x, y, color);
 *  Draws a lit sprite onto a linear bitmap.
 */
FUNC(_linear_draw_lit_sprite32)

   #define ALPHA     ARG5

   START_SPRITE_DRAW(lit_sprite)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   shrl $2, %eax
   subl S_W, %eax                /* - w */
   shll $2, %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X
   shll $2, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl ALPHA, %edi

   _align_
   LT_SPRITE_LOOP(lit_sprite) 
   movl (%esi), %eax             /* read pixel */
   cmpl $MASK_COLOR_32, %eax
   jz lit_sprite_skip
   pushl %edi
   pushl %eax
   pushl GLOBL(_blender_col_32)
   call *GLOBL(_blender_func32)  /* blend */
   addl $12, %esp
   movl %eax, %es:(%ebx)         /* write pixel */
lit_sprite_skip:
   addl $4, %esi
   addl $4, %ebx
   T_SPRITE_END_X(lit_sprite)
   SPRITE_END_Y(lit_sprite)

lit_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_lit_sprite32() */




/* void _linear_draw_character32(BITMAP *bmp, BITMAP *sprite, int x, y, color, bg);
 *  For proportional font output onto a linear bitmap: uses the sprite as 
 *  a mask, replacing all set pixels with the specified color.
 */
FUNC(_linear_draw_character32)

   #define COLOR  ARG5
   #define BG     ARG6

   START_SPRITE_DRAW(draw_char)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $2, S_X

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl COLOR, %ebx              /* bx = text color */
   movl BG, %edi                 /* di = background color */
   cmpl $0, %edi
   jl draw_masked_char

   /* opaque (bg >= 0) character output */
   _align_
   SPRITE_LOOP(draw_opaque_char) 
   cmpb $0, (%esi)               /* test pixel */
   jz draw_opaque_background
   movl %ebx, %es:(%eax)         /* write pixel */
   jmp draw_opaque_done
draw_opaque_background: 
   movl %edi, %es:(%eax)         /* write background */
draw_opaque_done:
   incl %esi 
   addl $4, %eax 
   SPRITE_END_X(draw_opaque_char)
   SPRITE_END_Y(draw_opaque_char)
   jmp draw_char_done

   /* masked (bg -1) character output */
   _align_
draw_masked_char:
   SPRITE_LOOP(draw_masked_char) 
   cmpb $0, (%esi)               /* test pixel */
   jz draw_masked_skip
   movl %ebx, %es:(%eax)         /* write pixel */
draw_masked_skip:
   incl %esi 
   addl $4, %eax 
   SPRITE_END_X(draw_masked_char)
   SPRITE_END_Y(draw_masked_char)

draw_char_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_character32() */




/* void _linear_draw_rle_sprite32(BITMAP *bmp, RLE_SPRITE *sprite, int x, y)
 *  Draws an RLE sprite onto a linear bitmap at the specified position.
 */
FUNC(_linear_draw_rle_sprite32)

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 4), %edi


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      rep ; movsl


   /* no special initialisation required */
   #define INIT_FAST_RLE_LOOP()


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      rep ; movsl


   /* tests an RLE command byte */
   #define TEST_RLE_COMMAND(done, skip)                                      \
      cmpl $MASK_COLOR_32, %eax                                            ; \
      je done                                                              ; \
      testl %eax, %eax                                                     ; \
      js skip


   /* adds the offset in %eax onto the destination address */
   #define ADD_EAX_EDI()                                                     \
      leal (%edi, %eax, 4), %edi


   /* no zero or sign extend required */
   #define RLE_ZEX_EAX()
   #define RLE_SEX_EAX()


   /* this can be a simple copy... */
   #define RLE_ZEX_ECX()                                                     \
      movl %eax, %ecx


   /* do it! */
   DO_RLE(rle, 4, l, %eax, $MASK_COLOR_32)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_trans_rle_sprite32(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                      int x, int y)
 *  Draws a translucent RLE sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_rle_sprite32)

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      READ_BANK()                   /* select read bank */                 ; \
      movl %eax, R_TMP                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                  /* select write bank */                ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 4), %edi                                           ; \
      subl %eax, R_TMP              /* calculate read/write diff */


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      pushl %ebx                                                           ; \
      movl %ecx, R_TMP2                                                    ; \
									   ; \
   trans_rle_clipped_run_loop##n:                                          ; \
      movl R_TMP, %edx                                                     ; \
      pushl GLOBL(_blender_alpha)                                          ; \
      pushl %es:(%edi, %edx)        /* read memory pixel */                ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      call *GLOBL(_blender_func32)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movl %eax, %es:(%edi)         /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rle_clipped_run_loop##n                                     ; \
									   ; \
      popl %ebx


   /* initialise the drawing loop */
   #define INIT_FAST_RLE_LOOP()


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      movl %ecx, R_TMP2                                                    ; \
									   ; \
   trans_rle_run_loop:                                                     ; \
      movl R_TMP, %edx                                                     ; \
      pushl GLOBL(_blender_alpha)                                          ; \
      pushl %es:(%edi, %edx)        /* read memory pixel */                ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      call *GLOBL(_blender_func32)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movl %eax, %es:(%edi)         /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rle_run_loop


   /* do it! */
   DO_RLE(rle_trans, 4, l, %eax, $MASK_COLOR_32)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_lit_rle_sprite32(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                    int x, int y, int color)
 *  Draws a tinted RLE sprite onto a linear bitmap.
 */
FUNC(_linear_draw_lit_rle_sprite32)

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 4), %edi


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      pushl %ebx                                                           ; \
      movl R_COLOR, %ebx                                                   ; \
      movl %ecx, R_TMP                                                     ; \
									   ; \
   lit_rle_clipped_run_loop##n:                                            ; \
      pushl %ebx                                                           ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      pushl GLOBL(_blender_col_32)                                         ; \
      call *GLOBL(_blender_func32)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movl %eax, %es:(%edi)         /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl R_TMP                                                           ; \
      jg lit_rle_clipped_run_loop##n                                       ; \
									   ; \
      popl %ebx


   /* initialise the drawing loop */
   #define INIT_FAST_RLE_LOOP()                                              \
      movl R_COLOR, %ebx


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      movl %ecx, R_TMP                                                     ; \
									   ; \
   lit_rle_run_loop:                                                       ; \
      pushl %ebx                                                           ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      pushl GLOBL(_blender_col_32)                                         ; \
      call *GLOBL(_blender_func32)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movl %eax, %es:(%edi)         /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %edi                                                        ; \
      decl R_TMP                                                           ; \
      jg lit_rle_run_loop


   /* do it! */
   DO_RLE(rle_lit, 4, l, %eax, $MASK_COLOR_32)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




#endif      /* ifdef ALLEGRO_COLOR32 */

