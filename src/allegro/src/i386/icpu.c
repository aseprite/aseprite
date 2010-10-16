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


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* helpers from icpus.s */
int _i_is_486(void);
int _i_is_fpu(void);
int _i_is_cyrix(void);
void _i_cx_w(int index, int value);
char _i_cx_r(int index);
int _i_is_cpuid_supported(void);
void _i_get_cpuid_info(uint32_t cpuid_levels, uint32_t *reg);



/* cyrix_type:
 *  Detects which type of Cyrix CPU is in use.
 */
static void cyrix_type(void)
{
   char orgc2, newc2, orgc3, newc3;
   int cr2_rw = FALSE, cr3_rw = FALSE, type;

   type = 0xFF;

   orgc2 = _i_cx_r(0xC2);              /* get current c2 value */
   newc2 = orgc2 ^ 4;                  /* toggle test bit */
   _i_cx_w(0xC2, newc2);               /* write test value to c2 */
   _i_cx_r(0xC0);                      /* dummy read to change bus */
   if (_i_cx_r(0xC2) != orgc2)         /* did test bit toggle */
      cr2_rw = TRUE;                   /* yes bit changed */
   _i_cx_w(0xC2, orgc2);               /* return c2 to original value */

   orgc3 = _i_cx_r(0xC3);              /* get current c3 value */
   newc3 = orgc3 ^ 0x80;               /* toggle test bit */
   _i_cx_w(0xC3, newc3);               /* write test value to c3 */
   _i_cx_r(0xC0);                      /* dummy read to change bus */
   if (_i_cx_r(0xC3) != orgc3)         /* did test bit change */
      cr3_rw = TRUE;                   /* yes it did */
   _i_cx_w(0xC3, orgc3);               /* return c3 to original value */

   if (((cr2_rw) && (cr3_rw)) || ((!cr2_rw) && (cr3_rw))) {
      type = _i_cx_r(0xFE);            /* DEV ID register ok */
   }
   else if ((cr2_rw) && (!cr3_rw)) {
      type = 0xFE;                     /* Cx486S A step */
   }
   else if ((!cr2_rw) && (!cr3_rw)) {
      type = 0xFD;                     /* Pre ID Regs. Cx486SLC or DLC */
   }

   if ((type < 0x30) || (type > 0xFC)) {
      cpu_family = 4;                  /* 486 class-including 5x86 */
      cpu_model = 14;                  /* Cyrix */
   }
   else if (type < 0x50) {
      cpu_family = 5;                  /* Pentium class-6x86 and Media GX */
      cpu_model = 14;                  /* Cyrix */
   }
   else {
      cpu_family = 6;                  /* Pentium || class- 6x86MX */
      cpu_model = 14;                  /* Cyrix */
      cpu_capabilities |= CPU_MMX;
   }
}



/* check_cpu:
 *  This is the function to call to set the globals
 */
void check_cpu() 
{
   uint32_t cpuid_levels;
   uint32_t vendor_temp[4];
   uint32_t reg[4];

   cpu_capabilities = 0;

   if (_i_is_cpuid_supported()) {
      cpu_capabilities |= CPU_ID;
      _i_get_cpuid_info(0x00000000, reg);
      cpuid_levels = reg[0];
      vendor_temp[0] = reg[1];
      vendor_temp[1] = reg[3];
      vendor_temp[2] = reg[2];
      vendor_temp[3] = 0;
      do_uconvert((char *)vendor_temp, U_ASCII, cpu_vendor, U_CURRENT,
		  _AL_CPU_VENDOR_SIZE);

      if (cpuid_levels > 0) {
	 reg[0] = reg[1] = reg[2] = reg[3] = 0;
	 _i_get_cpuid_info(1, reg);

	 cpu_family = (reg[0] & 0xF00) >> 8;
	 cpu_model = (reg[0] & 0xF0) >> 4;
	 /* Note: cpu_family = 15 can mean a Pentium IV, Xeon, AMD64, Opteron... */

	 cpu_capabilities |= (reg[3] & 1 ? CPU_FPU : 0);
	 cpu_capabilities |= (reg[3] & 0x800000 ? CPU_MMX : 0);

	 /* SSE has MMX+ included */
	 cpu_capabilities |= (reg[3] & 0x02000000 ? CPU_SSE | CPU_MMXPLUS : 0);
	 cpu_capabilities |= (reg[3] & 0x04000000 ? CPU_SSE2 : 0);
         cpu_capabilities |= (reg[2] & 0x00000001 ? CPU_SSE3 : 0);
         cpu_capabilities |= (reg[2] & 0x00000200 ? CPU_SSSE3 : 0);
         cpu_capabilities |= (reg[2] & 0x00080000 ? CPU_SSE41 : 0);
         cpu_capabilities |= (reg[2] & 0x00100000 ? CPU_SSE42 : 0);
	 cpu_capabilities |= (reg[3] & 0x00008000 ? CPU_CMOV : 0);
         cpu_capabilities |= (reg[3] & 0x40000000 ? CPU_IA64 : 0);
      }

      _i_get_cpuid_info(0x80000000, reg);
      if (reg[0] > 0x80000000) {
	 _i_get_cpuid_info(0x80000001, reg);

	 cpu_capabilities |= (reg[3] & 0x80000000 ? CPU_3DNOW : 0);
	 cpu_capabilities |= (reg[3] & 0x20000000 ? CPU_AMD64 : 0);
	  
	 /* Enhanced 3DNow! has MMX+ included */	 
	 cpu_capabilities |= (reg[3] & 0x40000000 ? CPU_ENH3DNOW | CPU_MMXPLUS : 0);
      }

      if (_i_is_cyrix())
	 cpu_model = 14;
   }
   else {
      cpu_capabilities |= (_i_is_fpu() ? CPU_FPU : 0);
      if (!_i_is_486()) {
	 cpu_family = 3;
      }
      else {
	 if (_i_is_cyrix()) {
	    do_uconvert("CyrixInstead", U_ASCII, cpu_vendor, U_CURRENT,
			_AL_CPU_VENDOR_SIZE);
	    cyrix_type();
	 }
	 else {
	    cpu_family = 4;
	    cpu_model = 15;
	 }
      }
   }
}

