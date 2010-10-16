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
 *      ESS AudioDrive driver.
 *
 *      By Carsten Sorensen.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* external interface to the AudioDrive driver */
static int ess_detect(int input);
static int ess_init(int input, int voices);
static void ess_exit(int input);
static int ess_set_mixer_volume(int volume);
static int ess_buffer_size(void);

static char ess_desc[256] = EMPTY_STRING;


DIGI_DRIVER digi_audiodrive =
{
   DIGI_AUDIODRIVE,
   empty_string,
   empty_string,
   "ESS AudioDrive",
   0, 0, MIXER_MAX_SFX, MIXER_DEF_SFX,
   ess_detect,
   ess_init,
   ess_exit,
   ess_set_mixer_volume,
   NULL,
   NULL,
   NULL,
   ess_buffer_size,
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


static int ess_int = -1;                     /* interrupt vector */
static int ess_hw_ver = -1;                  /* as reported by autodetect */
static int ess_dma_size = -1;                /* size of dma transfer */
static int ess_dma_count = 0;                /* need to resync with dma? */
static volatile int ess_semaphore = FALSE;   /* reentrant interrupt? */

static int ess_sel;                          /* selector for the DMA buffer */
static unsigned long ess_buf[2];             /* pointers to the two buffers */
static int ess_bufnum = 0;                   /* the one currently in use */

static void ess_lock_mem(void);



/* ess_buffer_size:
 *  Returns the current DMA buffer size, for use by the audiostream code.
 */
static int ess_buffer_size(void)
{
   return ess_dma_size/4;     /* convert bytes to stereo 16 bit samples */
}



/* is_dsp_ready_for_read:
 *  Determines if DSP is ready to be read from.
 */
static INLINE RET_VOLATILE int is_dsp_ready_for_read(void)
{
   return (inportb(0x0E + _sound_port) & 0x80);
}



/* ess_read_dsp:
 *  Reads a byte from the DSP chip. Returns -1 if it times out.
 */
static INLINE RET_VOLATILE int ess_read_dsp(void)
{
   int x;

   for (x=0; x<0xffff; x++)
      if (inportb(0x0E + _sound_port) & 0x80)
	 return inportb(0x0A+_sound_port);

   return -1; 
}



/* ess_write_dsp:
 *  Writes a byte to the DSP chip. Returns -1 if it times out.
 */
static INLINE RET_VOLATILE int ess_write_dsp(unsigned char byte)
{
   int x;

   for (x=0; x<0xffff; x++) {
      if (!(inportb(0x0C+_sound_port) & 0x80)) {
	 outportb(0x0C+_sound_port, byte);
	 return 0;
      }
   }
   return -1; 
}



/* ess_set_mixer_volume:
 *  Sets the AudioDrive mixer volume for playing digital samples.
 */
static int ess_set_mixer_volume(int volume)
{
   return _sb_set_mixer(volume, -1);
}



/* ess_set_sample_rate:
 *  The parameter is the rate to set in Hz (samples per second).
 */
static void ess_set_sample_rate(unsigned int rate)
{
   int tc;
   int divider;

   if (rate > 22094)
      tc = 256 - 795500/rate;
   else
      tc = 128 - 397700/rate;

   rate = (rate*9)/20; 
   divider = 256 - 7160000/(rate*82);

   ess_write_dsp(0xA1);
   ess_write_dsp(tc);

   ess_write_dsp(0xA2);
   ess_write_dsp(divider);
}



/* ess_read_dsp_version:
 *  Reads the version number of the AudioDrive DSP chip, returning -1 on error.
 */
static int ess_read_dsp_version(void)
{
   if (ess_hw_ver > 0)
      return ess_hw_ver;

   if (_sound_port <= 0)
      _sound_port = 0x220;

   if (_sb_reset_dsp(1) != 0) {
      ess_hw_ver = -1;
   }
   else {
      int major=0, minor=0;
      int i;

      ess_write_dsp(0xE7);

      for (i=0; i<240; i++) {
	 if (is_dsp_ready_for_read()) {
	    if (!major)
	       major = ess_read_dsp();
	    else
	       minor = ess_read_dsp();
	 }
      }

      if ((major==0x68) && ((minor&0xF0)==0x80)) {
	 if ((minor&0x0F) >= 0xB)
	    ess_hw_ver = 0x1868;
	 else if ((minor&0x0F) >= 8)
	    ess_hw_ver = 0x1688;
	 else
	    ess_hw_ver = 0x0688;

	 return ess_hw_ver;
      }
   }

   return -1;
}



/* ess_play_buffer:
 *  Starts a dma transfer of size bytes.
 */
static void ess_play_buffer(int size)
{
   int value;

   ess_write_dsp(0xA4);
   ess_write_dsp((-size)&0xFF);
   ess_write_dsp(0xA5);
   ess_write_dsp((-size)>>8);

   ess_write_dsp(0xC0);
   ess_write_dsp(0xB8);
   value = ess_read_dsp() | 0x05;
   ess_write_dsp(0xB8);
   ess_write_dsp(value);
}

END_OF_STATIC_FUNCTION(ess_play_buffer);



/* ess_interrupt:
 *  The AudioDrive end-of-buffer interrupt handler.
 */
static int ess_interrupt(void)
{
   int value;

   ess_dma_count++;
   if (ess_dma_count > 16) {
      ess_bufnum = (_dma_todo(_sound_dma) > (unsigned)ess_dma_size) ? 1 : 0;
      ess_dma_count = 0;
   }

   if (!ess_semaphore) {
      ess_semaphore = TRUE;

      ENABLE();                              /* mix some more samples */
      _mix_some_samples(ess_buf[ess_bufnum], _dos_ds, FALSE);
      DISABLE();

      ess_semaphore = FALSE;
   }

   ess_bufnum = 1 - ess_bufnum;

   /* acknowlege AudioDrive */
   ess_write_dsp(0xA4);
   ess_write_dsp((-ess_dma_size)&0xFF);
   ess_write_dsp(0xA5);
   ess_write_dsp((-ess_dma_size)>>8);

   ess_write_dsp(0xC0);
   ess_write_dsp(0xB8);
   value = ess_read_dsp() | 0x05;
   ess_write_dsp(0xB8);
   ess_write_dsp(value);

   /* acknowledge interrupt */
   _eoi(_sound_irq);

   return 0;
}

END_OF_STATIC_FUNCTION(ess_interrupt);



/* ess_detect:
 *  AudioDrive detection routine. Uses the BLASTER environment variable,
 *  or 'sensible' gueses if that doesn't exist.
 */
static int ess_detect(int input)
{
   int orig_port = _sound_port;
   int orig_irq = _sound_irq;
   int orig_dma = _sound_dma;
   char *blaster = getenv("BLASTER");

   if (input)
      return FALSE;

   /* parse BLASTER env */
   if (blaster) {
      while (*blaster) {
	 while ((*blaster == ' ') || (*blaster == '\t'))
	    blaster++;

	 if (*blaster) {
	    switch (*blaster) {

	       case 'a': case 'A':
		  if (_sound_port < 0)
		     _sound_port = strtol(blaster+1, NULL, 16);
		  break;

	       case 'i': case 'I':
		  if (_sound_irq < 0)
		     _sound_irq = strtol(blaster+1, NULL, 10);
		  break;

	       case 'd': case 'D':
		  if (_sound_dma < 0)
		     _sound_dma = strtol(blaster+1, NULL, 10);
		  break;
	    }

	    while ((*blaster) && (*blaster != ' ') && (*blaster != '\t'))
	       blaster++;
	 }
      }
   }

   if (_sound_port < 0)
      _sound_port = 0x220;

   if (_sound_irq < 0)
      _sound_irq = 7;

   if (_sound_dma < 0)
      _sound_dma = 1;

   /* make sure we got a good port addres */
   if (_sb_reset_dsp(1) != 0) {
      static int bases[] = { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0 };
      int i;

      for (i=0; bases[i]; i++) {
	 _sound_port = bases[i];
	 if (_sb_reset_dsp(1) == 0)
	    break;
      }
   }

   /* check if the card really exists */
   ess_read_dsp_version();
   if (ess_hw_ver < 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("ESS AudioDrive not found"));
      _sound_port = orig_port;
      _sound_irq = orig_irq;
      _sound_dma = orig_dma;
      return FALSE;
   }

   /* figure out the hardware interrupt number */
   ess_int = _map_irq(_sound_irq);

   /* set up the playback frequency */
   if (_sound_freq <= 0) {
      _sound_freq = 22729;
      ess_dma_size = 1024;
   }
   else if (_sound_freq < 15000) {
      _sound_freq = 11363;
      ess_dma_size = 512;
   }
   else if (_sound_freq < 20000) {
      _sound_freq = 17046; 
      ess_dma_size = 512;
   }
   else if (_sound_freq < 40000) {
      _sound_freq = 22729;
      ess_dma_size = 1024;
   }
   else {
      _sound_freq = 44194;
      ess_dma_size = 2048;
   }

   /* set up the card description */
   uszprintf(ess_desc, sizeof(ess_desc), get_config_text("ES%X (%d hz) on port %X, using IRQ %d and DMA channel %d"),
	     ess_hw_ver, _sound_freq, _sound_port, _sound_irq, _sound_dma);

   digi_audiodrive.desc = ess_desc;

   return TRUE;
}



