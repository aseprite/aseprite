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
 *      Helper macros for constructing the asm sprite drawing routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_I386_SPRITE_INC
#define ALLEGRO_I386_SPRITE_INC



/* generic framework for constructing sprite drawing routines, shared 
 * between the 8, 16, 24, and 32 bit versions of the code...
 */


#define S_BMP       ARG1
#define S_SPRITE    ARG2
#define S_X         ARG3
#define S_Y         ARG4

#define S_TGAP   -4(%ebp)
#define S_LGAP   -8(%ebp)
#define S_SGAP   -12(%ebp)
#define S_W      -16(%ebp)
#define S_H      -20(%ebp)
#define S_C      -24(%ebp)
#define S_MASK   -28(%ebp)



/* sets up a sprite draw operation and handles the clipping */
#define START_SPRITE_DRAW(name)                                              \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $28, %esp                         /* seven local variables */      ; \
									   ; \
   pushl %edi                                                              ; \
   pushl %esi                                                              ; \
   pushl %ebx                                                              ; \
   pushw %es                                                               ; \
									   ; \
   movl S_BMP, %edx                       /* edx = bitmap pointer */       ; \
   movl S_SPRITE, %esi                    /* esi = sprite pointer */       ; \
									   ; \
   movw BMP_SEG(%edx), %es                /* segment selector */           ; \
									   ; \
   cmpl $0, BMP_CLIP(%edx)                /* test bmp->clip */             ; \
   jz name##_no_clip                                                       ; \
									   ; \
   movl BMP_CT(%edx), %eax                /* bmp->ct */                    ; \
   subl S_Y, %eax                         /* eax -= y */                   ; \
   jge name##_tgap_ok                                                      ; \
   xorl %eax, %eax                                                         ; \
name##_tgap_ok:                                                            ; \
   movl %eax, S_TGAP                      /* set tgap */                   ; \
									   ; \
   movl BMP_H(%esi), %ebx                 /* sprite->h */                  ; \
   movl BMP_CB(%edx), %ecx                /* bmp->cb */                    ; \
   subl S_Y, %ecx                         /* ecx -= y */                   ; \
   cmpl %ebx, %ecx                        /* check bottom clipping */      ; \
   jg name##_height_ok                                                     ; \
   movl %ecx, %ebx                                                         ; \
name##_height_ok:                                                          ; \
   subl %eax, %ebx                        /* height -= tgap */             ; \
   jle name##_done                                                         ; \
   movl %ebx, S_H                         /* set h */                      ; \
									   ; \
   movl BMP_CL(%edx), %eax                /* bmp->cl */                    ; \
   subl S_X, %eax                         /* eax -= x */                   ; \
   jge name##_lgap_ok                                                      ; \
   xorl %eax, %eax                                                         ; \
name##_lgap_ok:                                                            ; \
   movl %eax, S_LGAP                      /* set lgap */                   ; \
									   ; \
   movl BMP_W(%esi), %ebx                 /* sprite->w */                  ; \
   movl BMP_CR(%edx), %ecx                /* bmp->cr */                    ; \
   subl S_X, %ecx                         /* ecx -= x */                   ; \
   cmpl %ebx, %ecx                        /* check left clipping */        ; \
   jg name##_width_ok                                                      ; \
   movl %ecx, %ebx                                                         ; \
name##_width_ok:                                                           ; \
   subl %eax, %ebx                        /* width -= lgap */              ; \
   jle name##_done                                                         ; \
   movl %ebx, S_W                         /* set w */                      ; \
									   ; \
   jmp name##_clip_done                                                    ; \
									   ; \
   _align_                                                                 ; \
name##_no_clip:                                                            ; \
   movl $0, S_TGAP                                                         ; \
   movl $0, S_LGAP                                                         ; \
   movl BMP_W(%esi), %eax                                                  ; \
   movl %eax, S_W                         /* w = sprite->w */              ; \
   movl BMP_H(%esi), %eax                                                  ; \
   movl %eax, S_H                         /* h = sprite->h */              ; \
									   ; \
   _align_                                                                 ; \
name##_clip_done:



/* cleans up the stack after a sprite draw operation */
#define END_SPRITE_DRAW()                                                    \
   popw %es                                                                ; \
									   ; \
   movl S_BMP, %edx                                                        ; \
   UNWRITE_BANK()                                                          ; \
									   ; \
   popl %ebx                                                               ; \
   popl %esi                                                               ; \
   popl %edi                                                               ; \
   movl %ebp, %esp                                                         ; \
   popl %ebp



