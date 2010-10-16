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
 *      Helper functions to make things work with Watcom. This file
 *      emulates the DPMI support functions from the djgpp libc, and
 *      provides interrupt wrapper routines using the same API as the
 *      stuff in djirq.c.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef SCAN_DEPEND
   #include <dos.h>
#endif

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"
#include "../i386/asmdefs.inc"



/* some useful globals */
static int out_eax, out_ebx, out_ecx, out_edx, out_esi, out_edi;

#define MAX_IRQS        8
#define STACK_SIZE      8192

static _IRQ_HANDLER irq_handler[MAX_IRQS];

static unsigned char *irq_stack[IRQ_STACKS];

static unsigned char *rmcb_stack;

static void (__interrupt __far *old_int24)() = NULL;

unsigned long __tb = 0;

static int tb_sel = 0;

int __djgpp_ds_alias = 0;



/* my_int24:
 *  Stop those stupid "abort, retry, fail" messages from popping up all
 *  over the place.
 */
#if __WATCOMC__ >= 1200

static void __interrupt __far my_int24(void)
{}

#else

static int __interrupt __far my_int24(void)
{
   return 3;
}

#endif



/* _dos_irq_init:
 *  Initialises this module.
 */
void _dos_irq_init(void)
{
   int c;

   old_int24 = _dos_getvect(0x24);
   _dos_setvect(0x24, my_int24);

   __tb = __dpmi_allocate_dos_memory(16384/16, &tb_sel) * 16;

   __djgpp_ds_alias = _default_ds();

   for (c=0; c<MAX_IRQS; c++) {
      irq_handler[c].handler = NULL;
      irq_handler[c].number = 0;
   }

   for (c=0; c<IRQ_STACKS; c++) {
      irq_stack[c] = _AL_MALLOC(STACK_SIZE);
      if (irq_stack[c]) {
	 LOCK_DATA(irq_stack[c], STACK_SIZE);
	 irq_stack[c] += STACK_SIZE - 32;
      }
   }

   rmcb_stack = _AL_MALLOC(STACK_SIZE);
   if (rmcb_stack) {
      LOCK_DATA(rmcb_stack, STACK_SIZE);
      rmcb_stack += STACK_SIZE - 32;
   }

   LOCK_VARIABLE(out_eax);
   LOCK_VARIABLE(out_ebx);
   LOCK_VARIABLE(out_ecx);
   LOCK_VARIABLE(out_edx);
   LOCK_VARIABLE(out_esi);
   LOCK_VARIABLE(out_edi);
   LOCK_VARIABLE(irq_handler);
   LOCK_VARIABLE(irq_stack);
   LOCK_VARIABLE(rmcb_stack);
   LOCK_VARIABLE(__tb);
   LOCK_VARIABLE(__djgpp_ds_alias);
}



/* _dos_irq_exit:
 *  Shuts down this module.
 */
void _dos_irq_exit(void)
{
   int c;

   for (c=0; c<IRQ_STACKS; c++) {
      if (irq_stack[c]) {
	 irq_stack[c] -= STACK_SIZE - 32;
	 _AL_FREE(irq_stack[c]);
	 irq_stack[c] = NULL;
      }
   }

   if (rmcb_stack) {
      rmcb_stack -= STACK_SIZE - 32;
      _AL_FREE(rmcb_stack);
      rmcb_stack = NULL;
   }

   __dpmi_free_dos_memory(tb_sel);

   tb_sel = 0;
   __tb = 0;

   _dos_setvect(0x24, old_int24);
}



/* wrapper for calling DPMI interrupts */
int DPMI(int ax, int bx, int cx, int dx, int si, int di);

#pragma aux DPMI =                              \
   "  int 0x31 "                                \
   "  jnc dpmi_ok "                             \
   "  mov eax, -1 "                             \
   "  jmp dpmi_done "                           \
   " dpmi_ok: "                                 \
   "  mov eax, 0 "                              \
   " dpmi_done: "                               \
						\
   parm [eax] [ebx] [ecx] [edx] [esi] [edi]     \
   value [eax];



/* wrapper for calling DPMI interrupts with output values */
int DPMI_OUT(int ax, int bx, int cx, int dx, int si, int di);

