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
 *      Windows Sound System driver.
 *      Supports DMA playback and sampling rates to up to 48khz.
 *
 *      The WSS CODEC is more common than you might think.
 *      Supported chips: AD1848, CS4231/A, CS4232 and compatibles.
 *
 *      By Antti Koskipaa.
 *
 *      See readme.txt for copyright information.
 *
 *      The WSS chips (all of them, clones too) have a bug which causes them
 *      to "swap" the LSB/MSB of each sample (or something like that) causing
 *      nothing but a loud fuzz from the speakers on "rare" occasions.
 *      An errata sheet on the CS4231 chip exists, but I haven't found it.
 *      If you fix this problem, email me and tell how...
 *
 *      Should I support recording? naah... =)
 *      AD1848 can't do full duplex, and this driver is meant to be AD1848 compatible.
 *      So, if you plan to support full duplex, check the chip revision first!
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* CODEC ports */
#define IADDR   (_sound_port+4)
#define IDATA   (_sound_port+5)
#define STATUS  (_sound_port+6)
#define PIO     (_sound_port+7)

/* IADDR bits */
#define INIT    0x80
#define MCE     0x40
#define TRD     0x20

/* STATUS bits */
#define INT     0x01
#define PRDY    0x02
#define PLR     0x04
#define PUL     0x08
#define SER     0x10
#define CRDY    0x20
#define CLR     0x40
#define CUL     0x80

/* IADDR registers */
#define LADC    0x00
#define RADC    0x01
#define LAUX1   0x02
#define RAUX1   0x03
#define LAUX2   0x04
#define RAUX2   0x05
#define LDAC    0x06
#define RDAC    0x07
#define FS      0x08
#define INTCON  0x09
#define PINCON  0x0A
#define ERRSTAT 0x0B
#define MODE_ID 0x0C
#define LOOPBCK 0x0D
#define PB_UCNT 0x0E
#define PB_LCNT 0x0F

#define NUMCODECRATES 16

/* DMA block size */
#define BLOCKLEN  512


#define WSSOUT(i, d)    outportb(IADDR, i); \
			outportb(IDATA, d);


static int wss_detect(int input);
static int wss_init(int input, int voices);
static void wss_exit(int input);
static int wss_set_mixer_volume(int volume);
static int wss_buffer_size(void);


static char wss_desc[256] = EMPTY_STRING;


DIGI_DRIVER digi_wss =
{
   DIGI_WINSOUNDSYS,
   empty_string,
   empty_string,
   "Windows Sound System",
   0, 0, MIXER_MAX_SFX, MIXER_DEF_SFX,
   wss_detect,
   wss_init,
   wss_exit,
   wss_set_mixer_volume,
   NULL,
   NULL,
   NULL,
   wss_buffer_size,
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,
   _mixer_get_position,
   _mixer_set_position,
   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,
   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,
   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,
   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato,
   0, 0,
   NULL, NULL, NULL, NULL, NULL, NULL
};


static int wss_usedrate = 0;
static int wss_stereo = TRUE;
static int wss_16bits = TRUE;
static int wss_dma_sel;
static unsigned long wss_dma_addr;
static int wss_dma_block;
static int wss_in_use = FALSE;
static int wss_detected = FALSE;


struct codec_rate_struct
{
   int freq;
   int divider;
};


/* List of supported rates */
static struct codec_rate_struct codec_rates[NUMCODECRATES] =
{
   { 5512,  0x01 },
   { 6620,  0x0F },
   { 8000,  0x00 },
   { 9600,  0x0E },
   { 11025, 0x03 },
   { 16000, 0x02 },
   { 18900, 0x05 },
   { 22050, 0x07 },
   { 27420, 0x04 },
   { 32000, 0x06 },
   { 33075, 0x0D },
   { 37800, 0x09 },
   { 44100, 0x0B },
   { 48000, 0x0C },
   { 54857, 0x08 },

   /* These last two values are ILLEGAL, { 64000, 0x0A }
      but they may work on some devices. Check 'em out! */
};



/* return size of audiostream buffers */
static int wss_buffer_size(void)
{
   return BLOCKLEN / (wss_stereo ? 2 : 1) / (wss_16bits ? 2 : 1);
}



/* WSS busy wait */
static void wss_wait(void)
{
   int i = 0xFFFF;

   /* Wait for INIT bit to clear */
   while ((inportb(_sound_port + 4) & INIT) || (i-- > 0))
      ;
}



/* Our IRQ handler. Kinda short... */
static int wss_irq_handler(void)
{
   /* Ack int */
   outportb(STATUS, 0);

   wss_dma_block = (_dma_todo(_sound_dma) > BLOCKLEN) ? 1 : 0;
   _mix_some_samples(wss_dma_addr + wss_dma_block * BLOCKLEN, _dos_ds, TRUE);

   _eoi(_sound_irq);
   return 0;
}

END_OF_STATIC_FUNCTION(wss_irq_handler);



static int wss_detect(int input)
{
   int i, diff, bestdiff, freq;
   char tmp[1024];

   if (input)
      return FALSE;

   if (wss_detected)
      return TRUE;

   if ((_sound_port < 0) || (_sound_irq < 0) || (_sound_dma < 0))
      return FALSE;

   /* Select best sampling rate */
   if (_sound_freq < 0)
      freq = 44100;
   else
      freq = _sound_freq;

   bestdiff = 10000000;

   for (i=0; i<NUMCODECRATES; i++) {
      diff = abs(freq - codec_rates[i].freq);
      if (diff < bestdiff) {
	 bestdiff = diff;
	 wss_usedrate = i;
      }
   }

   uszprintf(wss_desc, sizeof(wss_desc),
             uconvert_ascii("Windows Sound System (%d hz) on port %X, using IRQ %d and DMA channel %d", tmp),
	     codec_rates[wss_usedrate].freq, _sound_port, _sound_irq, _sound_dma);
   digi_wss.desc = wss_desc;

   wss_detected = TRUE;

   return TRUE;
}



