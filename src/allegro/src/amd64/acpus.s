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
 *      AMD64 CPU detection routines, by Evert Glebbeek.
 *
 *      Original i386 code by Phil Frisbie.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"

.text



/* int _i_is_486();
 *  Returns TRUE for 486+, and FALSE for 386.
 */
FUNC(_i_is_486)
   movl $0, %eax                 /* 80486 not present */
   ret




/* int _i_is_fpu();
 *  Returns TRUE is the CPU has floating point hardware.
 */
FUNC(_i_is_fpu)
   movl $1, %eax
   ret




/* int _i_is_cyrix();
 *  Returns TRUE if this is a Cyrix processor.
 */
FUNC(_i_is_cyrix)
   movl $0, %eax                 /* FALSE NON-Cyrix CPU */
   ret




/* void _i_cx_w(int index, int value);
 *  Writes to a Cyrix register.
 */
FUNC(_i_cx_w)
   ret




/* char _i_cx_r(int index);
 *  Reads from a Cyrix register.
 */
FUNC(_i_cx_r)
   ret




/* int _i_is_cpuid_supported();
 *  Checks whether the cpuid instruction is available.
 */
FUNC(_i_is_cpuid_supported)
   pushq %rbp
   movq %rsp, %rbp

   pushfq                        /* get extended flags */
   popq %rax
   movq %rax, %rdx               /* save current flags */
   xorq $0x200000, %rax          /* toggle bit 21 */
   pushq %rax                    /* put new flags on stack */
   popfq                         /* flags updated now in flags */
   pushfq                        /* get extended flags */
   popq %rax
   xorq %rdx, %rax               /* if bit 21 r/w then supports cpuid */
   jz cpuid_not_found

   movl $1, %eax
   jmp cpuid_done

cpuid_not_found:
   movl $0, %eax

cpuid_done:
   popq %rbp
   ret




/* void _i_get_cpuid_info(int cpuid_levels, int *reg);
 *  This is so easy!
 */
FUNC(_i_get_cpuid_info)
   pushq %rbp
   movq %rsp, %rbp
   pushq %rbx
   pushq %rcx
   pushq %rdi

   /* rdi contains the first argument */
   movl %edi, %eax               /* eax = cpuid_levels */

   .byte 0x0F, 0xA2              /* cpuid instruction */

   /* rsi contains the second argument */
   movl %eax, (%rsi)             /* store results */
   movl %ebx, 4(%rsi)            /* store results */
   movl %ecx, 8(%rsi)            /* store results */
   movl %edx, 12(%rsi)           /* store results */

   popq %rdi
   popq %rcx
   popq %rbx
   popq %rbp
   ret

