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
 *      VGA helper functions for Linux Allegro.
 *
 *      Originally by Marek Habersack, mangled by George Foot.
 *
 *      See readme.txt for copyright information.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "allegro.h"

#ifdef ALLEGRO_LINUX_VGA

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "allegro/internal/aintvga.h"
#include "linalleg.h"

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/mman.h>


/* Used by the text font routines, to access the video memory */
static struct MAPPED_MEMORY vram = {
	VGA_MEMORY_BASE, VGA_MEMORY_SIZE,
	PROT_READ | PROT_WRITE,
	NULL
};


int __al_linux_init_vga_helpers (void)
{
   __al_linux_map_memory (&vram);
   _vga_regs_init();
   return 0;
}

int __al_linux_shutdown_vga_helpers (void)
{
   __al_linux_unmap_memory (&vram);
   return 0;
}

/* __al_linux_set_vga_regs:
 *  Write the standard VGA registers using values from the passed structure.
 */
void __al_linux_set_vga_regs(MODE_REGISTERS *regs)
{
   int i = 0;

   if (!regs)
      return;
   
   outportb(MISC_REG_W, regs->misc);

   outportw(SEQ_REG_I, 0x0100);
   for (i = 1; i < N_SEQ_REGS; i++)
      outportw(SEQ_REG_I, (regs->seq[i] << 8) | i);
   outportw(SEQ_REG_I, 0x0300);

   outportw(_crtc, ((_read_vga_register(_crtc, 0x11) & 0x7F) << 8) | 0x11);
   for (i = 0; i < N_CRTC_REGS; i++)
      outportw(_crtc, (regs->crt[i] << 8) | i);

   for (i = 0; i < N_GC_REGS; i++)
      outportw(GC_REG_I, (regs->gc[i] << 8) | i);

   for (i = 0; i < N_ATC_REGS; i++) {
      inportb(_is1);
      outportb(ATC_REG_IW, i);
      outportb(ATC_REG_IW, regs->atc[i]);
      __just_a_moment();
   }
}

/* __al_linux_get_vga_regs:
 *  Read and save in the passed array all standard VGA regs for the current
 *  video mode.
 */
void __al_linux_get_vga_regs(MODE_REGISTERS *regs)
{
   int i = 0;

   if (!regs)
      return;
   
   regs->misc = inportb(MISC_REG_R);
   
   for (i = 0; i < N_CRTC_REGS; i++) {
      outportb(_crtc, i);
      regs->crt[i] = inportb(_crtc+1);
   }

   for (i = 0; i < N_GC_REGS; i++) {
      outportb(GC_REG_I, i);
      regs->gc[i] = inportb(GC_REG_RW);
   }

   for (i = 0; i < N_SEQ_REGS; i++) {
      outportb(SEQ_REG_I, i);
      regs->seq[i] = inportb(SEQ_REG_RW);
   }

   for (i = 0; i < N_ATC_REGS; i++) {
      inportb(_is1);
      outportb(ATC_REG_IW, i);
      regs->atc[i] = inportb(ATC_REG_R);
      __just_a_moment();
   }
}


static void __al_linux_save_palette (MODE_REGISTERS *regs)
{
   int c;
   unsigned char *ptr;
   ASSERT(regs);

   ptr = regs->palette.vga;

   for (c = 0; c < 256; c++) {
      outportb (0x3c7, c);
      *ptr++ = inportb (0x3c9);
      *ptr++ = inportb (0x3c9);
      *ptr++ = inportb (0x3c9);
   }
}

static void __al_linux_restore_palette (MODE_REGISTERS *regs)
{
   int c;
   unsigned char *ptr;
   ASSERT(regs);

   ptr = regs->palette.vga;

   for (c = 0; c < 256; c++) {
      outportb (0x3c8, c);
      outportb (0x3c9, *ptr++);
      outportb (0x3c9, *ptr++);
      outportb (0x3c9, *ptr++);
   }
}


static inline void slow_byte_copy(char *from, char *to, unsigned count)
{
   unsigned int i;
   ASSERT(from && to);

   for (i = 0; i < count; i++) {
      *to++ = *from++;
      outportb(0x80, 0x00);  /* 1ms delay */
   }
}

