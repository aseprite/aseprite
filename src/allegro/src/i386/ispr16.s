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
 *      16 bit sprite drawing (written for speed, not readability :-)
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "sprite.inc"

#ifdef ALLEGRO_COLOR16

.text



/* helper for reading the transparency mask value for a bitmap */
#define STORE_MASK_COLOR(bmp, tmp)                                           \
   movl BMP_VTABLE(bmp), tmp                                               ; \
   movl VTABLE_MASK_COLOR(tmp), tmp                                        ; \
   movl tmp, S_MASK




/* void _linear_draw_sprite16(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite onto a linear bitmap at the specified x, y position, 
 *  using a masked drawing mode where zero pixels are not output.
 */
FUNC(_linear_draw_sprite16)

   /* inner loop that copies just the one word */
   #define LOOP_ONLY_ONE_WORD                                                \
      SPRITE_LOOP(only_one_word)                                           ; \
      movw (%esi), %bx                       /* read a pixel */            ; \
      cmpw S_MASK, %bx                       /* test */                    ; \
      je only_one_word_skip                                                ; \
      movw %bx, %es:(%eax)                   /* write */                   ; \
   only_one_word_skip:                                                     ; \
      addl $2, %esi                                                        ; \
      addl $2, %eax                                                        ; \
      /* no x loop */                                                      ; \
      SPRITE_END_Y(only_one_word)


   /* inner loop that copies a long at a time */
   #define LOOP_LONGS_ONLY                                                   \
      SPRITE_LOOP(longs_only)                                              ; \
      movl (%esi), %ebx                      /* read two pixels */         ; \
      cmpw S_MASK, %bx                       /* test */                    ; \
      jz longs_only_skip_1                                                 ; \
      movw %bx, %es:(%eax)                   /* write */                   ; \
   longs_only_skip_1:                                                      ; \
      shrl $16, %ebx                         /* access next pixel */       ; \
      cmpw S_MASK, %bx                       /* test */                    ; \
      jz longs_only_skip_2                                                 ; \
      movw %bx, %es:2(%eax)                  /* write */                   ; \
   longs_only_skip_2:                                                      ; \
      addl $4, %esi                                                        ; \
      addl $4, %eax                                                        ; \
      SPRITE_END_X(longs_only)                                             ; \
      /* no cleanup at end of line */                                      ; \
      SPRITE_END_Y(longs_only) 


   /* inner loop that copies a long at a time, plus a leftover word */
   #define LOOP_LONGS_AND_WORD                                               \
      SPRITE_LOOP(longs_and_word)                                          ; \
      movl (%esi), %ebx                      /* read two pixels */         ; \
      cmpw S_MASK, %bx                       /* test */                    ; \
      jz longs_and_word_skip_1                                             ; \
      movw %bx, %es:(%eax)                   /* write */                   ; \
   longs_and_word_skip_1:                                                  ; \
      shrl $16, %ebx                         /* access next pixel */       ; \
      cmpw S_MASK, %bx                       /* test */                    ; \
      jz longs_and_word_skip_2                                             ; \
      movw %bx, %es:2(%eax)                  /* write */                   ; \
   longs_and_word_skip_2:                                                  ; \
      addl $4, %esi                                                        ; \
      addl $4, %eax                                                        ; \
      SPRITE_END_X(longs_and_word)           /* end of x loop */           ; \
      movw (%esi), %bx                       /* read pixel */              ; \
      cmpw S_MASK, %bx                       /* test */                    ; \
      jz longs_and_word_end_skip                                           ; \
      movw %bx, %es:(%eax)                   /* write */                   ; \
   longs_and_word_end_skip:                                                ; \
      addl $2, %esi                                                        ; \
      addl $2, %eax                                                        ; \
      SPRITE_END_Y(longs_and_word)


   /* the actual sprite drawing routine... */
   START_SPRITE_DRAW(sprite)
   STORE_MASK_COLOR(%esi, %eax);

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   subl S_W, %eax
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $1, S_X
   shll $1, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   shrl $1, S_W                  /* halve counter for long copies */
   jz sprite_only_one_word
   jnc sprite_even_words

   _align_
   LOOP_LONGS_AND_WORD           /* long at a time, plus leftover word */
   jmp sprite_done

   _align_
