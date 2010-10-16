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
 *
 *      By S. Suzuki,
 *
 *      based code by E.Watanabe (ifsega.c : IF-SEGA/ISA joystick driver).
 *
 *      Test program and informations provided by Saka (sp2p : analog joystick
 *      emulation driver for Win9x DOS Session + IF-SEGA/ISA, IF-SEGA2/PCI).
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* prototype */
int _sg_pci_init(void);
void _sg_pci_exit(void);
unsigned long _sg_pci_ioin(unsigned long offset);
void _sg_pci_ioout(unsigned long offset, unsigned long data);

static int sg_pci_poll(void);
static int sg_pci_poll1(void);
static int sg_pci_poll_sub(int num, int base);
static int sg_pci_poll_sub2(int base);
static int sg_pci_poll_sub3(int num, int count, int base);
static void sg_pci_button_init(int num);

/* grobal */
static unsigned long PowerVR_Physical_Address;
static unsigned long PowerVR_Linear_Address;
static int PowerVR_Linear_Selector;



JOYSTICK_DRIVER joystick_sg2 =
{
   JOY_TYPE_IFSEGA_PCI,
   NULL,
   NULL,
   "IF-SEGA2/PCI",
   _sg_pci_init,
   _sg_pci_exit,
   sg_pci_poll1,
   NULL, NULL,
   NULL, NULL
};



/* _sg_pci_init:
 *  Initialises the driver.
 */
int _sg_pci_init(void)
{
   static char *name_b[] = { "A", "B", "C", "X", "Y", "Z", "L", "R", "Start" };
   int i, j;
   __dpmi_regs r;

   num_joysticks = 4;

   PowerVR_Physical_Address = 0;
   PowerVR_Linear_Address = 0;
   PowerVR_Linear_Selector = 0;

   r.d.eax = 0x0000b101;        /* PCI BIOS - INSTALLATION CHECK */

   r.d.edi = 0x00000000;
   __dpmi_int(0x1a, &r);
   if (r.d.edx != 0x20494350) { /* ' ICP' */
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("PCI BIOS not installed"));
      return -1;
   }

   r.d.eax = 0x0000b102;        /* PCI BIOS - FIND PCI DEVICE */
   r.d.ecx = 0x00000046;        /* device ID */
   r.d.edx = 0x00001033;        /* vendor ID */
   r.d.esi = 0x00000000;        /* device index */

   __dpmi_int(0x1a, &r);
   if (r.h.ah != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("IF-SEGA2/PCI: device not found"));
      return -1;
   }

   r.d.eax = 0x0000b10a;        /* READ CONFIGURATION DWORD */
   /* BH = bus number */
   /* BL = device/function number */

   r.d.edi = 0x00000010;        /* register number */

   __dpmi_int(0x1a, &r);
   if (r.h.ah != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("IF-SEGA2/PCI: read configuration dword error"));
      return -1;
   }
   PowerVR_Physical_Address = r.d.ecx & 0xffff0000;

   if (_create_linear_mapping(&PowerVR_Linear_Address, PowerVR_Physical_Address, 0x10000) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("IF-SEGA2/PCI: _create_linear_mapping error"));
      return -1;
   }

   if (_create_selector(&PowerVR_Linear_Selector, PowerVR_Linear_Address, 0x10000) != 0) {
      _remove_linear_mapping(&PowerVR_Linear_Address);
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("IF-SEGA2/PCI: _create_selector error"));
      return -1;
   }

   for (i = 0; i < num_joysticks; i++) {
      joy[i].flags = JOYFLAG_ANALOGUE;
      joy[i].num_sticks = 2;

      for (j = 0; j < 2; j++) {
	 joy[i].stick[j].flags = JOYFLAG_ANALOGUE | JOYFLAG_SIGNED;
	 joy[i].stick[j].num_axis = 2;
	 joy[i].stick[j].axis[0].name = get_config_text("X");
	 joy[i].stick[j].axis[1].name = get_config_text("Y");
	 joy[i].stick[j].name = get_config_text("Pad");
      }

      joy[i].num_buttons = 9;
      for (j = 0; j < 9; j++) {
	 joy[i].button[j].name = get_config_text(name_b[j]);
      }
   }

   return 0;
}



/* _sg_pci_exit:
 *  Shuts down the driver.
 */
void _sg_pci_exit(void)
{
   _remove_linear_mapping(&PowerVR_Linear_Address);
   _remove_selector(&PowerVR_Linear_Selector);
}



/* sg_pci_poll:
 *  Common - Updates the joystick status variables.
 */
static int sg_pci_poll(void)
{
   int num = 0;

   num = sg_pci_poll_sub(num, 0x000);   /* IFSEGA 1P */

   if (num < num_joysticks)
      num = sg_pci_poll_sub(num, 0x200);        /* IFSEGA 2P */

   while (num < num_joysticks) {
      sg_pci_button_init(num);
      num++;
   }

   return 0;
}



