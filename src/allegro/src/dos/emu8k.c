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
 *      Low-level functions for the AWE32 driver.
 *
 *      By George Foot.
 *
 *      Information taken primarily from "AWE32/EMU8000 Programmer's Guide"
 *      (AEPG) by Dave Rossum. The AEPG is part of the AWE32 Developers'
 *      Information Pack (ADIP) from Creative Labs.
 *
 *      I/O port verification technique taken from "The Un-official Sound
 *      Blaster AWE32 Programming Guide" by Vince Vu a.k.a. Judge Dredd
 *
 *      Miscellaneous guidance taken from Takashi Iwai's Linux AWE32 driver
 *
 *      See readme.txt for copyright information.
 */


#include <math.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "emu8k.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/****************************************************************************\
*                                                                            *
* EMU8000.H : EMU8000 init arrays ( for hardware level programming only )    *
*                                                                            *
* (C) Copyright Creative Technology Ltd. 1992-96. All rights reserved        *
* worldwide.                                                                 *
*                                                                            *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY      *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE        *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR      *
* PURPOSE.                                                                   *
*                                                                            *
\****************************************************************************/

static unsigned short init1_1[32] = { 0x03ff, 0x0030, 0x07ff, 0x0130, 0x0bff, 0x0230, 0x0fff, 0x0330, 0x13ff, 0x0430, 0x17ff, 0x0530, 0x1bff, 0x0630, 0x1fff, 0x0730, 0x23ff, 0x0830, 0x27ff, 0x0930, 0x2bff, 0x0a30, 0x2fff, 0x0b30, 0x33ff, 0x0c30, 0x37ff, 0x0d30, 0x3bff, 0x0e30, 0x3fff, 0x0f30 };
static unsigned short init1_2[32] = { 0x43ff, 0x0030, 0x47ff, 0x0130, 0x4bff, 0x0230, 0x4fff, 0x0330, 0x53ff, 0x0430, 0x57ff, 0x0530, 0x5bff, 0x0630, 0x5fff, 0x0730, 0x63ff, 0x0830, 0x67ff, 0x0930, 0x6bff, 0x0a30, 0x6fff, 0x0b30, 0x73ff, 0x0c30, 0x77ff, 0x0d30, 0x7bff, 0x0e30, 0x7fff, 0x0f30 };
static unsigned short init1_3[32] = { 0x83ff, 0x0030, 0x87ff, 0x0130, 0x8bff, 0x0230, 0x8fff, 0x0330, 0x93ff, 0x0430, 0x97ff, 0x0530, 0x9bff, 0x0630, 0x9fff, 0x0730, 0xa3ff, 0x0830, 0xa7ff, 0x0930, 0xabff, 0x0a30, 0xafff, 0x0b30, 0xb3ff, 0x0c30, 0xb7ff, 0x0d30, 0xbbff, 0x0e30, 0xbfff, 0x0f30 };
static unsigned short init1_4[32] = { 0xc3ff, 0x0030, 0xc7ff, 0x0130, 0xcbff, 0x0230, 0xcfff, 0x0330, 0xd3ff, 0x0430, 0xd7ff, 0x0530, 0xdbff, 0x0630, 0xdfff, 0x0730, 0xe3ff, 0x0830, 0xe7ff, 0x0930, 0xebff, 0x0a30, 0xefff, 0x0b30, 0xf3ff, 0x0c30, 0xf7ff, 0x0d30, 0xfbff, 0x0e30, 0xffff, 0x0f30 };

static unsigned short init2_1[32] = { 0x03ff, 0x8030, 0x07ff, 0x8130, 0x0bff, 0x8230, 0x0fff, 0x8330, 0x13ff, 0x8430, 0x17ff, 0x8530, 0x1bff, 0x8630, 0x1fff, 0x8730, 0x23ff, 0x8830, 0x27ff, 0x8930, 0x2bff, 0x8a30, 0x2fff, 0x8b30, 0x33ff, 0x8c30, 0x37ff, 0x8d30, 0x3bff, 0x8e30, 0x3fff, 0x8f30 };
static unsigned short init2_2[32] = { 0x43ff, 0x8030, 0x47ff, 0x8130, 0x4bff, 0x8230, 0x4fff, 0x8330, 0x53ff, 0x8430, 0x57ff, 0x8530, 0x5bff, 0x8630, 0x5fff, 0x8730, 0x63ff, 0x8830, 0x67ff, 0x8930, 0x6bff, 0x8a30, 0x6fff, 0x8b30, 0x73ff, 0x8c30, 0x77ff, 0x8d30, 0x7bff, 0x8e30, 0x7fff, 0x8f30 };
static unsigned short init2_3[32] = { 0x83ff, 0x8030, 0x87ff, 0x8130, 0x8bff, 0x8230, 0x8fff, 0x8330, 0x93ff, 0x8430, 0x97ff, 0x8530, 0x9bff, 0x8630, 0x9fff, 0x8730, 0xa3ff, 0x8830, 0xa7ff, 0x8930, 0xabff, 0x8a30, 0xafff, 0x8b30, 0xb3ff, 0x8c30, 0xb7ff, 0x8d30, 0xbbff, 0x8e30, 0xbfff, 0x8f30 };
static unsigned short init2_4[32] = { 0xc3ff, 0x8030, 0xc7ff, 0x8130, 0xcbff, 0x8230, 0xcfff, 0x8330, 0xd3ff, 0x8430, 0xd7ff, 0x8530, 0xdbff, 0x8630, 0xdfff, 0x8730, 0xe3ff, 0x8830, 0xe7ff, 0x8930, 0xebff, 0x8a30, 0xefff, 0x8b30, 0xf3ff, 0x8c30, 0xf7ff, 0x8d30, 0xfbff, 0x8e30, 0xffff, 0x8f30 };