sprite_even_words: 
   LOOP_LONGS_ONLY               /* copy a long at a time */
   jmp sprite_done

   _align_
sprite_only_one_word: 
   LOOP_ONLY_ONE_WORD            /* copy just the one word */

   _align_
sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite16() */

FUNC(_linear_draw_sprite16_end)
   ret




/* void _linear_draw_256_sprite16(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a 256 color sprite onto a linear bitmap at the specified x, y 
 *  position, using a masked drawing mode where zero pixels are not output.
 */
FUNC(_linear_draw_256_sprite16)

   START_SPRITE_DRAW(sprite256)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $1, S_X

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   pushl %edx                    /* get color expansion lookup table */
   movl BMP_VTABLE(%edx), %edx
   pushl VTABLE_COLOR_DEPTH(%edx)
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
   movw %bx, %es:(%eax)          /* write */
draw_256_sprite_skip:
   incl %esi
   addl $2, %eax
   SPRITE_END_X(draw_sprite256)
   SPRITE_END_Y(draw_sprite256)

   _align_
sprite256_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_256_sprite16() */




/* void _linear_draw_sprite_v_flip16(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping vertically.
 */
FUNC(_linear_draw_sprite_v_flip16)

   START_SPRITE_DRAW(sprite_v_flip)
   STORE_MASK_COLOR(%esi, %eax);

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   addl S_W, %eax
   addl S_W, %eax                /* + w */
   negl %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $1, S_X
   shll $1, S_LGAP

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   _align_
   SPRITE_LOOP(v_flip) 
   movw (%esi), %bx              /* read pixel */
   cmpw S_MASK, %bx              /* test */
   je sprite_v_flip_skip 
   movw %bx, %es:(%eax)          /* write */
sprite_v_flip_skip: 
   addl $2, %esi 
   addl $2, %eax 
   SPRITE_END_X(v_flip)
   SPRITE_END_Y(v_flip)

sprite_v_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_v_flip16() */




/* void _linear_draw_sprite_h_flip16(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping horizontally.
 */
FUNC(_linear_draw_sprite_h_flip16)

   START_SPRITE_DRAW(sprite_h_flip)
   STORE_MASK_COLOR(%esi, %eax);

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   addl S_W, %eax
   addl S_W, %eax                /* + w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $1, S_X
   shll $1, S_LGAP

   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   leal (%esi, %ecx, 2), %esi
   subl S_LGAP, %esi 
   subl $2, %esi                 /* esi = sprite data ptr */

   _align_
   SPRITE_LOOP(h_flip) 
   movw (%esi), %bx              /* read pixel */
   cmpw S_MASK, %bx              /* test  */
   je sprite_h_flip_skip 
   movw %bx, %es:(%eax)          /* write */
sprite_h_flip_skip: 
   subl $2, %esi 
   addl $2, %eax 
   SPRITE_END_X(h_flip)
   SPRITE_END_Y(h_flip)

sprite_h_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_h_flip16() */




/* void _linear_draw_sprite_vh_flip16(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a sprite to a linear bitmap, flipping both vertically and horizontally.
 */
FUNC(_linear_draw_sprite_vh_flip16)

   START_SPRITE_DRAW(sprite_vh_flip)
   STORE_MASK_COLOR(%esi, %eax);

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   subl S_W, %eax
   subl S_W, %eax                /* - w */
   negl %eax
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $1, S_X
   shll $1, S_LGAP

   negl %eax                     /* - tgap */
   addl BMP_H(%esi), %eax        /* + sprite->h */
   decl %eax
   movl BMP_W(%esi), %ecx 
   movl BMP_LINE(%esi, %eax, 4), %esi
   leal (%esi, %ecx, 2), %esi
   subl S_LGAP, %esi 
   subl $2, %esi                 /* esi = sprite data ptr */

   _align_
   SPRITE_LOOP(vh_flip) 
   movw (%esi), %bx              /* read pixel */
   cmpw S_MASK, %bx              /* test  */
   je sprite_vh_flip_skip 
   movw %bx, %es:(%eax)          /* write */
