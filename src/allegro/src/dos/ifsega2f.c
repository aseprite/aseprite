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
 *      Joystick driver for the IF-SEGA2/PCI (I-O DATA) controller.
 *      (Normal PAD * 2 ONLY)
 *
 *      By S. Suzuki,
 *
 *      based on code by E.Watanabe (ifsega.c : IF-SEGA/ISA joystick driver).
 *
 *      Test program and informations provided by Saka (sp2p : analog joystick
 *      emulation driver for Win9x DOS Session + IF-SEGA/ISA, IF-SEGA2/PCI).
 */


#include "allegro.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* prototype */
static int sg_pci_poll_fast(void);
static int sg_pci_poll_sub_fast(int num, int base);


/* extern */
extern int _sg_pci_init(void);
extern void _sg_pci_exit(void);
extern unsigned long _sg_pci_ioin(unsigned long offset);
extern void _sg_pci_ioout(unsigned long offset, unsigned long data);



JOYSTICK_DRIVER joystick_sg2f =
{
   JOY_TYPE_IFSEGA_PCI_FAST,
   NULL,
   NULL,
   "IF-SEGA2/PCI (normal)",
   _sg_pci_init,
   _sg_pci_exit,
   sg_pci_poll_fast,
   NULL, NULL,
   NULL, NULL
};



/* sg_pci_poll:
 *  Common - Updates the joystick status variables.
 */
static int sg_pci_poll_fast(void)
{
   sg_pci_poll_sub_fast(0, 0x000);      /* IFSEGA 1P */
   sg_pci_poll_sub_fast(1, 0x200);      /* IFSEGA 2P */

   return 0;
}



static int sg_pci_poll_sub_fast(int num, int base)
{
   int i;

   _sg_pci_ioout(base, 0x1f);

   _sg_pci_ioout(base + 0x100, 0x80);
   i = _sg_pci_ioin(base + 0x100);      /* R X Y Z */

   joy[num].button[7].b = !(i & 0x08);
   joy[num].button[3].b = !(i & 0x04);
   joy[num].button[4].b = !(i & 0x02);
   joy[num].button[5].b = !(i & 0x01);

   _sg_pci_ioout(base + 0x100, 0xc0);   /* Start A C B */

   i = _sg_pci_ioin(base + 0x100);
   joy[num].button[8].b = !(i & 0x08);
   joy[num].button[0].b = !(i & 0x04);
   joy[num].button[2].b = !(i & 0x02);
   joy[num].button[1].b = !(i & 0x01);

   _sg_pci_ioout(base + 0x100, 0xa0);
   i = _sg_pci_ioin(base + 0x100);      /* axis R L D U */

   joy[num].stick[0].axis[0].d1 = !(i & 0x04);  /* -> */
   joy[num].stick[0].axis[0].d2 = !(i & 0x08);  /* <- */
   joy[num].stick[0].axis[1].d1 = !(i & 0x01);  /*  */
   joy[num].stick[0].axis[1].d2 = !(i & 0x02);  /* ^ */

   for (i = 0; i <= 1; i++) {
      if (joy[num].stick[0].axis[i].d1)
	 joy[num].stick[0].axis[i].pos = -128;
      else {
	 if (joy[num].stick[0].axis[i].d2)
	    joy[num].stick[0].axis[i].pos = 128;
	 else
	    joy[num].stick[0].axis[i].pos = 0;
      }
   }

   _sg_pci_ioout(base + 0x100, 0x60);
   return num;
}