static unsigned short init3_1[32] = { 0x0C10, 0x8470, 0x14FE, 0xB488, 0x167F, 0xA470, 0x18E7, 0x84B5, 0x1B6E, 0x842A, 0x1F1D, 0x852A, 0x0DA3, 0x8F7C, 0x167E, 0xF254, 0x0000, 0x842A, 0x0001, 0x852A, 0x18E6, 0x8BAA, 0x1B6D, 0xF234, 0x229F, 0x8429, 0x2746, 0x8529, 0x1F1C, 0x86E7, 0x229E, 0xF224 };
static unsigned short init3_2[32] = { 0x0DA4, 0x8429, 0x2C29, 0x8529, 0x2745, 0x87F6, 0x2C28, 0xF254, 0x383B, 0x8428, 0x320F, 0x8528, 0x320E, 0x8F02, 0x1341, 0xF264, 0x3EB6, 0x8428, 0x3EB9, 0x8528, 0x383A, 0x8FA9, 0x3EB5, 0xF294, 0x3EB7, 0x8474, 0x3EBA, 0x8575, 0x3EB8, 0xC4C3, 0x3EBB, 0xC5C3 };
static unsigned short init3_3[32] = { 0x0000, 0xA404, 0x0001, 0xA504, 0x141F, 0x8671, 0x14FD, 0x8287, 0x3EBC, 0xE610, 0x3EC8, 0x8C7B, 0x031A, 0x87E6, 0x3EC8, 0x86F7, 0x3EC0, 0x821E, 0x3EBE, 0xD208, 0x3EBD, 0x821F, 0x3ECA, 0x8386, 0x3EC1, 0x8C03, 0x3EC9, 0x831E, 0x3ECA, 0x8C4C, 0x3EBF, 0x8C55 };
static unsigned short init3_4[32] = { 0x3EC9, 0xC208, 0x3EC4, 0xBC84, 0x3EC8, 0x8EAD, 0x3EC8, 0xD308, 0x3EC2, 0x8F7E, 0x3ECB, 0x8219, 0x3ECB, 0xD26E, 0x3EC5, 0x831F, 0x3EC6, 0xC308, 0x3EC3, 0xB2FF, 0x3EC9, 0x8265, 0x3EC9, 0x8319, 0x1342, 0xD36E, 0x3EC7, 0xB3FF, 0x0000, 0x8365, 0x1420, 0x9570 };

static unsigned short init4_1[32] = { 0x0C10, 0x8470, 0x14FE, 0xB488, 0x167F, 0xA470, 0x18E7, 0x84B5, 0x1B6E, 0x842A, 0x1F1D, 0x852A, 0x0DA3, 0x0F7C, 0x167E, 0x7254, 0x0000, 0x842A, 0x0001, 0x852A, 0x18E6, 0x0BAA, 0x1B6D, 0x7234, 0x229F, 0x8429, 0x2746, 0x8529, 0x1F1C, 0x06E7, 0x229E, 0x7224 };
static unsigned short init4_2[32] = { 0x0DA4, 0x8429, 0x2C29, 0x8529, 0x2745, 0x07F6, 0x2C28, 0x7254, 0x383B, 0x8428, 0x320F, 0x8528, 0x320E, 0x0F02, 0x1341, 0x7264, 0x3EB6, 0x8428, 0x3EB9, 0x8528, 0x383A, 0x0FA9, 0x3EB5, 0x7294, 0x3EB7, 0x8474, 0x3EBA, 0x8575, 0x3EB8, 0x44C3, 0x3EBB, 0x45C3 };
static unsigned short init4_3[32] = { 0x0000, 0xA404, 0x0001, 0xA504, 0x141F, 0x0671, 0x14FD, 0x0287, 0x3EBC, 0xE610, 0x3EC8, 0x0C7B, 0x031A, 0x07E6, 0x3EC8, 0x86F7, 0x3EC0, 0x821E, 0x3EBE, 0xD208, 0x3EBD, 0x021F, 0x3ECA, 0x0386, 0x3EC1, 0x0C03, 0x3EC9, 0x031E, 0x3ECA, 0x8C4C, 0x3EBF, 0x0C55 };
static unsigned short init4_4[32] = { 0x3EC9, 0xC208, 0x3EC4, 0xBC84, 0x3EC8, 0x0EAD, 0x3EC8, 0xD308, 0x3EC2, 0x8F7E, 0x3ECB, 0x0219, 0x3ECB, 0xD26E, 0x3EC5, 0x031F, 0x3EC6, 0xC308, 0x3EC3, 0x32FF, 0x3EC9, 0x0265, 0x3EC9, 0x8319, 0x1342, 0xD36E, 0x3EC7, 0x33FF, 0x0000, 0x8365, 0x1420, 0x9570 };