sprite_vh_flip_skip: 
   subl $2, %esi 
   addl $2, %eax 
   SPRITE_END_X(vh_flip)
   SPRITE_END_Y(vh_flip)

sprite_vh_flip_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_sprite_vh_flip16() */




/* void _linear_draw_trans_sprite16(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a translucent sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_sprite16)

   START_SPRITE_DRAW(trans_sprite)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   subl S_W, %eax
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $1, S_X
   shll $1, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   TT_SPRITE_LOOP(trans, %edi) 
   movw (%esi), %ax              /* read source pixel */
   cmpw $MASK_COLOR_16, %ax
   jz trans_sprite_skip
   movw %es:(%ebx, %edi), %dx    /* read memory pixel */
   pushl GLOBL(_blender_alpha)
   pushl %edx
   pushl %eax
   call *GLOBL(_blender_func16)  /* blend */
   addl $12, %esp
   movw %ax, %es:(%ebx)          /* write the result */
trans_sprite_skip:
   addl $2, %ebx 
   addl $2, %esi 
   T_SPRITE_END_X(trans) 
   /* no cleanup at end of line */ 
   SPRITE_END_Y(trans) 

trans_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_trans_sprite16() */




/* void _linear_draw_trans_rgba_sprite16(BITMAP *bmp, *sprite, int x, y);
 *  Draws a translucent 32 bit sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_rgba_sprite16)

   START_SPRITE_DRAW(trans_rgba_sprite)

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

   shll $1, S_X
   shll $2, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   TT_SPRITE_LOOP(trans_rgba, %edi) 
   movl (%esi), %eax             /* read source pixel */
   cmpl $MASK_COLOR_32, %eax
   jz trans_rgba_sprite_skip
   movw %es:(%ebx, %edi), %dx    /* read memory pixel */
   pushl GLOBL(_blender_alpha)
   pushl %edx
   pushl %eax
   call *GLOBL(_blender_func16x) /* blend */
   addl $12, %esp
   movw %ax, %es:(%ebx)          /* write the result */
trans_rgba_sprite_skip:
   addl $2, %ebx 
   addl $4, %esi 
   T_SPRITE_END_X(trans_rgba) 
   /* no cleanup at end of line */ 
   SPRITE_END_Y(trans_rgba) 

trans_rgba_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end _linear_draw_trans_rgba_sprite16() */




/* void _linear_draw_lit_sprite16(BITMAP *bmp, BITMAP *sprite, int x, y, color);
 *  Draws a lit sprite onto a linear bitmap.
 */
FUNC(_linear_draw_lit_sprite16)

   #define ALPHA     ARG5

   START_SPRITE_DRAW(lit_sprite)

   movl BMP_LINE+4(%esi), %eax
   subl BMP_LINE(%esi), %eax     /* eax = sprite data pitch */
   subl S_W, %eax
   subl S_W, %eax                /* - w */
   movl %eax, S_SGAP             /* store sprite gap */

   movl S_LGAP, %eax
   addl %eax, S_X                /* X += lgap */

   movl S_TGAP, %eax 
   addl %eax, S_Y                /* Y += tgap */

   shll $1, S_X
   shll $1, S_LGAP

   movl BMP_LINE(%esi, %eax, 4), %esi
   addl S_LGAP, %esi             /* esi = sprite data ptr */

   movl ALPHA, %edi

   _align_
   LT_SPRITE_LOOP(lit_sprite) 
   movw (%esi), %ax              /* read pixel */
   cmpw $MASK_COLOR_16, %ax
   jz lit_sprite_skip
   pushl %edi
   pushl %eax
   pushl GLOBL(_blender_col_16)
   call *GLOBL(_blender_func16)  /* blend */
   addl $12, %esp
   movw %ax, %es:(%ebx)          /* write pixel */