/* sets up the inner sprite drawing loop, loads registers, etc */
#define SPRITE_LOOP(name)                                                    \
sprite_y_loop_##name:                                                      ; \
   movl S_Y, %eax                         /* load line */                  ; \
   WRITE_BANK()                           /* select bank */                ; \
   addl S_X, %eax                         /* add x offset */               ; \
   movl S_W, %ecx                         /* x loop counter */             ; \
									   ; \
   _align_                                                                 ; \
sprite_x_loop_##name:



/* ends the inner (x) part of a sprite drawing loop */
#define SPRITE_END_X(name)                                                   \
   decl %ecx                                                               ; \
   jg sprite_x_loop_##name



/* ends the outer (y) part of a sprite drawing loop */
#define SPRITE_END_Y(name)                                                   \
   addl S_SGAP, %esi                      /* skip sprite bytes */          ; \
   incl S_Y                               /* next line */                  ; \
   decl S_H                               /* loop counter */               ; \
   jg sprite_y_loop_##name



/* sets up the inner translucent sprite drawing loop, loads registers, etc */
#define T_SPRITE_LOOP(name)                                                  \
sprite_y_loop_##name:                                                      ; \
   movl S_BMP, %edx                       /* load bitmap pointer */        ; \
   movl S_Y, %eax                         /* load line */                  ; \
   READ_BANK()                            /* select read bank */           ; \
   movl %eax, %ecx                        /* read address in ecx */        ; \
   movl S_Y, %eax                         /* reload line */                ; \
   WRITE_BANK()                           /* select write bank */          ; \
   subl %eax, %ecx                        /* convert ecx to offset */      ; \
   addl S_X, %eax                         /* add x offset */               ; \
   movl S_W, %edx                         /* x loop counter */             ; \
   movl %edx, S_C                         /* store */                      ; \
									   ; \
   _align_                                                                 ; \
sprite_x_loop_##name:



/* sets up the inner truecolor translucent sprite drawing loop */
#define TT_SPRITE_LOOP(name, readreg)                                        \
sprite_y_loop_##name:                                                      ; \
   movl S_BMP, %edx                       /* load bitmap pointer */        ; \
   movl S_Y, %eax                         /* load line */                  ; \
   READ_BANK()                            /* select read bank */           ; \
   movl %eax, readreg                     /* read address in readreg */    ; \
   movl S_Y, %eax                         /* reload line */                ; \
   WRITE_BANK()                           /* select write bank */          ; \
   subl %eax, readreg                     /* convert readreg to offset */  ; \
   movl S_W, %edx                         /* x loop counter */             ; \
   addl S_X, %eax                         /* add x offset */               ; \
   movl %edx, S_C                         /* store */                      ; \
   movl %eax, %ebx                        /* move dest address */          ; \
									   ; \
   _align_                                                                 ; \
sprite_x_loop_##name:



/* sets up the inner truecolor lit sprite drawing loop */
#define LT_SPRITE_LOOP(name)                                                 \
sprite_y_loop_##name:                                                      ; \
   movl S_BMP, %edx                       /* load bitmap pointer */        ; \
   movl S_Y, %eax                         /* load line */                  ; \
   WRITE_BANK()                           /* select write bank */          ; \
   movl S_W, %edx                         /* x loop counter */             ; \
   addl S_X, %eax                         /* add x offset */               ; \
   movl %edx, S_C                         /* store */                      ; \
   movl %eax, %ebx                        /* move dest address */          ; \
									   ; \
   _align_                                                                 ; \
sprite_x_loop_##name:



/* ends the inner (x) part of a translucent sprite drawing loop */
#define T_SPRITE_END_X(name)                                                 \
   decl S_C                                                                ; \
   jg sprite_x_loop_##name




/* generic framework for constructing RLE sprite drawing routines, shared 
 * between the 8, 16, 24, and 32 bit versions of the code...
 */


#define R_BMP           ARG1
#define R_SPRITE        ARG2
#define R_X             ARG3
#define R_Y             ARG4
#define R_COLOR         ARG5

#define R_LGAP          -4(%ebp)
#define R_W             -8(%ebp)
#define R_H             -12(%ebp)
#define R_TMP           -16(%ebp)
#define R_TMP2          -20(%ebp)