/*** (end of init arrays) ***/



#define MIN_LOOP_LEN 2

int _emu8k_baseport = 0;
int _emu8k_numchannels = 32;



/*  Functions to deal with the AWE32's I/O ports
 */
static INLINE void write_word(int reg, int channel, int port, int data)
{
   int out_port = _emu8k_baseport;

   switch (port) {

      case 0:
	 out_port += 0x0000;
	 break;

      case 1:
	 out_port += 0x0400;
	 break;

      case 2:
	 out_port += 0x0402;
	 break;

      case 3:
	 out_port += 0x0800;
	 break;

      default:
	 return;
   }

   outportw(_emu8k_baseport + 0x802, (reg << 5) + (channel & 0x1f));
   outportw(out_port, data);
}



static INLINE unsigned int read_word(int reg, int channel, int port)
{
   int in_port = _emu8k_baseport;

   switch (port) {

      case 0:
	 in_port += 0x0000;
	 break;

      case 1:
	 in_port += 0x0400;
	 break;

      case 2:
	 in_port += 0x0402;
	 break;

      case 3:
	 in_port += 0x0800;
	 break;

      default:
	 return 0;
   }

   outportw(_emu8k_baseport + 0x802, (reg << 5) + (channel & 0x1f));
   return inportw(in_port);
}



static INLINE void write_dword(int reg, int channel, int port, unsigned int data)
{
   int out_port = _emu8k_baseport;

   switch (port) {

      case 0:
	 out_port += 0x0000;
	 break;

      case 1:
	 out_port += 0x0400;
	 break;

      case 2:
	 out_port += 0x0402;
	 break;

      case 3:
	 out_port += 0x0800;
	 break;

      default:
	 return;
   }

   outportw(_emu8k_baseport + 0x802, (reg << 5) + (channel & 0x1f));
   outportw(out_port, data & 0xffff);
   outportw(out_port + 2, (data >> 16) & 0xffff);
}



static INLINE unsigned int read_dword(int reg, int channel, int port)
{
   int in_port, a, b;

   in_port = _emu8k_baseport;

   switch (port) {

      case 0:
	 in_port += 0x0000;
	 break;

      case 1:
	 in_port += 0x0400;
	 break;

      case 2:
	 in_port += 0x0402;
	 break;

      case 3:
	 in_port += 0x0800;
	 break;

      default:
	 return 0;
   }

   outportw(_emu8k_baseport + 0x802, (reg << 5) + (channel & 0x1f));
   a = inportw(in_port);
   b = inportw(in_port + 2);
   return ((b << 16) + a);
}



/* write_*:
 *  Functions to write information to the AWE32's registers
 *  (abbreviated as in the EPG).
 */
static INLINE void write_CPF(int channel, int i)
{
   write_dword(0, channel, 0, i);
}



static INLINE void write_PTRX(int channel, int i)
{
   write_dword(1, channel, 0, i);
}



static INLINE void write_CVCF(int channel, int i)
{
   write_dword(2, channel, 0, i);
}



static INLINE void write_VTFT(int channel, int i)
{
   write_dword(3, channel, 0, i);
}



static INLINE void write_PSST(int channel, int i)
{
   write_dword(6, channel, 0, i);
}



static INLINE void write_CSL(int channel, int i)
{
   write_dword(7, channel, 0, i);
}



static INLINE void write_CCCA(int channel, int i)
{
   write_dword(0, channel, 1, i);
}



static INLINE void write_HWCF4(int i)
{
   write_dword(1, 9, 1, i);
}



static INLINE void write_HWCF5(int i)
{
   write_dword(1, 10, 1, i);
}



static INLINE void write_HWCF6(int i)
{
   write_dword(1, 13, 1, i);
}



static INLINE void write_SMALR(int i)
{
   write_dword(1, 20, 1, i);
}



static INLINE void write_SMARR(int i)
{
   write_dword(1, 21, 1, i);
}



static INLINE void write_SMALW(int i)
{
   write_dword(1, 22, 1, i);
}



static INLINE void write_SMARW(int i)
{
   write_dword(1, 23, 1, i);
}



/*
static INLINE void write_SMLD(int i)
{
   write_word(1, 26, 1, i);
}
*/



/*
static INLINE void write_SMRD(int i)
{
   write_word(1, 26, 2, i);
}
*/



/*
static INLINE void write_WC(int i)
{
   write_word(1, 27, 2, i);
}
*/



static INLINE void write_HWCF1(int i)
{
   write_word(1, 29, 1, i);
}



static INLINE void write_HWCF2(int i)
{
   write_word(1, 30, 1, i);
}



static INLINE void write_HWCF3(int i)
{
   write_word(1, 31, 1, i);
}



static INLINE void write_INIT1(int channel, int i)
{
   write_word(2, channel, 1, i);
}                               /* `channel' is really `element' here */



