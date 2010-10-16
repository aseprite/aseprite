/************************************************************************\
**                                                                      **
**                GrIP Prototype API Interface Library                  **
**                                for                                   **
**                               DJGPP                                  **
**                                                                      **
**                           Revision 1.00                              **
**                                                                      **
**  COPYRIGHT:                                                          **
**                                                                      **
**    (C) Copyright Advanced Gravis Computer Technology Ltd 1995.       **
**        All Rights Reserved.                                          **
**                                                                      **
**  DISCLAIMER OF WARRANTIES:                                           **
**                                                                      **
**    The following [enclosed] code is provided to you "AS IS",         **
**    without warranty of any kind.  You have a royalty-free right to   **
**    use, modify, reproduce and distribute the following code (and/or  **
**    any modified version) provided that you agree that Advanced       **
**    Gravis has no warranty obligations and shall not be liable for    **
**    any damages arising out of your use of this code, even if they    **
**    have been advised of the possibility of such damages.  This       **
**    Copyright statement and Disclaimer of Warranties may not be       **
**    removed.                                                          **
**                                                                      **
**  HISTORY:                                                            **
**                                                                      **
**    0.102   Jul 12 95   David Bollo     Initial public release on     **
**                                          GrIP hardware               **
**    0.200   Aug 10 95   David Bollo     Added Gravis Loadable Library **
**                                          support                     **
**    0.201   Aug 11 95   David Bollo     Removed Borland C++ support   **
**                                          for maintenance reasons     **
**    1.00    Nov  1 95   David Bollo     First official release as     **
**                                          part of GrIP SDK            **
**                                                                      **
\************************************************************************/

#include <string.h>

#include "allegro.h"
#include "grip.h"

/* Global Thunking Data */
GRIP_THUNK GRIP_Thunk;
static int GRIP_Thunked = 0;
static int GRIP_CS;
static int GRIP_DS;
#if defined(GRIP_DEBUG)
static short GRIP_ES;
#endif

#ifndef ALLEGRO_DJGPP
#define _farpokew(ds, addr, value)  *(short *)(addr) = (value)
#endif