/* __al_linux_save_text_font:
 *  Copy the text font data from the video memory.  Since some cards won't work
 *  properly unless we save/restore fonts from both 2nd and 3rd plane, we save
 *  them both.
 */
static void __al_linux_save_text_font (MODE_REGISTERS *regs)
{
   ASSERT(regs);

   if (!regs->text_font1)
      regs->text_font1 = (unsigned char*) _AL_MALLOC (VGA_FONT_SIZE);
   if (!regs->text_font2)
      regs->text_font2 = (unsigned char*) _AL_MALLOC (VGA_FONT_SIZE);

   /* First switch to a 4bpp video mode, so that we have selective 
    * access to video memory planes.  Do it with host palette 
    * access disabled. */
   inportb(_is1);
   outportb(ATC_REG_IW, 0x30);
   outportb(ATC_REG_IW, 0x01);   /* Put ATC into graphics mode */
   outportw(SEQ_REG_I, 0x0604);  /* Planar mode */
   outportw(GC_REG_I, 0x0005);   /* Write Mode == Read Mode == 0 */
   outportw(GC_REG_I, 0x0506);   /* Switch GC to graphics mode and start VGA
                                  * memory at 0xA0000 (64KB) */
   
   /* Now we're ready to read the first font.  We'll do it using the slowest
    * copy routine possible.  It is to account for the old VGA boards which may
    * lose data during too fast reads/writes.
    */
   outportw(GC_REG_I, 0x0204); /* Reading from plane 2 */
   slow_byte_copy ((char*)vram.data, (char*)regs->text_font1, VGA_FONT_SIZE);
   outportw(GC_REG_I, 0x0304); /* Reading from plane 3 */
   slow_byte_copy ((char*)vram.data, (char*)regs->text_font2, VGA_FONT_SIZE);
}

/* __al_linux_restore_text_font:
 *  Copy the saved font data back to video memory.
 */
static void __al_linux_restore_text_font(MODE_REGISTERS *regs)
{
   ASSERT(regs);

   /* Same as in the previous routing.  We're restoring the font in a 
    * 4bpp mode to have access to separate memory planes. */
   inportb(_is1);
   outportb(ATC_REG_IW, 0x30);
   outportb(ATC_REG_IW, 0x01);   /* Put ATC into graphics mode */
   outportw(SEQ_REG_I, 0x0604);  /* Planar mode */
   outportw(GC_REG_I, 0x0005);   /* Write Mode == Read Mode == 0 */
   outportw(GC_REG_I, 0x0506);   /* Switch GC to graphics mode and start VGA
                                  * memory at 0xA0000 (64KB) */
   outportw(GC_REG_I, 0x0001);

   if (regs->text_font1) {
      outportw(SEQ_REG_I, 0x0402);
      slow_byte_copy((char*)regs->text_font1, (char*)vram.data, VGA_FONT_SIZE);
   }

   if (regs->text_font2) {
      outportw(SEQ_REG_I, 0x0802);
      slow_byte_copy((char*)regs->text_font2, (char*)vram.data, VGA_FONT_SIZE);
   }
}


void __al_linux_screen_off(void)
{
   _write_vga_register(SEQ_REG_I, 0x01,
                       _read_vga_register(SEQ_REG_I, 0x01) | 0x20);
   inportb(_is1);
   __just_a_moment();
   outportb(ATC_REG_IW, 0x00);
   __just_a_moment();
}

void __al_linux_screen_on(void)
{
   _write_vga_register(SEQ_REG_I, 0x01,
                       _read_vga_register(SEQ_REG_I, 0x01) & 0xDF);
   inportb(_is1);
   __just_a_moment();
   outportb(ATC_REG_IW, 0x20);
   __just_a_moment();
}

void __al_linux_clear_vram (void)
{
   memset (vram.data, 0, vram.size);
}


static MODE_REGISTERS txt_regs;

void __al_linux_save_text_mode (void)
{
   __al_linux_get_vga_regs (&txt_regs);
   __al_linux_save_palette (&txt_regs);
   __al_linux_save_text_font (&txt_regs);
}

void __al_linux_restore_text_mode (void)
{
   __al_linux_restore_text_font (&txt_regs);
   __al_linux_restore_palette (&txt_regs);
   __al_linux_set_vga_regs (&txt_regs);
}

#endif /* ALLEGRO_LINUX_VGA */