/* helper macro for drawing RLE sprites */
#define DO_RLE(name, bpp, suf, areg, eolmarker)                              \
   pushl %ebp                                                              ; \
   movl %esp, %ebp                                                         ; \
   subl $20, %esp                                                          ; \
									   ; \
   pushl %ebx                                                              ; \
   pushl %esi                                                              ; \
   pushl %edi                                                              ; \
   pushw %es                                                               ; \
									   ; \
   movl $0, R_LGAP               /* normally zero gap on left */           ; \
   movl R_SPRITE, %esi           /* esi = sprite pointer */                ; \
   movl RLE_W(%esi), %eax        /* read sprite width */                   ; \
   movl %eax, R_W                                                          ; \
   movl RLE_H(%esi), %eax        /* read sprite height */                  ; \
   movl %eax, R_H                                                          ; \
   addl $RLE_DAT, %esi           /* points to start of RLE data */         ; \
									   ; \
   movl R_BMP, %edx              /* edx = bitmap pointer */                ; \
   movw BMP_SEG(%edx), %es       /* select segment */                      ; \
   cld                                                                     ; \
									   ; \
   cmpl $0, BMP_CLIP(%edx)       /* test clip flag */                      ; \
   je name##_noclip                                                        ; \
									   ; \
   movl R_Y, %ecx                /* ecx = Y */                             ; \
									   ; \
name##_clip_top:                                                           ; \
   cmpl %ecx, BMP_CT(%edx)       /* test top clipping */                   ; \
   jle name##_top_ok                                                       ; \
									   ; \
   incl %ecx                     /* increment Y */                         ; \
   decl R_H                      /* decrement height */                    ; \
   jle name##_done                                                         ; \
									   ; \
   _align_                                                                 ; \
name##_clip_top_loop:                                                      ; \
   lods##suf                     /* find zero EOL marker in RLE data */    ; \
   cmp##suf eolmarker, areg                                                ; \
   jne name##_clip_top_loop                                                ; \
									   ; \
   jmp name##_clip_top                                                     ; \
									   ; \
   _align_                                                                 ; \
name##_top_ok:                                                             ; \
   movl %ecx, R_Y                /* store clipped Y */                     ; \
									   ; \
   addl R_H, %ecx                /* ecx = Y + height */                    ; \
   subl BMP_CB(%edx), %ecx       /* test bottom clipping */                ; \
   jl name##_bottom_ok                                                     ; \
									   ; \
   subl %ecx, R_H                /* clip on the bottom */                  ; \
   jle name##_done                                                         ; \
									   ; \
   _align_                                                                 ; \
name##_bottom_ok:                                                          ; \
   movl BMP_CL(%edx), %eax       /* check left clipping */                 ; \
   subl R_X, %eax                                                          ; \
   jle name##_left_ok                                                      ; \
									   ; \
   movl %eax, R_LGAP             /* clip on the left */                    ; \
   addl %eax, R_X                                                          ; \
   subl %eax, R_W                                                          ; \
   jle name##_done                                                         ; \
									   ; \
   _align_                                                                 ; \
name##_left_ok:                                                            ; \
   movl R_X, %eax                /* check right clipping */                ; \
   addl R_W, %eax                                                          ; \
   subl BMP_CR(%edx), %eax                                                 ; \
   jle name##_no_right_clip                                                ; \
									   ; \
   subl %eax, R_W                                                          ; \
   jl name##_done                                                          ; \
   jmp name##_clip_y_loop                                                  ; \
									   ; \
   _align_                                                                 ; \
name##_no_right_clip:                                                      ; \
   cmpl $0, R_LGAP               /* can we use the fast noclip drawer? */  ; \
   je name##_noclip                                                        ; \
									   ; \
									   ; \
   /* slower version of the drawer for sprites that need clipping */       ; \
   _align_                                                                 ; \
name##_clip_y_loop:                                                        ; \
   INIT_RLE_LINE()                                                         ; \
									   ; \
   movl R_W, %ebx                                                          ; \
   movl R_LGAP, %ecx                                                       ; \
									   ; \
name##_clip_lgap_loop:                                                     ; \
   lods##suf                     /* read a command byte */                 ; \
   test##suf areg, areg          /* and test it */                         ; \
   js name##_clip_lgap_zeros                                               ; \
									   ; \
   RLE_ZEX_EAX()                 /* skip a solid run */                    ; \
   leal (%esi, %eax, bpp), %esi                                            ; \
   subl %eax, %ecx                                                         ; \
   jge name##_clip_lgap_loop                                               ; \
									   ; \
   leal (%esi, %ecx, bpp), %esi  /* oops, we overshot */                   ; \
   negl %ecx                                                               ; \
   movl %ecx, %eax                                                         ; \
   jmp name##_clip_x_loop                                                  ; \
									   ; \
   _align_                                                                 ; \