#define DPMI_AllocDOS(a, b) (__dpmi_allocate_dos_memory(a, b) << 4)
#define DPMI_FreeDOS(a) __dpmi_free_dos_memory(a)
#define DPMI_AllocSel() __dpmi_allocate_ldt_descriptors(1)
#define DPMI_FreeSel(a) __dpmi_free_ldt_descriptor(a)

   unsigned char _DPMI_SetCodeAR(unsigned short sel);
   unsigned char _DPMI_SetDataAR(unsigned short sel);

   static unsigned char DPMI_SetBounds(unsigned short sel, unsigned long base, unsigned short lm) {
      if (__dpmi_set_segment_base_address(sel, base) == -1)
	 return -1;
      if (__dpmi_set_segment_limit(sel, lm - 1) == -1)
	 return -1;
      return 0;
   } 

   GRIP_BOOL _GrLink(GRIP_BUF image, GRIP_VALUE size) {
      unsigned short img_off;   /* Offset of binary image     */
      unsigned short img_size;  /* Size of binary image       */
      unsigned char img_check;  /* Checksum of binary image   */
      unsigned long memory;     /* Image memory               */
      GRIP_BUF_C header;        /* Image header               */
      GRIP_BUF_C core;          /* Image core                 */
      int i;

      /* Check for duplicate GrLink calls */
      if (GRIP_Thunked)
	  return 0;

      /* Sanity check the inputs */
      if (image == 0 || size < 16)
	  return 0;

      /* Check the image header for the proper signature bytes */
       header = (GRIP_BUF_C) image;
      if (header[0] != 'D' || header[1] != 'B')
	  return 0;

      /* Extract the image offsets from the header */
       img_off = ((header[2] & 0x0F) << 12) |
       ((header[3] & 0x0F) << 8) |
       ((header[4] & 0x0F) << 4) |
       ((header[5] & 0x0F));
       img_size = ((header[6] & 0x0F) << 12) |
       ((header[7] & 0x0F) << 8) |
       ((header[8] & 0x0F) << 4) |
       ((header[9] & 0x0F));
       img_check = ((header[10] & 0x0F) << 4) |
       ((header[11] & 0x0F));

      /* Compute the start of image core */
      if (size < ((img_off + img_size) << 4))
	  return 0;
       core = header + (img_off << 4);

      /* Verify the checksum */
      for (i = 0; i < (img_size << 4); i++)
	  img_check ^= core[i];
      if (img_check != 0)
	  return 0;

      /* Allocate memory for the core */
      memory = (unsigned long)DPMI_AllocDOS(img_size, &GRIP_DS);
      if (!memory)
	  return 0;

      /* Copy the image */
      dosmemput(core, img_size << 4, memory);

      /* Allocate a code selector for the core */
      GRIP_CS = DPMI_AllocSel();
      if (GRIP_CS == 0) {
	 DPMI_FreeDOS(GRIP_DS);
	 return 0;
      } if (DPMI_SetBounds(GRIP_CS, memory, img_size << 4) != 0) {
	 DPMI_FreeSel(GRIP_CS);
	 DPMI_FreeDOS(GRIP_DS);
	 return 0;
      } if (_DPMI_SetCodeAR(GRIP_CS) != 0) {
	 DPMI_FreeSel(GRIP_CS);
	 DPMI_FreeDOS(GRIP_DS);
	 return 0;
      }

      /* Prepare the thunking layer */
      for (i = 0; i < (int)sizeof(GRIP_THUNK); i++)
	 GRIP_Thunk[i] = ((GRIP_BUF_C) core)[i];

      *(GRIP_BUF_S) (GRIP_Thunk + 0x02) = 0;
      *(GRIP_BUF_S) (GRIP_Thunk + 0x06) = GRIP_CS;
      *(GRIP_BUF_S) (GRIP_Thunk + 0x0C) = GRIP_CS;

      /* Allocate a debugging selector if GRIP_DEBUG is defined */
#if defined(GRIP_DEBUG)
      /* I don't know what this does... at least it compiles okay */
      GRIP_ES = DPMI_AllocSel();
      if (GRIP_ES == 0) {
	 DPMI_FreeSel(GRIP_CS);
	 DPMI_FreeDOS(GRIP_DS);
	 return 0;
      }
      if (DPMI_SetBounds(GRIP_ES, 0xB0000, 80 * 25 * 2) != 0) {
	 DPMI_FreeSel(GRIP_ES);
	 DPMI_FreeSel(GRIP_CS);
	 DPMI_FreeDOS(GRIP_DS);
	 return 0;
      }
      if (DPMI_SetDataAR(GRIP_ES) != 0) {
	 DPMI_FreeSel(GRIP_ES);
	 DPMI_FreeSel(GRIP_CS);
	 DPMI_FreeDOS(GRIP_DS);
	 return 0;
      }
      _farpokew(_dos_ds, memory + 0x12, 0xB000);
      _farpokew(_dos_ds, memory + 0x16, GRIP_ES);
#else
      _farpokew(_dos_ds, memory + 0x12, 0);
      _farpokew(_dos_ds, memory + 0x16, 0);
#endif

      /* Save the data selector */
      _farpokew(_dos_ds, memory + 0x10, 0);
      _farpokew(_dos_ds, memory + 0x14, GRIP_DS);

      /* Complete the link */
      if (_Gr__Link() == 0) {
#if defined(GRIP_DEBUG)
	 DPMI_FreeSel(GRIP_ES);
#endif
	 DPMI_FreeSel(GRIP_CS);
	 DPMI_FreeDOS(GRIP_DS);
	 return 0;
      }

      GRIP_Thunked = 1;

      return 1;
   }

   GRIP_BOOL _GrUnlink(void) {
      /* Check that the library has been loaded */
      if (!GRIP_Thunked)
	 return 0;

      /* Unlink the library */
      _Gr__Unlink();

      /* Free the library memory */
#if defined(GRIP_DEBUG)
      if (DPMI_FreeSel(GRIP_ES) != 0)
	 return 0;
#endif
      if (DPMI_FreeSel(GRIP_CS) != 0)
	 return 0;
      if (DPMI_FreeDOS(GRIP_DS) != 0)
	 return 0;

      GRIP_Thunked = 0;

      return 1;
   }
