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
 *      i386 CPU detection routines, by Phil Frisbie.
 *
 *      Theuzifan improved the support for Cyrix chips.
 *
 *      Calin Andrian added 3DNow! detection code.
 *
 *      See readme.txt for copyright information.
 */


#include "asmdefs.inc"

.text



/* int _i_is_486();
 *  Returns TRUE for 486+, and FALSE for 386.
 */
FUNC(_i_is_486)
   pushl %ebp
   movl %esp, %ebp

   pushf                         /* save EFLAGS */
   popl %eax                     /* get EFLAGS */
   movl %eax, %edx               /* temp storage EFLAGS */
   xorl $0x40000, %eax           /* change AC bit in EFLAGS */
   pushl %eax                    /* put new EFLAGS value on stack */
   popf                          /* replace current EFLAGS value */
   pushf                         /* get EFLAGS */
   popl %eax                     /* save new EFLAGS in EAX */
   cmpl %edx, %eax               /* compare temp and new EFLAGS */
   jz is406_not_found

   movl $1, %eax                 /* 80486 present */
   jmp is486_done

is406_not_found:
   movl $0, %eax                 /* 80486 not present */

is486_done:
   pushl %edx                    /* get original EFLAGS */
   popf                          /* restore EFLAGS */

   popl %ebp
   ret




/* int _i_is_fpu();
 *  Returns TRUE is the CPU has floating point hardware.
 */
FUNC(_i_is_fpu)
   pushl %ebp
   movl %esp, %ebp

   fninit
   movl $0x5A5A, %eax
   fnstsw %ax
   cmpl $0, %eax
   jne is_fpu_not_found

   movl $1, %eax
   jmp is_fpu_done

is_fpu_not_found:
   movl $0, %eax

is_fpu_done:
   popl %ebp
   ret




/* int _i_is_cyrix();
 *  Returns TRUE if this is a Cyrix processor.
 */
FUNC(_i_is_cyrix)
   pushl %ebp
   movl %esp, %ebp

   xorw %ax, %ax                 /* clear eax */
   sahf                          /* bit 1 is always 1 in flags */
   movw $5, %ax
   movw $2, %dx
   div %dl                       /* this does not change flags */
   lahf                          /* get flags */
   cmpb $2, %ah                  /* check for change in flags */
   jne is_cyrix_not_found

   movl $1, %eax                 /* TRUE Cyrix CPU */
   jmp is_cyrix_done

is_cyrix_not_found:
   movl $0, %eax                 /* FALSE NON-Cyrix CPU */

is_cyrix_done:
   popl %ebp
   ret




/* void _i_cx_w(int index, int value);
 *  Writes to a Cyrix register.
 */
FUNC(_i_cx_w)
   pushl %ebp
   movl %esp, %ebp
   cli

   movb ARG1, %al
   outb %al, $0x22

   movb ARG2, %al
   outb %al, $0x23

   sti
   popl %ebp
   ret




/* char _i_cx_r(int index);
 *  Reads from a Cyrix register.
 */
FUNC(_i_cx_r)
   pushl %ebp
   movl %esp, %ebp
   cli

   movb ARG1, %al
   outb %al, $0x22

   xorl %eax, %eax
   inb $0x23, %al

   sti
   popl %ebp
   ret




/* int _i_is_cpuid_supported();
 *  Checks whether the cpuid instruction is available.
 */
FUNC(_i_is_cpuid_supported)
   pushl %ebp
   movl %esp, %ebp

   pushfl                        /* get extended flags */
   popl %eax
   movl %eax, %edx               /* save current flags */
   xorl $0x200000, %eax          /* toggle bit 21 */
   pushl %eax                    /* put new flags on stack */
   popfl                         /* flags updated now in flags */
   pushfl                        /* get extended flags */
   popl %eax
   xorl %edx, %eax               /* if bit 21 r/w then supports cpuid */
   jz cpuid_not_found

   movl $1, %eax
   jmp cpuid_done

cpuid_not_found:
   movl $0, %eax

cpuid_done:
   popl %ebp
   ret




/* void _i_get_cpuid_info(uint32_t cpuid_levels, uint32_t *reg);
 *  This is so easy!
 */
FUNC(_i_get_cpuid_info)
   pushl %ebp
   movl %esp, %ebp
   pushl %ebx
   pushl %ecx
   pushl %edi

   movl ARG1, %eax               /* eax = cpuid_levels */

   .byte 0x0F, 0xA2              /* cpuid instruction */

   movl ARG2, %edi               /* edi = reg */

   movl %eax, (%edi)             /* store results */
   movl %ebx, 4(%edi)
   movl %ecx, 8(%edi)
   movl %edx, 12(%edi)

   popl %edi
   popl %ecx
   popl %ebx
   popl %ebp
   ret