#pragma aux DPMI_OUT =                          \
   "  int 0x31 "                                \
   "  jnc dpmi_ok "                             \
   "  mov eax, -1 "                             \
   "  jmp dpmi_done "                           \
   " dpmi_ok: "                                 \
   "  mov out_eax, eax "                        \
   "  mov out_ebx, ebx "                        \
   "  mov out_ecx, ecx "                        \
   "  mov out_edx, edx "                        \
   "  mov out_esi, esi "                        \
   "  mov out_edi, edi "                        \
   "  mov eax, 0 "                              \
   " dpmi_done: "                               \
						\
   parm [eax] [ebx] [ecx] [edx] [esi] [edi]     \
   value [eax];



/* __dpmi_int:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_int(int vector, __dpmi_regs *regs)
{
   regs->x.flags = 0;
   regs->x.sp = 0;
   regs->x.ss = 0;

   return DPMI(0x300, vector, 0, 0, 0, (int)regs);
}



/* __dpmi_simulate_real_mode_interrupt:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_simulate_real_mode_interrupt(int vector, __dpmi_regs *regs)
{
   return DPMI(0x300, vector, 0, 0, 0, (int)regs);
}



/* __dpmi_simulate_real_mode_procedure_retf:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_simulate_real_mode_procedure_retf(__dpmi_regs *regs)
{
   return DPMI(0x301, 0, 0, 0, 0, (int)regs);
}



/* __dpmi_allocate_dos_memory:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_allocate_dos_memory(int paragraphs, int *ret)
{
   if (DPMI_OUT(0x100, paragraphs, 0, 0, 0, 0) != 0)
      return -1;

   *ret = (out_edx & 0xFFFF);

   return (out_eax & 0xFFFF);
}



/* __dpmi_free_dos_memory:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_free_dos_memory(int selector)
{
   return DPMI(0x101, 0, 0, selector, 0, 0);
}



/* __dpmi_physical_address_mapping:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_physical_address_mapping(__dpmi_meminfo *info)
{
   if (DPMI_OUT(0x800, (info->address >> 16), (info->address & 0xFFFF), 0, 
		       (info->size >> 16), (info->size & 0xFFFF)) != 0)
      return -1;

   info->address = (out_ebx << 16) | (out_ecx & 0xFFFF);

   return 0;
}



/* __dpmi_free_physical_address_mapping:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_free_physical_address_mapping(__dpmi_meminfo *info)
{
   return DPMI(0x801, (info->address >> 16), (info->address & 0xFFFF), 0, 0, 0);
}



/* __dpmi_lock_linear_region:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_lock_linear_region(__dpmi_meminfo *info)
{
   return DPMI(0x600, (info->address >> 16), (info->address & 0xFFFF), 0, 
		      (info->size >> 16), (info->size & 0xFFFF));
}



/* __dpmi_unlock_linear_region:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_unlock_linear_region(__dpmi_meminfo *info)
{
   return DPMI(0x601, (info->address >> 16), (info->address & 0xFFFF), 0, 
		      (info->size >> 16), (info->size & 0xFFFF));
}



/* _go32_dpmi_lock_data:
 *  Watcom implementation of the djgpp library routine.
 */
int _go32_dpmi_lock_data(void *lockaddr, unsigned long locksize)
{
   unsigned long baseaddr;
   __dpmi_meminfo meminfo;

   if (__dpmi_get_segment_base_address(_default_ds(), &baseaddr) != 0)
      return -1;

   meminfo.handle = 0;
   meminfo.size = locksize;
   meminfo.address = baseaddr + (unsigned long)lockaddr;

   return __dpmi_lock_linear_region(&meminfo);
}



/* _go32_dpmi_lock_code:
 *  Watcom implementation of the djgpp library routine.
 */
int _go32_dpmi_lock_code(void *lockaddr, unsigned long locksize)
{
   unsigned long baseaddr;
   __dpmi_meminfo meminfo;

   if (__dpmi_get_segment_base_address(_my_cs(), &baseaddr) != 0)
      return -1;

   meminfo.handle = 0;
   meminfo.size = locksize;
   meminfo.address = baseaddr + (unsigned long)lockaddr;

   return __dpmi_lock_linear_region(&meminfo);
}



