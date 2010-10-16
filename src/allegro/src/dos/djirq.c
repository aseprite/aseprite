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
 *      Interrupt wrapper functions for djgpp. Unlike the libc
 *      _go32_dpmi_* wrapper functions, these can deal with
 *      reentrant interrupts.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"
#include "../i386/asmdefs.inc"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



#define MAX_IRQS     8           /* timer + keyboard + soundcard + spares */
#define STACK_SIZE   8192        /* an 8k stack should be plenty */

_IRQ_HANDLER _irq_handler[MAX_IRQS];

unsigned char *_irq_stack[IRQ_STACKS];

extern void _irq_wrapper_0(void), _irq_wrapper_1(void), 
	    _irq_wrapper_2(void), _irq_wrapper_3(void),
	    _irq_wrapper_4(void), _irq_wrapper_5(void), 
	    _irq_wrapper_6(void), _irq_wrapper_7(void),
	    _irq_wrapper_0_end(void);



/* _dos_irq_init:
 *  Initialises this module.
 */
void _dos_irq_init(void)
{
   int c;

   LOCK_VARIABLE(_irq_handler);
   LOCK_VARIABLE(_irq_stack);
   LOCK_FUNCTION(_irq_wrapper_0);

   for (c=0; c<MAX_IRQS; c++) {
      _irq_handler[c].handler = NULL;
      _irq_handler[c].number = 0;
   }

   for (c=0; c<IRQ_STACKS; c++) {
      _irq_stack[c] = _AL_MALLOC(STACK_SIZE);
      if (_irq_stack[c]) {
	 LOCK_DATA(_irq_stack[c], STACK_SIZE);
	 _irq_stack[c] += STACK_SIZE - 32;   /* stacks grow downwards */
      }
   }
}



/* _dos_irq_exit:
 *  Routine for freeing the interrupt handler stacks.
 */
void _dos_irq_exit(void)
{
   int c;

   for (c=0; c<IRQ_STACKS; c++) {
      if (_irq_stack[c]) {
	 _irq_stack[c] -= STACK_SIZE - 32;
	 _AL_FREE(_irq_stack[c]);
	 _irq_stack[c] = NULL;
      }
   }
}



/* _install_irq:
 *  Installs a hardware interrupt handler for the specified irq, allocating
 *  an asm wrapper function which will save registers and handle the stack
 *  switching. The C function should return zero to exit the interrupt with 
 *  an iret instruction, and non-zero to chain to the old handler.
 */
int _install_irq(int num, int (*handler)(void))
{
   __dpmi_paddr addr;
   int c;

   for (c=0; c<MAX_IRQS; c++) {
      if (_irq_handler[c].handler == NULL) {

	 addr.selector = _my_cs();

	 switch (c) {
	    case 0: addr.offset32 = (long)_irq_wrapper_0; break;
	    case 1: addr.offset32 = (long)_irq_wrapper_1; break;
	    case 2: addr.offset32 = (long)_irq_wrapper_2; break;
	    case 3: addr.offset32 = (long)_irq_wrapper_3; break;
	    case 4: addr.offset32 = (long)_irq_wrapper_4; break;
	    case 5: addr.offset32 = (long)_irq_wrapper_5; break;
	    case 6: addr.offset32 = (long)_irq_wrapper_6; break;
	    case 7: addr.offset32 = (long)_irq_wrapper_7; break;
	    default: return -1;
	 }

	 _irq_handler[c].handler = handler;
	 _irq_handler[c].number = num;

	 __dpmi_get_protected_mode_interrupt_vector(num, &_irq_handler[c].old_vector);
	 __dpmi_set_protected_mode_interrupt_vector(num, &addr);

	 return 0;
      }
   }

   return -1;
}



/* _remove_irq:
 *  Removes a hardware interrupt handler, restoring the old vector.
 */
void _remove_irq(int num)
{
   int c;

   for (c=0; c<MAX_IRQS; c++) {
      if (_irq_handler[c].number == num) {
	 __dpmi_set_protected_mode_interrupt_vector(num, &_irq_handler[c].old_vector);
	 _irq_handler[c].number = 0;
	 _irq_handler[c].handler = NULL;
	 break;
      }
   }
}