static int sg_pci_poll_sub(int num, int base)
{
   int i;
   int j;
   int a;
   int b;
   int num1;
   int result;

   i = 0x40;
   result = 0;

   _sg_pci_ioout(base, 0x1f);
   do {
      _sg_pci_ioout(base + 0x100, 0x60);
      a = _sg_pci_ioin(base + 0x100);
      if ((a & 0x07) == 0x01 || (a & 0x07) == 0x04)
	 break;
   } while (i--);
   if (a & 0x0c)
      result |= 0x08;
   if (a & 0x03)
      result |= 0x04;

   _sg_pci_ioout(base + 0x100, 0x20);
   a = _sg_pci_ioin(base + 0x100);
   if (a & 0x0c)
      result |= 0x02;
   if (a & 0x03)
      result |= 0x01;

   switch (result) {
      case 0x0b:                /* Normal Pad , Twin Stick , etc.. */
	 /*outportb(base,0x1f); */
	 /*outportb(base+2,0xe0); */
	 /*a = inportb(base+2);                                          // L 1 0 0 */

	 _sg_pci_ioout(base, 0x1f);
	 _sg_pci_ioout(base + 0x100, 0xe0);
	 a = _sg_pci_ioin(base + 0x100);
	 joy[num].button[6].b = !(a & 0x08);

	 /*outportb(base+2,0x80); */
	 /*a = inportb(base+2);                                          // R X Y Z */
	 _sg_pci_ioout(base + 0x100, 0x80);
	 a = _sg_pci_ioin(base + 0x100);
	 joy[num].button[7].b = !(a & 0x08);
	 joy[num].button[3].b = !(a & 0x04);
	 joy[num].button[4].b = !(a & 0x02);
	 joy[num].button[5].b = !(a & 0x01);

	 /*outportb(base+2,0xc0);                                        // Start A C B */
	 /*a = inportb(base+2); */
	 _sg_pci_ioout(base + 0x100, 0xc0);
	 a = _sg_pci_ioin(base + 0x100);
	 joy[num].button[8].b = !(a & 0x08);
	 joy[num].button[0].b = !(a & 0x04);
	 joy[num].button[2].b = !(a & 0x02);
	 joy[num].button[1].b = !(a & 0x01);

	 /*outportb(base+2,0xa0); */
	 /*a = inportb(base+2);                                          // axis R L D U */
	 _sg_pci_ioout(base + 0x100, 0xa0);
	 a = _sg_pci_ioin(base + 0x100);
	 joy[num].stick[0].axis[0].d1 = !(a & 0x04);
	 joy[num].stick[0].axis[0].d2 = !(a & 0x08);
	 joy[num].stick[0].axis[1].d1 = !(a & 0x01);
	 joy[num].stick[0].axis[1].d2 = !(a & 0x02);
	 for (j = 0; j <= 1; j++) {
	    if (joy[num].stick[0].axis[j].d1)
	       joy[num].stick[0].axis[j].pos = -128;
	    else {
	       if (joy[num].stick[0].axis[j].d2)
		  joy[num].stick[0].axis[j].pos = 128;
	       else
		  joy[num].stick[0].axis[j].pos = 0;
	    }
	 }
	 joy[num].stick[1].axis[0].d1 = 0;
	 joy[num].stick[1].axis[0].d2 = 0;
	 joy[num].stick[1].axis[1].d1 = 0;
	 joy[num].stick[1].axis[1].d2 = 0;
	 joy[num].stick[1].axis[0].pos = 0;
	 joy[num].stick[1].axis[1].pos = 0;

	 num++;
	 break;

      case 0x05:
	 switch (sg_pci_poll_sub2(base)) {
	    case 0x00:          /* Multi Controller , Racing Controller , etc.. */

	    case 0x01:
	       sg_pci_poll_sub3(num, sg_pci_poll_sub2(base) * 2, base);
	       num++;
	       break;

	    case 0x04:          /* Multi-Terminal 6 */

	       sg_pci_poll_sub2(base);
	       sg_pci_poll_sub2(base);
	       sg_pci_poll_sub2(base);
	       num1 = num;
	       i = 6 + 1;
	       while (num < num_joysticks) {
		  sg_pci_button_init(num);
		  if (!(--i))
		     break;

		  a = sg_pci_poll_sub2(base);
		  b = sg_pci_poll_sub2(base) * 2;
		  if (a == 0x0f)
		     continue;  /* no pad */

		  if ((a == 0x0e) && (b == 6)) {
		     sg_pci_poll_sub2(base);    /* Shuttle Mouse */

		     sg_pci_poll_sub2(base);
		     sg_pci_poll_sub2(base);
		     sg_pci_poll_sub2(base);
		     sg_pci_poll_sub2(base);
		     sg_pci_poll_sub2(base);
		     continue;
		  }
		  sg_pci_poll_sub3(num, b, base);
		  num++;
	       }
	       if (num == num1)
		  num++;
	       break;

	    default:
	       sg_pci_button_init(num);
	 }
	 break;

      case 0x03:                /* Shuttle Mouse */

      default:
	 sg_pci_button_init(num);
	 num++;
   }

   /*outportb(base+2,0x60); */
   _sg_pci_ioout(base + 0x100, 0x60);
   return num;
}



