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
 *      VESA bank switching code. These routines will be called with
 *      a line number in %eax and a pointer to the bitmap in %edx. The
 *      bank switcher should select the appropriate bank for the line,
 *      and replace %eax with a pointer to the start of the line.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"

.text




/* Template for bank switchers. Produces a framework that checks which 
 * bank is currently selected and only executes the provided code if it
 * needs to change bank.
 */
#define BANK_SWITCHER(name, cache, code)                                     \
   pushl %eax                                                              ; \
									   ; \
   addl BMP_YOFFSET(%edx), %eax              /* add line offset */         ; \
									   ; \
   shll $2, %eax                                                           ; \
   addl GLOBL(_gfx_bank), %eax               /* lookup which bank */       ; \
   movl (%eax), %eax                                                       ; \
   cmpl cache, %eax                          /* need to change? */         ; \
   je name##_done                                                          ; \
									   ; \
   movl %eax, cache                          /* store the new bank */      ; \
   code                                      /* and change it */           ; \
									   ; \
   _align_                                                                 ; \
name##_done:                                                               ; \
   popl %eax                                                               ; \
   movl BMP_LINE(%edx, %eax, 4), %eax        /* load line address */




/* Uses VESA function 05h (real mode interrupt) to select the bank in %eax.
 */
#define SET_VESA_BANK_RM(window)                                             \
   pushal                                    /* store registers */         ; \
   pushw %es                                                               ; \
									   ; \
   movw GLOBL(__djgpp_ds_alias), %bx                                       ; \
   movw %bx, %es                                                           ; \
									   ; \
   movl $0x10, %ebx                          /* call int 0x10 */           ; \
   movl $0, %ecx                             /* no stack required */       ; \
   movl $GLOBL(_dpmi_reg), %edi              /* register structure */      ; \
									   ; \
   movw $0x4F05, DPMI_AX(%edi)               /* VESA function 05h */       ; \
   movw $window, DPMI_BX(%edi)               /* which window? */           ; \
   movw %ax, DPMI_DX(%edi)                   /* which bank? */             ; \
   movw $0, DPMI_SP(%edi)                    /* zero stack */              ; \
   movw $0, DPMI_SS(%edi)                                                  ; \
   movw $0, DPMI_FLAGS(%edi)                 /* and zero flags */          ; \
									   ; \
   movl $0x0300, %eax                        /* simulate RM interrupt */   ; \
   int $0x31                                                               ; \
									   ; \
   popw %es                                  /* restore registers */       ; \
   popal




/* VESA 1.x window 1 bank switcher */
FUNC(_vesa_window_1)
   BANK_SWITCHER(vesa_window_1, GLOBL(_last_bank_1), SET_VESA_BANK_RM(0))
   ret

FUNC(_vesa_window_1_end)
   ret




/* VESA 1.x window 2 bank switcher */
FUNC(_vesa_window_2)
   BANK_SWITCHER(vesa_window_2, GLOBL(_last_bank_2), SET_VESA_BANK_RM(1))
   addl GLOBL(_window_2_offset), %eax
   ret

FUNC(_vesa_window_2_end)
   ret




/* Uses the VESA 2.0 protected mode interface to select the bank in %eax.
 */
#define SET_VESA_BANK_PM(window)                                             \
   pushal                                    /* store registers */         ; \
									   ; \
   movw $window, %bx                         /* which window? */           ; \
   movw %ax, %dx                             /* which bank? */             ; \
   movl GLOBL(_pm_vesa_switcher), %eax                                     ; \
   call *%eax                                /* do it! */                  ; \
									   ; \
   popal                                     /* restore registers */




/* VESA 2.0 window 1 bank switcher */
FUNC(_vesa_pm_window_1)
   BANK_SWITCHER(vesa_pm_window_1, GLOBL(_last_bank_1), SET_VESA_BANK_PM(0))
   ret

FUNC(_vesa_pm_window_1_end)
   ret




/* VESA 2.0 window 2 bank switcher */
FUNC(_vesa_pm_window_2)
   BANK_SWITCHER(vesa_pm_window_2, GLOBL(_last_bank_2), SET_VESA_BANK_PM(1))
   addl GLOBL(_window_2_offset), %eax
   ret

FUNC(_vesa_pm_window_2_end)
   ret




/* Uses the VESA 2.0 protected mode interface to select the bank in %eax,
 * passing a selector for memory mapped io in %es.
 */
#define SET_VESA_BANK_PM_ES(window)                                          \
   pushal                                    /* store registers */         ; \
   pushw %es                                                               ; \
									   ; \
   movw $window, %bx                         /* which window? */           ; \
   movw %ax, %dx                             /* which bank? */             ; \
   movw GLOBL(_mmio_segment), %ax            /* load selector into %es */  ; \
   movw %ax, %es                                                           ; \
   movl GLOBL(_pm_vesa_switcher), %eax                                     ; \
   call *%eax                                /* do it! */                  ; \
									   ; \
   popw %es                                  /* restore registers */       ; \
   popal




/* VESA 2.0 MMIO mode window 2 bank switcher */
FUNC(_vesa_pm_es_window_1)
   BANK_SWITCHER(vesa_pm_es_window_1, GLOBL(_last_bank_1), SET_VESA_BANK_PM_ES(0))
   ret

FUNC(_vesa_pm_es_window_1_end) 
   ret




/* VESA 2.0 MMIO mode window 2 bank switcher */
FUNC(_vesa_pm_es_window_2)
   BANK_SWITCHER(vesa_pm_es_window_2, GLOBL(_last_bank_2), SET_VESA_BANK_PM_ES(1))
   addl GLOBL(_window_2_offset), %eax
   ret

FUNC(_vesa_pm_es_window_2_end)
   ret