/* ess_init:
 *  AudioDrive init routine: returns zero on succes, -1 on failure.
 */
static int ess_init(int input, int voices)
{
   int value;

   if (_dma_allocate_mem(ess_dma_size*2, &ess_sel, &ess_buf[0]) != 0)
      return -1;

   ess_buf[1] = ess_buf[0] + ess_dma_size;

   ess_lock_mem();

   digi_audiodrive.voices = voices;

   if (_mixer_init(ess_dma_size/2, _sound_freq, TRUE, TRUE, &digi_audiodrive.voices) != 0)
      return -1;

   _mix_some_samples(ess_buf[0], _dos_ds, FALSE);
   _mix_some_samples(ess_buf[1], _dos_ds, FALSE);
   ess_bufnum = 0;

   _enable_irq(_sound_irq);
   _install_irq(ess_int, ess_interrupt);

   /* switch to AudioDrive extended mode */
   _sb_reset_dsp(0x03);
   ess_write_dsp(0xC6);

   ess_set_sample_rate(_sound_freq);

   ess_write_dsp(0xB8);
   ess_write_dsp(0x04);

   ess_write_dsp(0xC0);
   ess_write_dsp(0xA8);
   value = ess_read_dsp() & ~3;

   /* 16 bit stereo */
   value |= 0x01;
   ess_write_dsp(0xA8); 
   ess_write_dsp(value);
   ess_write_dsp(0xB6); 
   ess_write_dsp(0x00);
   ess_write_dsp(0xB7); 
   ess_write_dsp(0x71);
   ess_write_dsp(0xB7); 
   ess_write_dsp(0x9C); 

   /* demand mode (4 bytes/request) */
   ess_write_dsp(0xB9);
   ess_write_dsp(2); 

   ess_write_dsp(0xC0);
   ess_write_dsp(0xB1);
   value = (ess_read_dsp() & 0x0F) | 0x50;
   ess_write_dsp(0xB1);
   ess_write_dsp(value);

   ess_write_dsp(0xC0);
   ess_write_dsp(0xB2);
   value = (ess_read_dsp() & 0x0F) | 0x50;
   ess_write_dsp(0xB2);
   ess_write_dsp(value);

   _dma_start(_sound_dma, ess_buf[0], ess_dma_size*2, TRUE, FALSE);

   ess_play_buffer(ess_dma_size);

   rest(100);
   _sb_voice(1);

   return 0;
}



