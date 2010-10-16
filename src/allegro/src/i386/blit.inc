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
 *      Helper macros for constructing the asm bitmap blitting routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_I386_BLIT_INC
#define ALLEGRO_I386_BLIT_INC



/* generic framework for constructing blitting routines, shared 
 * between the 8, 16, 24, and 32 bit versions of the code.
 */


#define B_SOURCE        ARG1 
#define B_DEST          ARG2
#define B_SOURCE_X      ARG3
#define B_SOURCE_Y      ARG4
#define B_DEST_X        ARG5
#define B_DEST_Y        ARG6
#define B_WIDTH         ARG7
#define B_HEIGHT        ARG8



#define BLIT_LOOP(name, bpp, code)                                           \
blit_loop_##name:                                                          ; \
   movl B_DEST, %edx             /* destination bitmap */                  ; \
   movl B_DEST_Y, %eax           /* line number */                         ; \
   movl B_DEST_X, %edi           /* x offset */                            ; \
   WRITE_BANK()                  /* select bank */                         ; \
   leal (%eax, %edi, bpp), %edi                                            ; \
									   ; \
   movl B_SOURCE, %edx           /* source bitmap */                       ; \
   movl B_SOURCE_Y, %eax         /* line number */                         ; \
   movl B_SOURCE_X, %esi         /* x offset */                            ; \
   READ_BANK()                   /* select bank */                         ; \
   leal (%eax, %esi, bpp), %esi                                            ; \
									   ; \
   movl B_WIDTH, %ecx            /* x loop counter */                      ; \
   movl BMP_SEG(%edx), %eax      /* load data segment */                   ; \
   movl %eax, %ds                                                          ; \
   code                          /* do the transfer */                     ; \
									   ; \
   movl %ebx, %ds                /* restore data segment */                ; \
   incl B_SOURCE_Y                                                         ; \
   incl B_DEST_Y                                                           ; \
   decl B_HEIGHT                                                           ; \
   jg blit_loop_##name           /* and loop */




#endif          /* ifndef ALLEGRO_I386_BLIT_INC */