lit_sprite_skip:
   addl $2, %esi
   addl $2, %ebx
   T_SPRITE_END_X(lit_sprite)
   SPRITE_END_Y(lit_sprite)

lit_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_lit_sprite16() */




/* void _linear_draw_character16(BITMAP *bmp, BITMAP *sprite, int x, y, color, bg);
 *  For proportional font output onto a linear bitmap: uses the sprite as 
 *  a mask, replacing all set pixels with the specified color.
 */
FUNC(_linear_draw_character16)

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

   shll $1, S_X

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
   movw %bx, %es:(%eax)          /* write pixel */
   jmp draw_opaque_done
draw_opaque_background: 
   movw %di, %es:(%eax)          /* write background */
draw_opaque_done:
   incl %esi 
   addl $2, %eax 
   SPRITE_END_X(draw_opaque_char)
   SPRITE_END_Y(draw_opaque_char)
   jmp draw_char_done

   /* masked (bg -1) character output */
   _align_
draw_masked_char:
   SPRITE_LOOP(draw_masked_char) 
   cmpb $0, (%esi)               /* test pixel */
   jz draw_masked_skip
   movw %bx, %es:(%eax)          /* write pixel */
draw_masked_skip:
   incl %esi 
   addl $2, %eax 
   SPRITE_END_X(draw_masked_char)
   SPRITE_END_Y(draw_masked_char)

draw_char_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_character16() */




/* void _linear_draw_rle_sprite16(BITMAP *bmp, RLE_SPRITE *sprite, int x, y)
 *  Draws an RLE sprite onto a linear bitmap at the specified position.
 */
FUNC(_linear_draw_rle_sprite16)

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 2), %edi


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      rep ; movsw


   /* no special initialisation required */
   #define INIT_FAST_RLE_LOOP()


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      shrl $1, %ecx                                                        ; \
      jnc rle_noclip_no_word                                               ; \
      movsw                      /* copy odd word? */                      ; \
   rle_noclip_no_word:                                                     ; \
      jz rle_noclip_x_loop                                                 ; \
      rep ; movsl                /* 32 bit string copy */


   /* tests an RLE command byte */
   #define TEST_RLE_COMMAND(done, skip)                                      \
      cmpw $MASK_COLOR_16, %ax                                             ; \
      je done                                                              ; \
      testw %ax, %ax                                                       ; \
      js skip


   /* adds the offset in %eax onto the destination address */
   #define ADD_EAX_EDI()                                                     \
      leal (%edi, %eax, 2), %edi


   /* zero extend %ax into %eax */
   #define RLE_ZEX_EAX()                                                     \
      movzwl %ax, %eax


   /* zero extend %ax into %ecx */
   #define RLE_ZEX_ECX()                                                     \
      movzwl %ax, %ecx


   /* sign extend %ax into %eax */
   #define RLE_SEX_EAX()                                                     \
      movswl %ax, %eax


   /* do it! */
   DO_RLE(rle, 2, w, %ax, $MASK_COLOR_16)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_trans_rle_sprite16(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                      int x, int y)
 *  Draws a translucent RLE sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_rle_sprite16)

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      READ_BANK()                   /* select read bank */                 ; \
      movl %eax, R_TMP                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                  /* select write bank */                ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 2), %edi                                           ; \
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
      call *GLOBL(_blender_func16)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      addl $2, %esi                                                        ; \
      addl $2, %edi                                                        ; \
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
      call *GLOBL(_blender_func16)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      addl $2, %esi                                                        ; \
      addl $2, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rle_run_loop


   /* do it! */
   DO_RLE(rle_trans, 2, w, %ax, $MASK_COLOR_16)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_lit_rle_sprite16(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                    int x, int y, int color)
 *  Draws a tinted RLE sprite onto a linear bitmap.
 */