/* __dpmi_allocate_ldt_descriptors:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_allocate_ldt_descriptors(int count)
{
   if (DPMI_OUT(0x000, 0, count, 0, 0, 0) != 0)
      return -1;

   return (out_eax & 0xFFFF);
}



/* __dpmi_free_ldt_descriptor:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_free_ldt_descriptor(int descriptor)
{
   return DPMI(0x001, descriptor, 0, 0, 0, 0);
}



/* __dpmi_get_segment_base_address:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_get_segment_base_address(int selector, unsigned long *addr)
{
   if (DPMI_OUT(0x006, selector, 0, 0, 0, 0) != 0)
      return -1;

   *addr = (out_ecx << 16) | (out_edx & 0xFFFF);
   return 0;
}



/* __dpmi_set_segment_base_address:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_set_segment_base_address(int selector, unsigned long address)
{
   return DPMI(0x007, selector, (address >> 16), (address & 0xFFFF), 0, 0);
}



/* __dpmi_set_segment_limit:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_set_segment_limit(int selector, unsigned long limit)
{
   return DPMI(0x008, selector, (limit >> 16), (limit & 0xFFFF), 0, 0);
}



/* __dpmi_get_free_memory_information:
 *  Watcom implementation of the djgpp library routine.
 */
int __dpmi_get_free_memory_information(__dpmi_free_mem_info *info)
{
   return DPMI(0x500, 0, 0, 0, 0, (int)info);
}



/* switches to a custom stack when running interrupt code */
int CALL_HANDLER(int (*func)(void), int max_stack);

#pragma aux CALL_HANDLER =                                                 \
   "  push ds "                                                            \
   "  pop es "                                                             \
									   \
   " stack_search_loop: "              /* look for a free stack */         \
   "  lea ebx, irq_stack[ecx*4] "                                          \
   "  cmp [ebx], 0 "                                                       \
   "  jnz found_stack "                /* found one! */                    \
									   \
   "  dec ecx "                                                            \
   "  jge stack_search_loop "                                              \
									   \
   "  mov eax, 0 "                     /* oh shit.. */                     \
   "  jmp get_out "                                                        \
									   \
   " found_stack: "                                                        \
   "  mov ecx, esp "                   /* old stack in ecx + dx */         \
   "  mov dx, ss "                                                         \
									   \
   "  mov ax, ds "                     /* set up our stack */              \
   "  mov ss, ax "                                                         \
   "  mov esp, [ebx] "                                                     \
									   \
   "  mov dword ptr [ebx], 0 "         /* flag the stack is in use */      \
									   \
   "  push edx "                       /* push old stack onto new */       \
   "  push ecx "                                                           \
   "  push ebx "                                                           \
									   \
   "  cld "                            /* call the handler */              \
   "  cli "                                                                \
   "  call esi "                                                           \
									   \
   "  pop ebx "                        /* restore the old stack */         \
   "  pop ecx "                                                            \
   "  pop edx "                                                            \
   "  mov [ebx], esp "                                                     \
   "  mov ss, dx "                                                         \
   "  mov esp, ecx "                                                       \
   "  get_out:     "                                                       \
									   \
   parm [esi] [ecx]                                                        \
   modify [ebx edx edi]                                                    \
   value [eax];



/* interrupt wrapper */
#define WRAPPER(num)                                                       \
									   \
   static void __interrupt __far irq_wrapper_##num()                       \
   {                                                                       \
      if (CALL_HANDLER(irq_handler[num].handler, IRQ_STACKS-1))            \
	 _chain_intr(irq_handler[num].old_vector);                         \
   }


WRAPPER(0);
WRAPPER(1);
WRAPPER(2);
WRAPPER(3);
WRAPPER(4);
WRAPPER(5);
WRAPPER(6);
WRAPPER(7);

END_OF_STATIC_FUNCTION(irq_wrapper_0);



/* _install_irq:
 *  Installs a custom IRQ handler.
 */