static INLINE void write_INIT2(int channel, int i)
{
   write_word(2, channel, 2, i);
}



static INLINE void write_INIT3(int channel, int i)
{
   write_word(3, channel, 1, i);
}



static INLINE void write_INIT4(int channel, int i)
{
   write_word(3, channel, 2, i);
}



static INLINE void write_ENVVOL(int channel, int i)
{
   write_word(4, channel, 1, i);
}



static INLINE void write_DCYSUSV(int channel, int i)
{
   write_word(5, channel, 1, i);
}



static INLINE void write_ENVVAL(int channel, int i)
{
   write_word(6, channel, 1, i);
}



static INLINE void write_DCYSUS(int channel, int i)
{
   write_word(7, channel, 1, i);
}



static INLINE void write_ATKHLDV(int channel, int i)
{
   write_word(4, channel, 2, i);
}



static INLINE void write_LFO1VAL(int channel, int i)
{
   write_word(5, channel, 2, i);
}



static INLINE void write_ATKHLD(int channel, int i)
{
   write_word(6, channel, 2, i);
}



static INLINE void write_LFO2VAL(int channel, int i)
{
   write_word(7, channel, 2, i);
}



static INLINE void write_IP(int channel, int i)
{
   write_word(0, channel, 3, i);
}



static INLINE void write_IFATN(int channel, int i)
{
   write_word(1, channel, 3, i);
}



static INLINE void write_PEFE(int channel, int i)
{
   write_word(2, channel, 3, i);
}



static INLINE void write_FMMOD(int channel, int i)
{
   write_word(3, channel, 3, i);
}



static INLINE void write_TREMFRQ(int channel, int i)
{
   write_word(4, channel, 3, i);
}



static INLINE void write_FM2FRQ2(int channel, int i)
{
   write_word(5, channel, 3, i);
}


/* read_*:
 *  Functions to read information from the AWE32's registers
 *  (abbreviated as in the AEPG).
 */
/*
static INLINE int read_CPF(int channel)
{
   return read_dword(0, channel, 0);
}
*/



/*
static INLINE int read_PTRX(int channel)
{
   return read_dword(1, channel, 0);
}
*/



/*
static INLINE int read_CVCF(int channel)
{
   return read_dword(2, channel, 0);
}
*/



/*
static INLINE int read_VTFT(int channel)
{
   return read_dword(3, channel, 0);
}
*/



static INLINE int read_PSST(int channel)
{
   return read_dword(6, channel, 0);
}



/*
static INLINE int read_CSL(int channel)
{
   return read_dword(7, channel, 0);
}
*/



/*
static INLINE int read_CCCA(int channel)
{
   return read_dword(0, channel, 1);
}
*/



/*
static INLINE int read_HWCF4(void)
{
   return read_dword(1, 9, 1);
}
*/



/*
static INLINE int read_HWCF5(void)
{
   return read_dword(1, 10, 1);
}
*/



/*
static INLINE int read_HWCF6(void)
{
   return read_dword(1, 13, 1);
}
*/



/*
static INLINE int read_SMALR(void)
{
   return read_dword(1, 20, 1);
}
*/



/*
static INLINE int read_SMARR(void)
{
   return read_dword(1, 21, 1);
}
*/



/*
static INLINE int read_SMALW(void)
{
   return read_dword(1, 22, 1);
}
*/



/*
static INLINE int read_SMARW(void)
{
   return read_dword(1, 23, 1);
}
*/



/*
static INLINE int read_SMLD(void)
{
   return read_word(1, 26, 1);
}
*/



/*
static INLINE int read_SMRD(void)
{
   return read_word(1, 26, 2);
}
*/



/*
static INLINE int read_WC(void)
{
   return read_word(1, 27, 2);
}
*/



/*
static INLINE int read_HWCF1(void)
{
   return read_word(1, 29, 1);
}
*/



/*
static INLINE int read_HWCF2(void)
{
   return read_word(1, 30, 1);
}
*/



/*
static INLINE int read_HWCF3(void)
{
   return read_word(1, 31, 1);
}
*/



/*
static INLINE int read_INIT1(void)
{
   return read_word(2, 0, 1);
}
*/



/*
static INLINE int read_INIT2(void)
{
   return read_word(2, 0, 2);
}
*/



/*
static INLINE int read_INIT3(void)
{
   return read_word(3, 0, 1);
}
*/



/*
static INLINE int read_INIT4(void)
{
   return read_word(3, 0, 2);
}
*/



/*
static INLINE int read_ENVVOL(int channel)
{
   return read_word(4, channel, 1);
}
*/



/*
static INLINE int read_DCYSUSV(int channel)
{
   return read_word(5, channel, 1);
}
*/



/*
static INLINE int read_ENVVAL(int channel)
{
   return read_word(6, channel, 1);
}
*/



/*
static INLINE int read_DCYSUS(int channel)
{
   return read_word(7, channel, 1);
}
*/



/*
static INLINE int read_ATKHLDV(int channel)
{
   return read_word(4, channel, 2);
}
*/