static int wss_init(int input, int voices)
{
   int nsamples, i;

   if (input)
      return -1;                /* Input not supported */

   if (wss_in_use)
      return -1;

   if (!wss_detected)           /* Parameters not yet set? */
      wss_detect(0);

   if (!wss_detected)           /* If still no detection, fail */
      return -1;

   digi_wss.voices = voices;

   if (_dma_allocate_mem(BLOCKLEN * 2, &wss_dma_sel, &wss_dma_addr) != 0)
      return -1;

   nsamples = BLOCKLEN;
   if (wss_stereo)
      nsamples /= 2;

   if (_mixer_init(nsamples, codec_rates[wss_usedrate].freq, wss_stereo, wss_16bits, &digi_wss.voices) != 0)
      return -1;

   /* Codec wants the number of samples, not bytes */
   if (wss_16bits)
      nsamples /= 2;

   /* Begin CODEC initialization */
   WSSOUT(LADC, 0);             /* Lots of mutes... */
   WSSOUT(RADC, 0);
   WSSOUT(LAUX1, 0);
   WSSOUT(RAUX1, 0);
   WSSOUT(LAUX2, 0);
   WSSOUT(RAUX2, 0);
   WSSOUT(LDAC, 0x80);
   WSSOUT(RDAC, 0x80);
   WSSOUT(LOOPBCK, 0);
   WSSOUT(MODE_ID, 0x8A);

   /* Enable MCE */
   outportb(IADDR, MCE | INTCON);
   wss_wait();

   /* Enable full ACAL */
   outportb(IDATA, 0x18);
   wss_wait();

   /* Disable MCE */
   outportb(IADDR, ERRSTAT);

   /* Wait for ACAL to finish */
   i = 0xFFFF;

   while ((inportb(IDATA) & 0x20) && (i-- > 0))
      ;

   if (i < 1)
      return -1;

   /* Enter MCE */
   outportb(IADDR, MCE | FS);
   wss_wait();

   /* Set playback format */
   i = codec_rates[wss_usedrate].divider;
   if (wss_stereo)
      i |= 0x10;
   if (wss_16bits)
      i |= 0x40;
   outportb(IDATA, i);
   wss_wait();

   outportb(IADDR, 0);
   outportb(STATUS, 0);
   outportb(IADDR, PINCON);
   wss_wait();
   outportb(IDATA, 0x2);        /* Enable interrupts in Pin Control reg. */

   _enable_irq(_sound_irq);
   _install_irq(_map_irq(_sound_irq), wss_irq_handler);

   _mix_some_samples(wss_dma_addr, _dos_ds, TRUE);
   _mix_some_samples(wss_dma_addr + BLOCKLEN, _dos_ds, TRUE);

   _dma_start(_sound_dma, wss_dma_addr, BLOCKLEN * 2, TRUE, FALSE);

   LOCK_FUNCTION(wss_irq_handler);
   LOCK_VARIABLE(wss_dma_addr);
   LOCK_VARIABLE(wss_dma_block);
   LOCK_VARIABLE(_sound_dma);

   _eoi(_sound_irq);

   /* Set playback length */
   outportb(IADDR, PB_UCNT);
   wss_wait();
   outportb(IDATA, (nsamples - 1) >> 8);
   outportb(IADDR, PB_LCNT);
   wss_wait();
   outportb(IDATA, (nsamples - 1) & 0xFF);
   wss_wait();
   outportb(IADDR, INTCON | MCE);
   wss_wait();
   outportb(IDATA, 0x5);        /* PEN - Playback Enable! */
   outportb(IADDR, 0);

   /* Unmute outputs */
   WSSOUT(LDAC, 0);
   WSSOUT(RDAC, 0);

   wss_in_use = TRUE;
   return 0;
}



static void wss_exit(int input)
{
   if (!wss_in_use)
      return;

   /* Mute outputs */
   WSSOUT(LDAC, 0x80);
   WSSOUT(RDAC, 0x80);

   /* Stop playback */
   outportb(IADDR, MCE | INTCON);
   wss_wait();
   outportb(IDATA, 0);
   outportb(IADDR, 0);
   outportb(STATUS, 0);

   _remove_irq(_map_irq(_sound_irq));
   _restore_irq(_sound_irq);
   __dpmi_free_dos_memory(wss_dma_sel);
   _mixer_exit();
   wss_in_use = FALSE;
}



static int wss_set_mixer_volume(int volume)
{
   if (volume > 255)
      volume = 255;

   if (volume < 0)
      volume = 0;

   /* WSS has only attenuation regs, so we totally mute outputs to get
      silence w/volume 0 */

   DISABLE();

   if (!volume) {
      WSSOUT(RDAC, 0x80);
      WSSOUT(LDAC, 0x80);
   }
   else {
      WSSOUT(RDAC, 0x3F-volume/4);
      WSSOUT(LDAC, 0x3F-volume/4);
   }

   ENABLE();

   return 0;
}