int _install_irq(int num, int (*handler)(void))
{
   int c;

   LOCK_FUNCTION(irq_wrapper_0);

   for (c=0; c<MAX_IRQS; c++) {
      if (!irq_handler[c].handler) {
	 irq_handler[c].handler = handler;
	 irq_handler[c].number = num;
	 irq_handler[c].old_vector = _dos_getvect(num);

	 switch (c) {
	    case 0: _dos_setvect(num, irq_wrapper_0); break;
	    case 1: _dos_setvect(num, irq_wrapper_1); break;
	    case 2: _dos_setvect(num, irq_wrapper_2); break;
	    case 3: _dos_setvect(num, irq_wrapper_3); break;
	    case 4: _dos_setvect(num, irq_wrapper_4); break;
	    case 5: _dos_setvect(num, irq_wrapper_5); break;
	    case 6: _dos_setvect(num, irq_wrapper_6); break;
	    case 7: _dos_setvect(num, irq_wrapper_7); break;
	 }

	 return 0;
      }
   }

   return -1;
}



/* _remove_irq:
 *  Returns to the default IRQ handler.
 */
void _remove_irq(int num)
{
   int c;

   for (c=0; c<MAX_IRQS; c++) {
      if (irq_handler[c].number == num) {
	 _dos_setvect(num, irq_handler[c].old_vector);
	 irq_handler[c].number = 0;
	 irq_handler[c].handler = NULL;
	 break;
      }
   }
}



/* real mode callback for the mouse driver */
static void (*rmcb_handler)(__dpmi_regs *) = NULL;

static __dpmi_regs *rmcb_regs = NULL;

static short rmcb_saved_ss = 0;
static int rmcb_saved_esp = 0;



/* has to be done in two parts because Watcom can't handle the pragma size */
void CALL_RMCB_PART1(void);
void CALL_RMCB_PART2(void);



#pragma aux CALL_RMCB_PART1 =                                              \
   "  push eax "                       /* save registers */                \
   "  push ecx "                                                           \
   "  push edx "                                                           \
   "  push ds "                                                            \
   "  push es "                                                            \
									   \
   "  mov eax, cs:__djgpp_ds_alias "   /* set up our data selector */      \
   "  mov ds, ax "                                                         \
   "  mov es, ax "                                                         \
									   \
   "  mov ax, ss "                     /* switch to our own stack */       \
   "  mov rmcb_saved_ss, ax "                                              \
   "  mov rmcb_saved_esp, esp "                                            \
   "  mov ax, ds "                                                         \
   "  mov ss, ax "                                                         \
   "  mov esp, rmcb_stack "



#pragma aux CALL_RMCB_PART2 =                                              \
   "  mov eax, rmcb_regs "             /* fill register structure */       \
   "  mov 0[eax], edi"                                                     \
   "  mov 4[eax], esi"                                                     \
   "  mov 16[eax], ebx"                                                    \
   "  mov 20[eax], edx"                                                    \
   "  mov 24[eax], ecx "                                                   \
									   \
   "  cld "                            /* call the handler */              \
   "  mov esi, rmcb_handler "                                              \
   "  push eax "                                                           \
   "  call esi "                                                           \
									   \
   "  mov esp, rmcb_saved_esp "        /* restore the original stack */    \
   "  mov ax, rmcb_saved_ss "                                              \
   "  mov ss, ax "                                                         \
									   \
   "  pop es "                         /* restore registers */             \
   "  pop ds "                                                             \
   "  pop edx "                                                            \
   "  pop ecx "                                                            \
   "  pop eax "



static void far rmcb_callback(void)
{
   CALL_RMCB_PART1();
   CALL_RMCB_PART2();
}

END_OF_STATIC_FUNCTION(rmcb_callback);



/* _allocate_real_mode_callback:
 *  Sets up a realmode callback for the mouse driver.
 */
long _allocate_real_mode_callback(void (*handler)(__dpmi_regs *), __dpmi_regs *regs)
{
   LOCK_VARIABLE(rmcb_handler);
   LOCK_VARIABLE(rmcb_regs);
   LOCK_VARIABLE(rmcb_saved_ss);
   LOCK_VARIABLE(rmcb_saved_esp);
   LOCK_FUNCTION(rmcb_callback);

   rmcb_handler = handler;
   rmcb_regs = regs;

   return (long)rmcb_callback;
}