/*
static INLINE int read_LFO1VAL(int channel)
{
   return read_word(5, channel, 2);
}
*/



/*
static INLINE int read_ATKHLD(int channel)
{
   return read_word(6, channel, 2);
}
*/



/*
static INLINE int read_LFO2VAL(int channel)
{
   return read_word(7, channel, 2);
}
*/



/*
static INLINE int read_IP(int channel)
{
   return read_word(0, channel, 3);
}
*/



static INLINE int read_IFATN(int channel)
{
   return read_word(1, channel, 3);
}



/*
static INLINE int read_PEFE(int channel)
{
   return read_word(2, channel, 3);
}
*/



/*
static INLINE int read_FMMOD(int channel)
{
   return read_word(3, channel, 3);
}
*/



/*
static INLINE int read_TREMFRQ(int channel)
{
   return read_word(4, channel, 3);
}
*/



/*
static INLINE int read_FM2FRQ2(int channel)
{
   return read_word(5, channel, 3);
}
*/



/* wait_*:
 *  Functions to wait for DMA streams to be ready.
 */
/*
static INLINE void wait_LR(void)
{
   while (read_SMALR() & 0x80000000) ;
}
*/



/*
static INLINE void wait_LW(void)
{
   while (read_SMALW() & 0x80000000) ;
}
*/



/*
static INLINE void wait_RR(void)
{
   while (read_SMARR() & 0x80000000) ;
}
*/



/*
static INLINE void wait_RW(void)
{
   while (read_SMARW() & 0x80000000) ;
}
*/



/* write_init_arrays:
 *  Writes the given set of initialisation arrays to the card.
 */
static void write_init_arrays(unsigned short *init1, unsigned short *init2, unsigned short *init3, unsigned short *init4)
{
   int i;

   for (i = 0; i < 32; i++)
      write_INIT1(i, init1[i]);

   for (i = 0; i < 32; i++)
      write_INIT2(i, init2[i]);

   for (i = 0; i < 32; i++)
      write_INIT3(i, init3[i]);

   for (i = 0; i < 32; i++)
      write_INIT4(i, init4[i]);
}



/* emu8k_init:
 *  Initialise the synthesiser. See AEPG chapter 4.
 */
void emu8k_init(void)
{
   int channel;

   write_HWCF1(0x0059);
   write_HWCF2(0x0020);

   for (channel=0; channel<_emu8k_numchannels; channel++)
      write_DCYSUSV(channel, 0x0080);

   for (channel=0; channel<_emu8k_numchannels; channel++) {
      write_ENVVOL(channel, 0);
      write_ENVVAL(channel, 0);
      write_DCYSUS(channel, 0);
      write_ATKHLDV(channel, 0);
      write_LFO1VAL(channel, 0);
      write_ATKHLD(channel, 0);
      write_LFO2VAL(channel, 0);
      write_IP(channel, 0);
      write_IFATN(channel, 0);
      write_PEFE(channel, 0);
      write_FMMOD(channel, 0);
      write_TREMFRQ(channel, 0);
      write_FM2FRQ2(channel, 0);
      write_PTRX(channel, 0);
      write_VTFT(channel, 0);
      write_PSST(channel, 0);
      write_CSL(channel, 0);
      write_CCCA(channel, 0);
   }

   for (channel=0; channel<_emu8k_numchannels; channel++) {
      write_CPF(channel, 0);
      write_CVCF(channel, 0);
   }

   write_SMALR(0);
   write_SMARR(0);
   write_SMALW(0);
   write_SMARW(0);
   write_init_arrays(init1_1, init1_2, init1_3, init1_4);
   rest(25);                    /* wait 24 msec + 1 for luck */
   write_init_arrays(init2_1, init2_2, init2_3, init2_4);
   write_init_arrays(init3_1, init3_2, init3_3, init3_4);
   write_HWCF4(0);
   write_HWCF5(0x00000083);
   write_HWCF6(0x00008000);
   write_init_arrays(init4_1, init4_2, init4_3, init4_4);
   write_HWCF3(0x0004);
}



/* emu8k_startsound:
 *  Start a sound on a channel.
 */
