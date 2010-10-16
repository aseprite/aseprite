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
 *      The djgpp interrupt wrappers used by the stuff in djirq.c.
 *
 *      By Shawn Hargreaves.
 *
 *      Thanks to Marcel de Kogel for identifying the problems Allegro was 
 *      having with reentrant interrupts, and for suggesting this solution.
 *
 *      See readme.txt for copyright information.
 */


#include "../i386/asmdefs.inc"

.text



#define WRAPPER(x)                                                         ; \
FUNC(_irq_wrapper_##x)                                                     ; \
   pushw %ds                              /* save registers */             ; \
   pushw %es                                                               ; \
   pushw %fs                                                               ; \
   pushw %gs                                                               ; \
   pushal                                                                  ; \
									   ; \
   .byte 0x2e                             /* cs: override */               ; \
   movw GLOBL(__djgpp_ds_alias), %ax                                       ; \
   movw %ax, %ds                          /* set up selectors */           ; \
   movw %ax, %es                                                           ; \
   movw %ax, %fs                                                           ; \
   movw %ax, %gs                                                           ; \
									   ; \
   movl $IRQ_STACKS-1, %ecx               /* look for a free stack */      ; \
									   ; \
stack_search_loop_##x:                                                     ; \
   leal GLOBL(_irq_stack)(, %ecx, 4), %ebx                                 ; \
   cmpl $0, (%ebx)                                                         ; \
   jnz found_stack_##x                    /* found one! */                 ; \
									   ; \
   decl %ecx                                                               ; \
   jge stack_search_loop_##x                                               ; \
									   ; \
   jmp get_out_##x                        /* oh shit.. */                  ; \
									   ; \
found_stack_##x:                                                           ; \
   movl %esp, %ecx                        /* old stack in ecx + dx */      ; \
   movw %ss, %dx                                                           ; \
									   ; \
   movl (%ebx), %esp                      /* set up our stack */           ; \
   movw %ax, %ss                                                           ; \
									   ; \
   movl $0, (%ebx)                        /* flag the stack is in use */   ; \
									   ; \
   pushl %edx                             /* push old stack onto new */    ; \
   pushl %ecx                                                              ; \
   pushl %ebx                                                              ; \
									   ; \
   cld                                    /* clear the direction flag */   ; \
									   ; \
   movl GLOBL(_irq_handler) + IRQ_HANDLER + IRQ_SIZE*x, %eax               ; \
   call *%eax                             /* call the C handler */         ; \
									   ; \
   cli                                                                     ; \
									   ; \
   popl %ebx                              /* restore the old stack */      ; \
   popl %ecx                                                               ; \
   popl %edx                                                               ; \
   movl %esp, (%ebx)                                                       ; \
   movw %dx, %ss                                                           ; \
   movl %ecx, %esp                                                         ; \
									   ; \
   orl %eax, %eax                         /* check return value */         ; \
   jz get_out_##x                                                          ; \
									   ; \
   popal                                  /* chain to old handler */       ; \
   popw %gs                                                                ; \
   popw %fs                                                                ; \
   popw %es                                                                ; \
   popw %ds                                                                ; \
   ljmp *%cs:GLOBL(_irq_handler) + IRQ_OLDVEC + IRQ_SIZE*x                 ; \
									   ; \
get_out_##x:                                                               ; \
   popal                                  /* iret */                       ; \
   popw %gs                                                                ; \
   popw %fs                                                                ; \
   popw %es                                                                ; \
   popw %ds                                                                ; \
   sti                                                                     ; \
   iret



WRAPPER(0);
WRAPPER(1);
WRAPPER(2);
WRAPPER(3);
WRAPPER(4);
WRAPPER(5);
WRAPPER(6);
WRAPPER(7);


FUNC(_irq_wrapper_0_end)
   ret

