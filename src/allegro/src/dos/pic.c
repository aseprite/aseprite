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
 *      PIC (interrupt controller) handling routines.
 *
 *      By Ove Kaaven.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



static int pic_virgin = TRUE;

static unsigned char default_pic1;
static unsigned char default_pic2;
static unsigned char altered_pic1;
static unsigned char altered_pic2;



/* exit_irq:
 *  Restores the default hardware interrupt masks.
 */
static void exit_irq(void)
{
   if (!pic_virgin) {
      outportb(0x21, default_pic1);
      outportb(0xA1, default_pic2);
      _remove_exit_func(exit_irq);
      pic_virgin = TRUE;
   }
}



/* init_irq:
 *  Reads the default hardware interrupt masks.
 */
static void init_irq(void)
{
   if (pic_virgin) {
      default_pic1 = inportb(0x21);
      default_pic2 = inportb(0xA1);
      altered_pic1 = 0;
      altered_pic2 = 0;
      _add_exit_func(exit_irq, "exit_irq");
      pic_virgin = FALSE;
   }
}



/* _restore_irq:
 *  Restores default masking for a hardware interrupt.
 */
void _restore_irq(int num)
{
   unsigned char pic;

   if (!pic_virgin) {
      if (num>7) {
	 pic = inportb(0xA1) & (~(1<<(num-8)));
	 outportb(0xA1, pic | (default_pic2 & (1<<(num-8))));
	 altered_pic2 &= ~(1<<(num-8));
	 if (altered_pic2) 
	    return;
	 num = 2; /* if no high IRQs remain, also restore Cascade (IRQ2) */
      }
      pic = inportb(0x21) & (~(1<<num));
      outportb(0x21, pic | (default_pic1 & (1<<num)));
      altered_pic1 &= ~(1<<num);
   }
}



/* _enable_irq:
 *  Unmasks a hardware interrupt.
 */
void _enable_irq(int num)
{
   unsigned char pic;

   init_irq();

   pic = inportb(0x21);

   if (num > 7) {
      outportb(0x21, pic & 0xFB);   /* unmask Cascade (IRQ2) interrupt */
      pic = inportb(0xA1);
      outportb(0xA1, pic & (~(1<<(num-8)))); /* unmask PIC-2 interrupt */
      altered_pic2 |= 1<<(num-8);
   } 
   else {
      outportb(0x21, pic & (~(1<<num)));     /* unmask PIC-1 interrupt */
      altered_pic1 |= 1<<num;
   }
}



/* _disable_irq:
 *  Masks a hardware interrupt.
 */
void _disable_irq(int num)
{
   unsigned char pic;

   init_irq();

   if (num > 7) {
      pic = inportb(0xA1);
      outportb(0xA1, pic & (1<<(num-8))); /* mask PIC-2 interrupt */
      altered_pic2 |= 1<<(num-8);
   } 
   else {
      pic = inportb(0x21);
      outportb(0x21, pic & (1<<num));     /* mask PIC-1 interrupt */
      altered_pic1 |= 1<<num;
   }
}