void emu8k_startsound(int channel, struct envparms_t *envparms)
{
   int psst = 0, csl = 0;

   write_DCYSUSV(channel, 0x0080);
   write_VTFT(channel, 0);
   write_CVCF(channel, 0);
   write_PTRX(channel, 0);
   write_CPF(channel, 0);

   write_ENVVOL(channel, envparms->envvol);
   write_ENVVAL(channel, envparms->envval);
   write_DCYSUS(channel, envparms->dcysus);
   write_ATKHLDV(channel, envparms->atkhldv);
   write_LFO1VAL(channel, envparms->lfo1val);
   write_ATKHLD(channel, envparms->atkhld);
   write_LFO2VAL(channel, envparms->lfo2val);
   write_IP(channel, envparms->ip);
   write_IFATN(channel, envparms->ifatn);
   write_PEFE(channel, envparms->pefe);
   write_FMMOD(channel, envparms->fmmod);
   write_TREMFRQ(channel, envparms->tremfrq);
   write_FM2FRQ2(channel, envparms->fm2frq2);

   switch (envparms->smpmode & 3) {

      case 0:                    /* no looping */
      case 2:                    /* invalid; should be read as `no looping' */
	 psst = (envparms->psst & 0xff000000)   /* copy pan verbatim; mask out loop start */
	     + (envparms->sampend)              /* set loop start to near sample end */
	     - MIN_LOOP_LEN;
	 csl = (envparms->csl & 0xff000000)     /* copy chorus verbatim; mask out loop end */
	     + (envparms->sampend)              /* set loop end to sample end */
	     + MIN_LOOP_LEN;
	 break;

      case 1:                    /* loop indefinitely */
      case 3:                    /* loop until key released, then continue */
	 psst = envparms->psst;  /* leave the loop as it stands */
	 csl = envparms->csl;
	 break;
   }

   write_PSST(channel, psst);
   write_CSL(channel, csl);

   write_CCCA(channel, envparms->ccca);

   write_VTFT(channel, 0x0000FFFF);
   write_CVCF(channel, 0x0000FFFF);
   write_DCYSUSV(channel, envparms->dcysusv);
   write_PTRX(channel, envparms->ptrx);
   write_CPF(channel, 0x40000000);
}

END_OF_STATIC_FUNCTION(emu8k_startsound);



/* emu8k_releasesound:
 *  End a sound on a channel by starting its release phase.
 */
void emu8k_releasesound(int channel, struct envparms_t *envparms)
{
   if (envparms->smpmode == 3) {        /* If sound should stop looping and continue, do so */
      write_CSL(channel, (envparms->chorus << 24) + envparms->sampend + MIN_LOOP_LEN);
      write_PSST(channel, (envparms->pan << 24) + envparms->sampend - MIN_LOOP_LEN);
   }

   write_DCYSUSV(channel, 0x8000 + envparms->volrel);   /* Start volume envelope release phase */
   write_DCYSUS(channel, 0x8000 + envparms->modrel);    /* Start modulation envelope release phase */
}

END_OF_STATIC_FUNCTION(emu8k_releasesound);



/* emu8k_modulate_*:
 *  Modulation functions - changing a channel's sound in various ways.
 */
void emu8k_modulate_atten(int channel, int atten)
{
   int ifatn;
   ifatn = read_IFATN(channel);
   write_IFATN(channel, (ifatn & 0xff00) + (atten & 0xff));
}

END_OF_STATIC_FUNCTION(emu8k_modulate_atten);




void emu8k_modulate_ip(int channel, int ip)
{
   write_IP(channel, ip);
}

END_OF_STATIC_FUNCTION(emu8k_modulate_ip);



void emu8k_modulate_pan(int channel, int pan)
{
   int psst;
   psst = read_PSST(channel);
   write_PSST(channel, (pan << 24) + (psst & 0x00ffffff));
}

END_OF_STATIC_FUNCTION(emu8k_modulate_pan);



/* emu8k_terminatesound:
 *  End a sound on a channel immediately (may produce an audible click).
 */
void emu8k_terminatesound(int channel)
{
   write_DCYSUSV(channel, 0x0080);
   write_VTFT(channel, 0x0000FFFF);
   write_CVCF(channel, 0x0000FFFF);
}

END_OF_STATIC_FUNCTION(emu8k_terminatesound);



/* emu8k_detect:
 *  Locate the EMU8000. This tries to extract the base port from the E section
 *  of the BLASTER environment variable, and then does some test reads to check
 *  that there is an EMU8000 there.
 */
int emu8k_detect(void)
{
   char *envvar;

   if (!(envvar = getenv("BLASTER"))) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("BLASTER environment variable not set"));
      return 0;
   }

   _emu8k_baseport = 0;

   while (*envvar) {
      if (*envvar == 'E')
	 _emu8k_baseport = strtol(envvar + 1, NULL, 16);

      while ((*envvar != ' ') && (*envvar != 0))
	 envvar++;

      if (*envvar)
	 envvar++;
   }

   if (!_emu8k_baseport) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("BLASTER environment variable has no E section"));
      return 0;
   }

   uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("AWE32 detection failed on port 0x%04x"), _emu8k_baseport);

   if ((read_word(7, 0, 3) & 0x000f) != 0x000c)
      return 0;
   if ((read_word(1, 29, 1) & 0x007e) != 0x0058)
      return 0;
   if ((read_word(1, 30, 1) & 0x0003) != 0x0003)
      return 0;

   usetc(allegro_error, 0);
   return 1;
}



/* emu_*:
 *  Functions to convert SoundFont information to EMU8000 parameters.
 */
static INLINE unsigned short emu_delay(int x)
{
   int a = 0x8000 - pow(2, x / 1200.0) / 0.000725;
   return (a < 0) ? 0 : ((a > 0x8000) ? 0x8000 : a);
}



static INLINE unsigned char emu_attack(int x)
{
   /*
    * AEPG doesn't specify exact conversion here; I'm using what Takashi 
    * Iwai used for his Linux driver
    */
   int a, msec;
   msec = pow(2, x / 1200.0) * 1000;
   if (msec == 0)
      return 0x7f;

   a = 11878 / msec;

   return (a < 1) ? 1 : ((a > 0x7f) ? 0x7f : a);
}