name##_clip_lgap_zeros:                                                    ; \
   RLE_SEX_EAX()                 /* skip a run of zeros */                 ; \
   addl %eax, %ecx                                                         ; \
   jge name##_clip_lgap_loop                                               ; \
									   ; \
   movl %ecx, %eax               /* oops, we overshot */                   ; \
									   ; \
   _align_                                                                 ; \
name##_clip_x_loop:                                                        ; \
   TEST_RLE_COMMAND(name##_clip_x_done, name##_clip_skip_zeros)            ; \
									   ; \
   RLE_ZEX_ECX()                 /* write a string of pixels */            ; \
   subl %ecx, %ebx                                                         ; \
   jle name##_clip_string                                                  ; \
									   ; \
   SLOW_RLE_RUN(0)                                                         ; \
   lods##suf                     /* read next command byte */              ; \
   jmp name##_clip_x_loop                                                  ; \
									   ; \
   _align_                                                                 ; \
name##_clip_string:                                                        ; \
   addl %ebx, %ecx               /* only write part of the string */       ; \
   jle name##_clip_altogether                                              ; \
   SLOW_RLE_RUN(1)                                                         ; \
name##_clip_altogether:                                                    ; \
   negl %ebx                                                               ; \
   leal (%esi, %ebx, bpp), %esi                                            ; \
   jmp name##_clip_skip_rgap                                               ; \
									   ; \
   _align_                                                                 ; \
name##_clip_skip_zeros:                                                    ; \
   RLE_SEX_EAX()                 /* skip over a string of zeros */         ; \
   negl %eax                                                               ; \
   ADD_EAX_EDI()                                                           ; \
   subl %eax, %ebx                                                         ; \
   jle name##_clip_skip_rgap                                               ; \
									   ; \
   lods##suf                     /* read next command byte */              ; \
   jmp name##_clip_x_loop                                                  ; \
									   ; \
   _align_                                                                 ; \
name##_clip_skip_rgap:                                                     ; \
   lods##suf                     /* skip forward to zero EOL marker */     ; \
   cmp##suf eolmarker, areg                                                ; \
   jne name##_clip_skip_rgap                                               ; \
									   ; \
name##_clip_x_done:                                                        ; \
   incl R_Y                                                                ; \
   decl R_H                                                                ; \
   jg name##_clip_y_loop                                                   ; \
   jmp name##_done                                                         ; \
									   ; \
									   ; \
   /* fast drawer for sprites that don't need clipping */                  ; \
   _align_                                                                 ; \
name##_noclip:                                                             ; \
   INIT_FAST_RLE_LOOP()                                                    ; \
									   ; \
   _align_                                                                 ; \
name##_noclip_y_loop:                                                      ; \
   INIT_RLE_LINE()                                                         ; \
									   ; \
   _align_                                                                 ; \
name##_noclip_x_loop:                                                      ; \
   lods##suf                     /* read a command byte */                 ; \
   TEST_RLE_COMMAND(name##_noclip_x_done, name##_noclip_skip_zeros)        ; \
									   ; \
   RLE_ZEX_ECX()                 /* write a string of pixels */            ; \
   FAST_RLE_RUN()                                                          ; \
   jmp name##_noclip_x_loop                                                ; \
									   ; \
   _align_                                                                 ; \
name##_noclip_skip_zeros:                                                  ; \
   neg##suf areg                 /* skip over a string of zeros */         ; \
   RLE_ZEX_EAX()                                                           ; \
   ADD_EAX_EDI()                                                           ; \
   jmp name##_noclip_x_loop                                                ; \
									   ; \
   _align_                                                                 ; \
name##_noclip_x_done:                                                      ; \
   incl R_Y                                                                ; \
   decl R_H                                                                ; \
   jg name##_noclip_y_loop                                                 ; \
									   ; \
									   ; \
name##_done:                                                               ; \
   popw %es                                                                ; \
									   ; \
   movl R_BMP, %edx                                                        ; \
   UNWRITE_BANK()                                                          ; \
									   ; \
   popl %edi                                                               ; \
   popl %esi                                                               ; \
   popl %ebx                                                               ; \
   movl %ebp, %esp                                                         ; \
   popl %ebp                     /* finished drawing an RLE sprite */




#endif          /* ifndef ALLEGRO_I386_SPRITE_INC */