/* ess_exit:
 *  AudioDrive driver cleanup routine, removes ints, stops dma, 
 *  frees buffers, etc.
 */
static void ess_exit(int input)
{
   /* halt sound output */
   _sb_voice(0);

   /* stop dma transfer */
   _dma_stop(_sound_dma);

   _sb_reset_dsp(1);

   /* restore interrupts */
   _remove_irq(ess_int);

   /* reset PIC channels */
   _restore_irq(_sound_irq);

   /* free conventional memory buffer */
   __dpmi_free_dos_memory(ess_sel);

   _mixer_exit();

   ess_hw_ver = -1;
}



/* ess_lock_mem:
 *  Locks all the memory touched by parts of the AudiDrive code that are 
 *  executed in an interrupt context.
 */
static void ess_lock_mem(void)
{
   LOCK_VARIABLE(digi_audiodrive);
   LOCK_VARIABLE(_sound_freq);
   LOCK_VARIABLE(_sound_port);
   LOCK_VARIABLE(_sound_dma);
   LOCK_VARIABLE(_sound_irq);
   LOCK_VARIABLE(ess_int);
   LOCK_VARIABLE(ess_hw_ver);
   LOCK_VARIABLE(ess_dma_size);
   LOCK_VARIABLE(ess_sel);
   LOCK_VARIABLE(ess_buf);
   LOCK_VARIABLE(ess_bufnum);
   LOCK_VARIABLE(ess_dma_count);
   LOCK_VARIABLE(ess_semaphore);
   LOCK_FUNCTION(ess_play_buffer);
   LOCK_FUNCTION(ess_interrupt);

   _dma_lock_mem();
}