static INLINE unsigned char emu_hold(int x)
{
   int a = 0x7f - (int)(pow(2, x / 1200.0) * 0x7f / 11.68);
   return (a < 0) ? 0 : ((a > 0x7f) ? 0x7f : a);
}



static INLINE unsigned char emu_decay(int x)
{
   /*
    * AEPG doesn't specify exact conversion here; I'm using an adaptation of
    * what Takashi Iwai used for his Linux driver
    */
   int a, msec;
   msec = pow(2, x / 1200.0) * 1000;
   if (msec == 0)
      return 0x7f;
   a = 0x7f - 54.8 * log10(msec / 23.04);       /* Takashi's */
   // a = 47349/(msec+349);                     /* mine */
   return (a < 1) ? 1 : ((a > 0x7f) ? 0x7f : a);
}



static INLINE unsigned char emu_sustain(int x)
{
   /* This is not the same as in Takashi Iwai's Linux driver, but I think I'm right */
   int a = 0x7f - x / 7.5;
   return (a < 0) ? 0 : ((a > 0x7f) ? 0x7f : a);
}



static INLINE unsigned char emu_release(int x)
{
   return emu_decay(x);
}



static INLINE unsigned char emu_mod(int x, int peak_deviation)
{
   int a = x * 0x80 / peak_deviation;
   return (a < -0x80) ? 0x80 : ((a > 0x7f) ? 0x7f : a);
}



static INLINE unsigned char emu_freq(int x)
{
   int a = (8.176 * pow(2, x / 1200.0) + 0.032) / 0.042;
   return (a < 0) ? 0 : ((a > 0xff) ? 0xff : a);
}



static INLINE unsigned char emu_reverb(int x)
{
   int a = (x * 0xff) / 1000;
   return (a < 0) ? 0 : ((a > 0xff) ? 0xff : a);
}



static INLINE unsigned char emu_chorus(int x)
{
   int a = (x * 0xff) / 1000;
   return (a < 0) ? 0 : ((a > 0xff) ? 0xff : a);
}



static INLINE unsigned char emu_pan(int x)
{
   int a = 0x7f - (x * 0xff) / 1000;
   return (a < 8) ? 8 : ((a > 0xf8) ? 0xf8 : a);
}



static INLINE unsigned char emu_filterQ(int x)
{
   int a = (x / 15);
   return (a < 0) ? 0 : ((a > 15) ? 15 : a);
}



static INLINE unsigned int emu_address(int base, int offset, int coarse_offset)
{
   return base + offset + coarse_offset * 32 * 1024;
}



/*
static INLINE unsigned short emu_pitch(int key, int rootkey, int scale, int coarse, int fine, int initial)
{
   int a = (((key - rootkey) * scale + coarse * 100 + fine - initial) * 0x1000) / 1200 + 0xE000;
   return (a < 0) ? 0 : ((a > 0xFFFF) ? 0xFFFF : a);
}
*/



static INLINE unsigned char emu_filter(int x)
{
   int a = (x - 4721) / 25;
   return (a < 0) ? 0 : ((a > 0xff) ? 0xff : a);
}



static INLINE unsigned char emu_atten(int x)
{
   int a = x / 3.75;
   return (a < 0) ? 0 : ((a > 0xff) ? 0xff : a);
}



/* emu8k_createenvelope:
 *  Converts a set of SoundFont generators to equivalent EMU8000 register
 *  settings, for passing to the sound playing functions above.
 */
