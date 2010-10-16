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
 *      VBE/AF bank switchers and assorted helper functions.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"

#if (defined ALLEGRO_DOS) || (defined ALLEGRO_LINUX_VBEAF)

.text



#ifdef ALLEGRO_DOS
   #define VBEAF_SELECTOR     GLOBL(__djgpp_ds_alias)
#else
   #define VBEAF_SELECTOR     GLOBL(_vbeaf_selector)
#endif



/* memory locking marker */
FUNC(_af_wrapper)




/* callback for VBE/AF to access real mode interrupts (DPMI 0x300) */
FUNC(_af_int86)

#ifdef ALLEGRO_DOS
   pushal
   pushw %es
   pushw %ds
   popw %es

   xorb %bh, %bh
   xorl %ecx, %ecx

   movl $0x300, %eax
   int $0x31 

   popw %es
   popal
#endif
   ret




/* callback for VBE/AF to access real mode functions (DPMI 0x301) */
FUNC(_af_call_rm)

#ifdef ALLEGRO_DOS
   pushal
   pushw %es
   pushw %ds
   popw %es

   xorb %bh, %bh
   xorl %ecx, %ecx

   movl $0x301, %eax
   int $0x31 

   popw %es
   popal
#endif
   ret




/* memory locking marker */
FUNC(_af_wrapper_end)




/* helper for accelerated driver bank switches, which arbitrates between
 * hardware and cpu control over the video memory.
 */
#define DISABLE_ACCEL(name)                                                  \
   cmpl $0, GLOBL(_accel_active)                                           ; \
   jz name##_inactive                                                      ; \
									   ; \
   pushl %eax                 /* save registers */                         ; \
   pushl %ebx                                                              ; \
   pushl %ecx                                                              ; \
   pushl %edx                                                              ; \
									   ; \
   movw %es, %bx              /* make %es be valid */                      ; \
   movw VBEAF_SELECTOR, %dx                                                ; \
   movw %dx, %es                                                           ; \
									   ; \
   movl GLOBL(_accel_driver), %ecx                                         ; \
   movl GLOBL(_accel_idle), %edx                                           ; \
   pushl %ecx                                                              ; \
   call *%edx                 /* turn off the graphics hardware */         ; \
   addl $4, %esp                                                           ; \
									   ; \
   movw %bx, %es              /* restore registers */                      ; \
									   ; \
   popl %edx                                                               ; \
   popl %ecx                                                               ; \
   popl %ebx                                                               ; \
   popl %eax                                                               ; \
									   ; \
   movl $0, GLOBL(_accel_active)                                           ; \
									   ; \
   _align_                                                                 ; \
name##_inactive:




/* stub for linear accelerated graphics modes */
FUNC(_accel_bank_stub)
   DISABLE_ACCEL(accel_bank_stub)
   movl BMP_LINE(%edx, %eax, 4), %eax
   ret

FUNC(_accel_bank_stub_end)
   ret




/* bank switcher for accelerated graphics modes */
FUNC(_accel_bank_switch)
   DISABLE_ACCEL(accel_bank_switch)
   pushl %eax

   addl BMP_YOFFSET(%edx), %eax              /* lookup which bank */
   shll $2, %eax
   addl GLOBL(_gfx_bank), %eax 
   movl (%eax), %eax

   cmpl GLOBL(_last_bank_1), %eax            /* need to change? */
   je accel_bank_switch_done

   movl %eax, GLOBL(_last_bank_1)            /* store the new bank */

   pushl %ebx                                /* store registers */
   pushl %ecx
   pushl %edx

   movw %es, %bx                             /* make %es be valid */
   movw VBEAF_SELECTOR, %dx
   movw %dx, %es

   movl GLOBL(_accel_driver), %ecx           /* switch banks */
   movl GLOBL(_accel_set_bank), %edx
   pushl %eax
   pushl %ecx
   call *%edx
   addl $8, %esp

   movw %bx, %es                             /* restore registers */

   popl %edx
   popl %ecx
   popl %ebx

   _align_
accel_bank_switch_done:
   popl %eax
   movl BMP_LINE(%edx, %eax, 4), %eax        /* load line address */
   ret

FUNC(_accel_bank_switch_end)
   ret




#endif      /* ifdef VBE/AF is cool on this platform */
