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
 *      DMA routines for the sound code. I used to feel slightly guilty 
 *      because I pinched this code from MikMod, but then I looked at 
 *      the GUS SDK and realised that Jean Paul Mikkers took it from 
 *      there, so I think that justifies me in using it.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"



/* DMA controller #1 (8-bit controller) */
#define DMA1_STAT       0x08            /* read status register */
#define DMA1_WCMD       0x08            /* write command register */
#define DMA1_WREQ       0x09            /* write request register */
#define DMA1_SNGL       0x0A            /* write single bit register */
#define DMA1_MODE       0x0B            /* write mode register */
#define DMA1_CLRFF      0x0C            /* clear byte ptr flip/flop */
#define DMA1_MCLR       0x0D            /* master clear register */
#define DMA1_CLRM       0x0E            /* clear mask register */
#define DMA1_WRTALL     0x0F            /* write all mask register */

/* DMA controller #2 (16-bit controller) */
#define DMA2_STAT       0xD0            /* read status register */
#define DMA2_WCMD       0xD0            /* write command register */
#define DMA2_WREQ       0xD2            /* write request register */
#define DMA2_SNGL       0xD4            /* write single bit register */
#define DMA2_MODE       0xD6            /* write mode register */
#define DMA2_CLRFF      0xD8            /* clear byte ptr flip/flop */
#define DMA2_MCLR       0xDA            /* master clear register */
#define DMA2_CLRM       0xDC            /* clear mask register */
#define DMA2_WRTALL     0xDE            /* write all mask register */

/* stuff for each DMA channel */
#define DMA0_ADDR       0x00            /* chan 0 base adddress */
#define DMA0_CNT        0x01            /* chan 0 base count */
#define DMA1_ADDR       0x02            /* chan 1 base adddress */
#define DMA1_CNT        0x03            /* chan 1 base count */
#define DMA2_ADDR       0x04            /* chan 2 base adddress */
#define DMA2_CNT        0x05            /* chan 2 base count */
#define DMA3_ADDR       0x06            /* chan 3 base adddress */
#define DMA3_CNT        0x07            /* chan 3 base count */
#define DMA4_ADDR       0xC0            /* chan 4 base adddress */
#define DMA4_CNT        0xC2            /* chan 4 base count */
#define DMA5_ADDR       0xC4            /* chan 5 base adddress */
#define DMA5_CNT        0xC6            /* chan 5 base count */
#define DMA6_ADDR       0xC8            /* chan 6 base adddress */
#define DMA6_CNT        0xCA            /* chan 6 base count */
#define DMA7_ADDR       0xCC            /* chan 7 base adddress */
#define DMA7_CNT        0xCE            /* chan 7 base count */

#define DMA0_PAGE       0x87            /* chan 0 page register (refresh) */
#define DMA1_PAGE       0x83            /* chan 1 page register */
#define DMA2_PAGE       0x81            /* chan 2 page register */
#define DMA3_PAGE       0x82            /* chan 3 page register */
#define DMA4_PAGE       0x8F            /* chan 4 page register (unuseable) */
#define DMA5_PAGE       0x8B            /* chan 5 page register */
#define DMA6_PAGE       0x89            /* chan 6 page register */
#define DMA7_PAGE       0x8A            /* chan 7 page register */


typedef struct {
   unsigned char dma_disable;           /* bits to disable dma channel */
   unsigned char dma_enable;            /* bits to enable dma channel */
   unsigned short page;                 /* page port location */
   unsigned short addr;                 /* addr port location */
   unsigned short count;                /* count port location */
   unsigned short single;               /* single mode port location */
   unsigned short mode;                 /* mode port location */
   unsigned short clear_ff;             /* clear flip-flop port location */
   unsigned char write;                 /* bits for write transfer */
   unsigned char read;                  /* bits for read transfer */
} DMA_ENTRY;