FUNC(_linear_draw_lit_rle_sprite16)

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                                                         ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 2), %edi


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      pushl %ebx                                                           ; \
      movl R_COLOR, %ebx                                                   ; \
      movl %ecx, R_TMP                                                     ; \
									   ; \
   lit_rle_clipped_run_loop##n:                                            ; \
      pushl %ebx                                                           ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      pushl GLOBL(_blender_col_16)                                         ; \
      call *GLOBL(_blender_func16)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      addl $2, %esi                                                        ; \
      addl $2, %edi                                                        ; \
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
      pushl GLOBL(_blender_col_16)                                         ; \
      call *GLOBL(_blender_func16)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      addl $2, %esi                                                        ; \
      addl $2, %edi                                                        ; \
      decl R_TMP                                                           ; \
      jg lit_rle_run_loop


   /* do it! */
   DO_RLE(rle_lit, 2, w, %ax, $MASK_COLOR_16)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN

   #undef TEST_RLE_COMMAND
   #undef RLE_ZEX_EAX
   #undef RLE_ZEX_ECX
   #undef RLE_SEX_EAX




/* void _linear_draw_trans_rgba_rle_sprite16(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                           int x, int y)
 *  Draws a translucent 32 bit RLE sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_rgba_rle_sprite16)

   /* bank switch routine */
   #define INIT_RLE_LINE()                                                   \
      movl R_BMP, %edx                                                     ; \
      movl R_Y, %eax                                                       ; \
      READ_BANK()                   /* select read bank */                 ; \
      movl %eax, R_TMP                                                     ; \
      movl R_Y, %eax                                                       ; \
      WRITE_BANK()                  /* select write bank */                ; \
      movl R_X, %edi                                                       ; \
      leal (%eax, %edi, 2), %edi                                           ; \
      subl %eax, R_TMP              /* calculate read/write diff */


   /* copy a clipped pixel run */
   #define SLOW_RLE_RUN(n)                                                   \
      pushl %ebx                                                           ; \
      movl %ecx, R_TMP2                                                    ; \
									   ; \
   trans_rgba_rle_clipped_run_loop##n:                                     ; \
      movl R_TMP, %edx                                                     ; \
      pushl GLOBL(_blender_alpha)                                          ; \
      pushl %es:(%edi, %edx)        /* read memory pixel */                ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      call *GLOBL(_blender_func16x) /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $2, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rgba_rle_clipped_run_loop##n                                ; \
									   ; \
      popl %ebx


   /* initialise the drawing loop */
   #define INIT_FAST_RLE_LOOP()


   /* copy a run of solid pixels */
   #define FAST_RLE_RUN()                                                    \
      movl %ecx, R_TMP2                                                    ; \
									   ; \
   trans_rgba_rle_run_loop:                                                ; \
      movl R_TMP, %edx                                                     ; \
      pushl GLOBL(_blender_alpha)                                          ; \
      pushl %es:(%edi, %edx)        /* read memory pixel */                ; \
      pushl (%esi)                  /* read sprite pixel */                ; \
      call *GLOBL(_blender_func16x) /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      addl $4, %esi                                                        ; \
      addl $2, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rgba_rle_run_loop


   /* tests an RLE command byte */
   #define TEST_RLE_COMMAND(done, skip)                                      \
      cmpl $MASK_COLOR_32, %eax                                            ; \
      je done                                                              ; \
      testl %eax, %eax                                                     ; \
      js skip


   /* no zero or sign extend required */
   #define RLE_ZEX_EAX()
   #define RLE_SEX_EAX()


   /* this can be a simple copy... */
   #define RLE_ZEX_ECX()                                                     \
      movl %eax, %ecx


   /* do it! */
   DO_RLE(rle_trans_rgba, 4, l, %eax, $MASK_COLOR_32)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




#endif      /* ifdef ALLEGRO_COLOR16 */

