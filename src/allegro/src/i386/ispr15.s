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
 *      15 bit sprite drawing (written for speed, not readability :-)
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"
#include "sprite.inc"

#ifdef ALLEGRO_COLOR16

.text



/* void _linear_draw_trans_sprite15(BITMAP *bmp, BITMAP *sprite, int x, y);
 *  Draws a translucent sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_sprite15)

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
   cmpw $MASK_COLOR_15, %ax
   jz trans_sprite_skip
   movw %es:(%ebx, %edi), %dx    /* read memory pixel */
   pushl GLOBL(_blender_alpha)
   pushl %edx
   pushl %eax
   call *GLOBL(_blender_func15)  /* blend */
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
   ret                           /* end of _linear_draw_trans_sprite15() */




/* void _linear_draw_trans_rgba_sprite15(BITMAP *bmp, sprite, int x, y);
 *  Draws a translucent 32 bit sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_rgba_sprite15)

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
   call *GLOBL(_blender_func15x) /* blend */
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
   ret                           /* end _linear_draw_trans_rgba_sprite15() */




/* void _linear_draw_lit_sprite15(BITMAP *bmp, BITMAP *sprite, int x, y, color);
 *  Draws a lit sprite onto a linear bitmap.
 */
FUNC(_linear_draw_lit_sprite15)

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
   cmpw $MASK_COLOR_15, %ax
   jz lit_sprite_skip
   pushl %edi
   pushl %eax
   pushl GLOBL(_blender_col_15)
   call *GLOBL(_blender_func15)  /* blend */
   addl $12, %esp
   movw %ax, %es:(%ebx)          /* write pixel */
lit_sprite_skip:
   addl $2, %esi
   addl $2, %ebx
   T_SPRITE_END_X(lit_sprite)
   SPRITE_END_Y(lit_sprite)

lit_sprite_done:
   END_SPRITE_DRAW()
   ret                           /* end of _linear_draw_lit_sprite15() */




/* void _linear_draw_rle_sprite15(BITMAP *bmp, RLE_SPRITE *sprite, int x, y)
 *  Draws an RLE sprite onto a linear bitmap at the specified position.
 */
FUNC(_linear_draw_rle_sprite15)

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
      cmpw $MASK_COLOR_15, %ax                                             ; \
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
   DO_RLE(rle, 2, w, %ax, $MASK_COLOR_15)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_trans_rle_sprite15(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                      int x, int y)
 *  Draws a translucent RLE sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_rle_sprite15)

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
      call *GLOBL(_blender_func15)  /* blend */                            ; \
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
      call *GLOBL(_blender_func15)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      addl $2, %esi                                                        ; \
      addl $2, %edi                                                        ; \
      decl R_TMP2                                                          ; \
      jg trans_rle_run_loop


   /* do it! */
   DO_RLE(rle_trans, 2, w, %ax, $MASK_COLOR_15)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN




/* void _linear_draw_lit_rle_sprite15(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                    int x, int y, int color)
 *  Draws a tinted RLE sprite onto a linear bitmap.
 */
FUNC(_linear_draw_lit_rle_sprite15)

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
      pushl GLOBL(_blender_col_15)                                         ; \
      call *GLOBL(_blender_func15)  /* blend */                            ; \
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
      pushl GLOBL(_blender_col_15)                                         ; \
      call *GLOBL(_blender_func15)  /* blend */                            ; \
      addl $12, %esp                                                       ; \
      movw %ax, %es:(%edi)          /* write the pixel */                  ; \
      addl $2, %esi                                                        ; \
      addl $2, %edi                                                        ; \
      decl R_TMP                                                           ; \
      jg lit_rle_run_loop


   /* do it! */
   DO_RLE(rle_lit, 2, w, %ax, $MASK_COLOR_15)
   ret

   #undef INIT_RLE_LINE
   #undef SLOW_RLE_RUN
   #undef INIT_FAST_RLE_LOOP
   #undef FAST_RLE_RUN

   #undef TEST_RLE_COMMAND
   #undef RLE_ZEX_EAX
   #undef RLE_ZEX_ECX
   #undef RLE_SEX_EAX




/* void _linear_draw_trans_rgba_rle_sprite15(BITMAP *bmp, RLE_SPRITE *sprite, 
 *                                           int x, int y)
 *  Draws a translucent 32 bit RLE sprite onto a linear bitmap.
 */
FUNC(_linear_draw_trans_rgba_rle_sprite15)

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
      call *GLOBL(_blender_func15x) /* blend */                            ; \
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
      call *GLOBL(_blender_func15x) /* blend */                            ; \
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