static int sg_pci_poll_sub2(int base)
{
   int i = 0x100;
   int a;
   int b;

   /*a = inportb(base+2); */
   /*outportb(base+2, a^0x20); */
   a = _sg_pci_ioin(base + 0x100);
   _sg_pci_ioout(base + 0x100, a ^ 0x20);
   do {
      /*b = inportb(base+2); */
      b = _sg_pci_ioin(base + 0x100);
      if ((a & 0x10) != (b & 0x10))
	 break;
   } while (i--);

   return (b & 0x0f);
}



static int sg_pci_poll_sub3(int num, int count, int base)
{
   int i;
   int a;

   a = sg_pci_poll_sub2(base);  /* axis R L D U (digital) */

   joy[num].stick[0].axis[0].d1 = !(a & 0x04);
   joy[num].stick[0].axis[0].d2 = !(a & 0x08);
   joy[num].stick[0].axis[1].d1 = !(a & 0x01);
   joy[num].stick[0].axis[1].d2 = !(a & 0x02);
   a = sg_pci_poll_sub2(base);  /* Start A C B */

   joy[num].button[8].b = !(a & 0x08);
   joy[num].button[0].b = !(a & 0x04);
   joy[num].button[2].b = !(a & 0x02);
   joy[num].button[1].b = !(a & 0x01);
   a = sg_pci_poll_sub2(base);  /* R X T Z */

   joy[num].button[7].b = !(a & 0x08);
   joy[num].button[3].b = !(a & 0x04);
   joy[num].button[4].b = !(a & 0x02);
   joy[num].button[5].b = !(a & 0x01);
   a = sg_pci_poll_sub2(base);  /* L */

   joy[num].button[6].b = !(a & 0x08);

   joy[num].stick[1].axis[0].d1 = 0;
   joy[num].stick[1].axis[0].d2 = 0;
   joy[num].stick[1].axis[1].d1 = 0;
   joy[num].stick[1].axis[1].d2 = 0;
   joy[num].stick[1].axis[0].pos = 0;
   joy[num].stick[1].axis[1].pos = 0;

   if (count -= 4)
      joy[num].stick[0].axis[0].pos = ((sg_pci_poll_sub2(base) << 4) | sg_pci_poll_sub2(base)) - 128;
   else {
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
      return 0;
   }
   if (count -= 2)
      joy[num].stick[0].axis[1].pos = ((sg_pci_poll_sub2(base) << 4) | sg_pci_poll_sub2(base)) - 128;
   else {
      if (joy[num].stick[0].axis[1].d1)
	 joy[num].stick[0].axis[1].pos = -128;
      else {
	 if (joy[num].stick[0].axis[1].d2)
	    joy[num].stick[0].axis[1].pos = 128;
	 else
	    joy[num].stick[0].axis[1].pos = 0;
      }
      return 0;
   }
   if (count -= 2)
      joy[num].stick[1].axis[0].pos = ((sg_pci_poll_sub2(base) << 4) | sg_pci_poll_sub2(base)) - 128;
   else
      return 0;
   if (count -= 2)
      joy[num].stick[1].axis[1].pos = ((sg_pci_poll_sub2(base) << 4) | sg_pci_poll_sub2(base)) - 128;

   return 0;
}



static void sg_pci_button_init(int num)
{
   int i;

   for (i = 0; i < 9; i++) {
      joy[num].button[i].b = 0;
   }
   for (i = 0; i <= 1; i++) {
      joy[num].stick[i].axis[0].d1 = 0;
      joy[num].stick[i].axis[0].d2 = 0;
      joy[num].stick[i].axis[1].d1 = 0;
      joy[num].stick[i].axis[1].d2 = 0;
      joy[num].stick[i].axis[0].pos = 0;
      joy[num].stick[i].axis[1].pos = 0;
   }
}



/* sg_pci_poll1:
 *  1P - Updates the joystick status variables.
 */
static int sg_pci_poll1(void)
{
   return sg_pci_poll();
}



/*
   offset       0x0000
   0x0100
   0x0200
   0x0300
   data 0x00??
 */
unsigned long _sg_pci_ioin(unsigned long offset)
{
   unsigned long in_data;
   unsigned long data;
   unsigned long data2;

   data2 = 0xff00b000 | offset;
   data = 0x00004000 | data2;

   _farsetsel(PowerVR_Linear_Selector);
   _farnspokel(0x0058, data);
   _farnspokel(0x0058, data2);
   in_data = _farnspeekl(0x0058);
   _farnspokel(0x0058, data);

   in_data &= 0xff;

   return (in_data);
}



/*
   offset       0x0000
   0x0100
   0x0200
   0x0300
   data 0x00??
 */
void _sg_pci_ioout(unsigned long offset, unsigned long data)
{
   unsigned long data2;

   data2 = 0xffff7000 | offset | data;
   data = 0x00008000 | data2;

   _farsetsel(PowerVR_Linear_Selector);
   _farnspokel(0x0058, data);
   _farnspokel(0x0058, data2);
   _farnspokel(0x0058, data);

}

