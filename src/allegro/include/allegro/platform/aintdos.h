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
 *      Some definitions for internal use by the DOS library code.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef AINTDOS_H
#define AINTDOS_H

#ifndef ALLEGRO_H
   #error must include allegro.h first
#endif

#ifndef ALLEGRO_DOS
   #error bad include
#endif

#ifdef __cplusplus
   extern "C" {
#endif


/* macros to enable and disable interrupts */
#if defined ALLEGRO_GCC

   #define DISABLE()    asm volatile ("cli")
   #define ENABLE()     asm volatile ("sti")

#elif defined ALLEGRO_WATCOM

   void DISABLE(void);
   void ENABLE(void);

   #pragma aux DISABLE  = "cli";
   #pragma aux ENABLE   = "sti";

#else

   #define DISABLE()    asm { cli }
   #define ENABLE()     asm { sti }

#endif


AL_INLINE(void, _enter_critical, (void),
{
   /* check if windows is running */
   if ((os_type == OSTYPE_WIN3)  || (os_type == OSTYPE_WIN95) ||
       (os_type == OSTYPE_WIN98) || (os_type == OSTYPE_WINME) ||
       (os_type == OSTYPE_WINNT) || (os_type == OSTYPE_WIN2000)) {
      __dpmi_regs r;
      r.x.ax = 0x1681; 
      __dpmi_int(0x2F, &r);
   }

   DISABLE();
})


AL_INLINE(void, _exit_critical, (void),
{
   /* check if windows is running */
   if ((os_type == OSTYPE_WIN3)  || (os_type == OSTYPE_WIN95) ||
       (os_type == OSTYPE_WIN98) || (os_type == OSTYPE_WINME) || 
       (os_type == OSTYPE_WINNT) || (os_type == OSTYPE_WIN2000)) {
      __dpmi_regs r;
      r.x.ax = 0x1682; 
      __dpmi_int(0x2F, &r);
   }

   ENABLE();
})


/* interrupt hander stuff */
AL_FUNC(void, _dos_irq_init, (void));
AL_FUNC(void, _dos_irq_exit, (void));

#define _map_irq(irq)   (((irq)>7) ? ((irq)+104) : ((irq)+8))

AL_FUNC(int, _install_irq, (int num, AL_METHOD(int, handler, (void))));
AL_FUNC(void, _remove_irq, (int num));
AL_FUNC(void, _restore_irq, (int irq));
AL_FUNC(void, _enable_irq, (int irq));
AL_FUNC(void, _disable_irq, (int irq));

#define _eoi(irq) { outportb(0x20, 0x20); if ((irq)>7) outportb(0xA0, 0x20); }

typedef struct _IRQ_HANDLER
{
   AL_METHOD(int, handler, (void));    /* our C handler */
   int number;                         /* irq number */

   #ifdef ALLEGRO_DJGPP
      __dpmi_paddr old_vector;         /* original protected mode vector */
   #else
      void (__interrupt __far *old_vector)();
   #endif
} _IRQ_HANDLER;


/* sound lib stuff */
AL_VAR(int, _fm_port);
AL_VAR(int, _mpu_port);
AL_VAR(int, _mpu_irq);


/* DPMI memory mapping routines */
AL_FUNC(int, _create_physical_mapping, (unsigned long *linear, int *segment, unsigned long physaddr, int size));
AL_FUNC(void, _remove_physical_mapping, (unsigned long *linear, int *segment));
AL_FUNC(int, _create_linear_mapping, (unsigned long *linear, unsigned long physaddr, int size));
AL_FUNC(void, _remove_linear_mapping, (unsigned long *linear));
AL_FUNC(int, _create_selector, (int *segment, unsigned long linear, int size));
AL_FUNC(void, _remove_selector, (int *segment));
AL_FUNC(void, _unlock_dpmi_data, (void *addr, int size));


/* bank switching routines (these use a non-C calling convention on i386!) */
AL_FUNC(void, _vesa_window_1, (void));
AL_FUNC(void, _vesa_window_1_end, (void));

AL_FUNC(void, _vesa_window_2, (void));
AL_FUNC(void, _vesa_window_2_end, (void));

AL_FUNC(void, _vesa_pm_window_1, (void));
AL_FUNC(void, _vesa_pm_window_1_end, (void));

AL_FUNC(void, _vesa_pm_window_2, (void));
AL_FUNC(void, _vesa_pm_window_2_end, (void));

AL_FUNC(void, _vesa_pm_es_window_1, (void));
AL_FUNC(void, _vesa_pm_es_window_1_end, (void));

AL_FUNC(void, _vesa_pm_es_window_2, (void));
AL_FUNC(void, _vesa_pm_es_window_2_end, (void));


/* stuff for the VESA driver */
AL_VAR(__dpmi_regs, _dpmi_reg);

AL_VAR(int, _window_2_offset);

AL_VAR(int, _mmio_segment);

AL_FUNCPTR(void, _pm_vesa_switcher, (void));
AL_FUNCPTR(void, _pm_vesa_scroller, (void));
AL_FUNCPTR(void, _pm_vesa_palette, (void));


AL_FUNC(int, _sb_read_dsp_version, (void));
AL_FUNC(int, _sb_reset_dsp, (int data));
AL_FUNC(void, _sb_voice, (int state));
AL_FUNC(int, _sb_set_mixer, (int digi_volume, int midi_volume));

AL_FUNC(void, _mpu_poll, (void));

AL_FUNC(int, _dma_allocate_mem, (int bytes, int *sel, unsigned long *phys));
AL_FUNC(void, _dma_start, (int channel, unsigned long addr, int size, int auto_init, int input));
AL_FUNC(void, _dma_stop, (int channel));
AL_FUNC(unsigned long, _dma_todo, (int channel));
AL_FUNC(void, _dma_lock_mem, (void));



#ifdef __cplusplus
   }
#endif

/* VGA register access helpers */
#include "allegro/internal/aintvga.h"

#endif          /* ifndef AINTDOS_H */