static DMA_ENTRY mydma[] =
{
   /* channel 0 */
   { 0x04, 0x00, DMA0_PAGE, DMA0_ADDR, DMA0_CNT,
     DMA1_SNGL, DMA1_MODE, DMA1_CLRFF, 0x48, 0x44 },

   /* channel 1 */
   { 0x05, 0x01, DMA1_PAGE, DMA1_ADDR, DMA1_CNT,
     DMA1_SNGL,DMA1_MODE,DMA1_CLRFF,0x49,0x45 },

   /* channel 2 */
   { 0x06, 0x02, DMA2_PAGE, DMA2_ADDR, DMA2_CNT,
     DMA1_SNGL, DMA1_MODE, DMA1_CLRFF, 0x4A, 0x46 },

   /* channel 3 */
   { 0x07, 0x03, DMA3_PAGE, DMA3_ADDR, DMA3_CNT,
     DMA1_SNGL, DMA1_MODE, DMA1_CLRFF, 0x4B, 0x47 },

   /* channel 4 */
   { 0x04, 0x00, DMA4_PAGE, DMA4_ADDR, DMA4_CNT,
     DMA2_SNGL, DMA2_MODE, DMA2_CLRFF, 0x48, 0x44 },

   /* channel 5 */
   { 0x05, 0x01, DMA5_PAGE, DMA5_ADDR, DMA5_CNT,
     DMA2_SNGL, DMA2_MODE, DMA2_CLRFF, 0x49, 0x45 },

   /* channel 6 */
   { 0x06, 0x02, DMA6_PAGE, DMA6_ADDR, DMA6_CNT,
     DMA2_SNGL, DMA2_MODE, DMA2_CLRFF, 0x4A, 0x46 },

   /* channel 7 */
   { 0x07, 0x03, DMA7_PAGE, DMA7_ADDR, DMA7_CNT,
     DMA2_SNGL, DMA2_MODE, DMA2_CLRFF, 0x4B, 0x47 }
};



/* _dma_allocate_mem:
 *  Allocates the specified amount of conventional memory, ensuring that
 *  the returned block doesn't cross a page boundary. Sel will be set to
 *  the protected mode segment that should be used to free the block, and
 *  phys to the linear address of the block. On error, returns non-zero
 *  and sets sel and phys to 0.
 */
int _dma_allocate_mem(int bytes, int *sel, unsigned long *phys)
{
   int seg;

   /* allocate twice as much memory as we really need */
   seg = __dpmi_allocate_dos_memory((bytes*2 + 15) >> 4, sel);

   if (seg < 0) {
      *sel = 0;
      *phys = 0;
      return -1;
   }

   *phys = seg << 4;

   /* if it crosses a page boundary, use the second half of the block */
   if ((*phys>>16) != ((*phys+bytes)>>16))
      *phys += bytes;

   return 0;
}

END_OF_FUNCTION(_dma_allocate_mem);



/* dma_start:
 *  Starts the DMA controller for the specified channel, transferring
 *  size bytes from addr (the block must not cross a page boundary).
 *  If auto_init is set, it will use the endless repeat DMA mode.
 */
void _dma_start(int channel, unsigned long addr, int size, int auto_init, int input)
{
   DMA_ENTRY *tdma;
   unsigned long page, offset;
   int mode;

   tdma = &mydma[channel]; 
   page = addr >> 16;

   if (channel >= 4) {          /* 16 bit data is halved */
      addr >>= 1;
      size >>= 1;
   }

   offset = addr & 0xFFFF;
   size--;

   mode = (input) ? tdma->read : tdma->write;
   if (auto_init)
      mode |= 0x10;

   outportb(tdma->single, tdma->dma_disable);      /* disable channel */
   outportb(tdma->mode, mode);                     /* set mode */
   outportb(tdma->clear_ff, 0);                    /* clear flip-flop */
   outportb(tdma->addr, offset & 0xFF);            /* address LSB */
   outportb(tdma->addr, offset >> 8);              /* address MSB */
   outportb(tdma->page, page);                     /* page number */
   outportb(tdma->clear_ff, 0);                    /* clear flip-flop */
   outportb(tdma->count, size & 0xFF);             /* count LSB */
   outportb(tdma->count, size >> 8);               /* count MSB */
   outportb(tdma->single, tdma->dma_enable);       /* enable channel */
}

END_OF_FUNCTION(_dma_start);



/* dma_stop:
 *  Disables the specified DMA channel.
 */
void _dma_stop(int channel)
{
   DMA_ENTRY *tdma = &mydma[channel];
   outportb(tdma->single, tdma->dma_disable);
}

END_OF_FUNCTION(_dma_stop);



/* dma_todo:
 *  Returns the current position in a dma transfer. Interrupts should be
 *  disabled before calling this function.
 */
unsigned long _dma_todo(int channel)
{
   int val1, val2;
   DMA_ENTRY *tdma = &mydma[channel];

   outportb(tdma->clear_ff, 0xff);

   do {
      val1 = inportb(tdma->count);
      val1 |= inportb(tdma->count) << 8;
      val2 = inportb(tdma->count);
      val2 |= inportb(tdma->count) << 8;

      val1 -= val2;
   } while (val1 > 0x40);

   if (channel > 3)
      val2 <<= 1;

   return val2;
}

END_OF_FUNCTION(_dma_todo);



/* _dma_lock_mem:
 *  Locks the memory used by the dma routines.
 */
void _dma_lock_mem(void)
{
   LOCK_VARIABLE(mydma);
   LOCK_FUNCTION(_dma_allocate_mem);
   LOCK_FUNCTION(_dma_start);
   LOCK_FUNCTION(_dma_stop);
   LOCK_FUNCTION(_dma_todo);
}