envparms_t *emu8k_createenvelope(generators_t sfgen)
{
   envparms_t *envelope = (envparms_t *) _lock_malloc(sizeof(envparms_t));
   if (envelope) {
      envelope->envvol  = emu_delay(sfgen[sfgen_delayModEnv]);
      envelope->envval  = emu_delay(sfgen[sfgen_delayVolEnv]);
      envelope->modsust = (emu_sustain(sfgen[sfgen_sustainModEnv]) << 8);
      envelope->modrel  = (emu_release(sfgen[sfgen_releaseModEnv]) << 0);
      envelope->dcysus  = (0 << 15) // we're programming the decay, not the release
			+ envelope->modsust
			+ (0 << 7)  // unused
			+ (emu_decay(sfgen[sfgen_decayModEnv]) << 0);
      envelope->atkhldv = (0 << 15) // 0 otherwise it won't attack
			+ (emu_hold(sfgen[sfgen_holdVolEnv]) << 8)
			+ (0 << 7)  // unused
			+ (emu_attack(sfgen[sfgen_attackVolEnv]) << 0);
      envelope->lfo1val = emu_delay(sfgen[sfgen_delayModLFO]);
      envelope->atkhld  = (0 << 15) // 0 otherwise it won't attack
			+ (emu_hold(sfgen[sfgen_holdModEnv]) << 8)
			+ (0 << 7)  // unused
			+ (emu_attack(sfgen[sfgen_attackModEnv]) << 0);
      envelope->lfo2val = emu_delay(sfgen[sfgen_delayVibLFO]);
      envelope->pitch   = 0xe000;
      envelope->ip      = envelope->pitch;
      envelope->filter  = (emu_filter(sfgen[sfgen_initialFilterFc]) << 8);
      envelope->atten   = (emu_atten(sfgen[sfgen_initialAttenuation]) << 0);
      envelope->ifatn   = envelope->filter + envelope->atten;
      envelope->pefe    = (emu_mod(sfgen[sfgen_modEnvToPitch], 1200) << 8)
			+ (emu_mod(sfgen[sfgen_modEnvToFilterFc], 7200) << 0);
      envelope->fmmod   = (emu_mod(sfgen[sfgen_modLfoToPitch], 1200) << 8)
			+ (emu_mod(sfgen[sfgen_modLfoToFilterFc], 3600) << 0);
      envelope->tremfrq = (emu_mod(sfgen[sfgen_modLfoToVolume], 120) << 8)
			+ (emu_freq(sfgen[sfgen_freqModLFO]) << 0);
      envelope->fm2frq2 = (emu_mod(sfgen[sfgen_vibLfoToPitch], 1200) << 8)
			+ (emu_freq(sfgen[sfgen_freqVibLFO]) << 0);
      envelope->volsust = (emu_sustain(sfgen[sfgen_sustainVolEnv]) << 8);
      envelope->volrel  = (emu_release(sfgen[sfgen_releaseVolEnv]) << 0);
      envelope->dcysusv = (0 << 15) // we're programming decay not release
			+ envelope->volsust
			+ (0 << 7)  // turn on envelope engine
			+ (emu_decay(sfgen[sfgen_decayVolEnv]) << 0);
      envelope->ptrx    = (0x4000 << 16)  // ??? I thought the top word wasn't for me to use
			+ (emu_reverb(sfgen[sfgen_reverbEffectsSend]) << 8)
			+ (0 << 0);    // unused

      envelope->pan     = emu_pan(sfgen[sfgen_pan]);
      envelope->loopst  = emu_address(sfgen[gfgen_startloopAddrs], sfgen[sfgen_startloopAddrsOffset], sfgen[sfgen_startloopAddrsCoarseOffset]);
      envelope->sampend = emu_address(sfgen[gfgen_endAddrs], sfgen[sfgen_endAddrsOffset], sfgen[sfgen_endAddrsCoarseOffset]);
      envelope->psst    = (envelope->pan << 24)
			+ (envelope->loopst);
      envelope->chorus  = emu_chorus(sfgen[sfgen_chorusEffectsSend]);
      envelope->csl     = (envelope->chorus << 24)
			+ (emu_address(sfgen[gfgen_endloopAddrs], sfgen[sfgen_endloopAddrsOffset], sfgen[sfgen_endloopAddrsCoarseOffset]) << 0);
      envelope->ccca    = (emu_filterQ(sfgen[sfgen_initialFilterQ]) << 28)
			+ (0 << 24)    // DMA control bits
			+ (emu_address(sfgen[gfgen_startAddrs], sfgen[sfgen_startAddrsOffset], sfgen[sfgen_startAddrsCoarseOffset]) << 0);

      envelope->rootkey = (sfgen[sfgen_overridingRootKey] != -1) ? sfgen[sfgen_overridingRootKey] : 69;
      envelope->ipbase  = ((sfgen[sfgen_coarseTune] * 100 + sfgen[sfgen_fineTune] - envelope->rootkey * sfgen[sfgen_scaleTuning]) * 0x1000) / 1200 + 0xE000;
      envelope->ipscale = sfgen[sfgen_scaleTuning];

      envelope->minkey  = sfgen[sfgen_keyRange] & 0xff;
      envelope->maxkey  = (sfgen[sfgen_keyRange] >> 8) & 0xff;
      envelope->minvel  = sfgen[sfgen_velRange] & 0xff;
      envelope->maxvel  = (sfgen[sfgen_velRange] >> 8) & 0xff;
      envelope->key     = -1;
      envelope->vel     = -1;
      envelope->exc     = sfgen[sfgen_exclusiveClass];
      envelope->keyMEH  = 0;
      envelope->keyMED  = 0;
      envelope->keyVEH  = 0;
      envelope->keyVED  = 0;
      envelope->smpmode = sfgen[sfgen_sampleModes];
   }

   return envelope;
}



/* emu8k_destroyenvelope:
 *  Destroys an envelope created by emu8k_createenvelope.
 */
void emu8k_destroyenvelope(envparms_t * env)
{
   _AL_FREE(env);
}



/* emu8k_lock:
 *  Locks all data and functions which might be called in an interrupt context.
 */
void emu8k_lock(void)
{
   LOCK_VARIABLE(_emu8k_baseport);
   LOCK_VARIABLE(_emu8k_numchannels);

   LOCK_FUNCTION(emu8k_startsound);
   LOCK_FUNCTION(emu8k_releasesound);
   LOCK_FUNCTION(emu8k_modulate_atten);
   LOCK_FUNCTION(emu8k_modulate_ip);
   LOCK_FUNCTION(emu8k_modulate_pan);
   LOCK_FUNCTION(emu8k_terminatesound);
}
