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
 *      Soundblaster driver. Supports DMA driven sample playback (mixing 
 *      up to eight samples at a time) and raw note output to the SB MIDI 
 *      port. The Adlib (FM synth) MIDI playing code is in adlib.c.
 *
 *      By Shawn Hargreaves.
 *
 *      Input routines added by Ove Kaaven.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* external interface to the digital SB driver */
static int sb_detect(int input);
static int sb_init(int input, int voices);
static void sb_exit(int input);
static int sb_set_mixer_volume(int volume);
static int sb_buffer_size(void);
static int sb_rec_cap_rate(int bits, int stereo);
static int sb_rec_cap_parm(int rate, int bits, int stereo);
static int sb_rec_source(int source);
static int sb_rec_start(int rate, int bits, int stereo);
static void sb_rec_stop(void);
static int sb_rec_read(void *buf);

static char sb_desc[256] = EMPTY_STRING;



#define SB_DRIVER_CONTENTS             \
   0, 0,                               \
   MIXER_MAX_SFX, MIXER_DEF_SFX,       \
   sb_detect,                          \
   sb_init,                            \
   sb_exit,                            \
   sb_set_mixer_volume,                \
   NULL,                               \
   NULL,                               \
   NULL,                               \
   sb_buffer_size,                     \
   _mixer_init_voice,                  \
   _mixer_release_voice,               \
   _mixer_start_voice,                 \
   _mixer_stop_voice,                  \
   _mixer_loop_voice,                  \
   _mixer_get_position,                \
   _mixer_set_position,                \
   _mixer_get_volume,                  \
   _mixer_set_volume,                  \
   _mixer_ramp_volume,                 \
   _mixer_stop_volume_ramp,            \
   _mixer_get_frequency,               \
   _mixer_set_frequency,               \
   _mixer_sweep_frequency,             \
   _mixer_stop_frequency_sweep,        \
   _mixer_get_pan,                     \
   _mixer_set_pan,                     \
   _mixer_sweep_pan,                   \
   _mixer_stop_pan_sweep,              \
   _mixer_set_echo,                    \
   _mixer_set_tremolo,                 \
   _mixer_set_vibrato,                 \
   0, 0,                               \
   sb_rec_cap_rate,                    \
   sb_rec_cap_parm,                    \
   sb_rec_source,                      \
   sb_rec_start,                       \
   sb_rec_stop,                        \
   sb_rec_read



DIGI_DRIVER digi_sb10 =
{
   DIGI_SB10,
   empty_string,
   empty_string,
   "Sound Blaster 1.0", 
   SB_DRIVER_CONTENTS
};



DIGI_DRIVER digi_sb15 =
{
   DIGI_SB15,
   empty_string,
   empty_string,
   "Sound Blaster 1.5", 
   SB_DRIVER_CONTENTS
};



DIGI_DRIVER digi_sb20 =
{
   DIGI_SB20,
   empty_string,
   empty_string,
   "Sound Blaster 2.0", 
   SB_DRIVER_CONTENTS
};



DIGI_DRIVER digi_sbpro =
{
   DIGI_SBPRO,
   empty_string,
   empty_string,
   "Sound Blaster Pro", 
   SB_DRIVER_CONTENTS
};



DIGI_DRIVER digi_sb16=
{
   DIGI_SB16,
   empty_string,
   empty_string,
   "Sound Blaster 16", 
   SB_DRIVER_CONTENTS
};



/* external interface to the SB midi output driver */
static int sb_midi_detect(int input);
static int sb_midi_init(int input, int voices);
static void sb_midi_exit(int input);
static void sb_midi_output(int data);

static char sb_midi_desc[256] = EMPTY_STRING;



MIDI_DRIVER midi_sb_out =
{
   MIDI_SB_OUT,
   empty_string,
   empty_string,
   "SB MIDI interface", 
   0, 0, 0xFFFF, 0, -1, -1,
   sb_midi_detect,
   sb_midi_init,
   sb_midi_exit,
   NULL,
   NULL,
   sb_midi_output,
   _dummy_load_patches,
   _dummy_adjust_patches,
   _dummy_key_on,
   _dummy_noop1,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop2,
   _dummy_noop2
};



static int sb_in_use = FALSE;             /* is SB being used? */
static int sb_stereo = FALSE;             /* in stereo mode? */
static int sb_recording = FALSE;          /* in input mode? */
static int sb_16bit = FALSE;              /* in 16 bit mode? */
static int sb_midi_out_mode = FALSE;      /* active for MIDI output? */
static int sb_midi_in_mode = FALSE;       /* active for MIDI input? */
static int sb_int = -1;                   /* interrupt vector */
static int sb_dsp_ver = -1;               /* SB DSP version */
static int sb_dma8 = -1;                  /* 8-bit DMA channel (SB16) */
static int sb_dma16 = -1;                 /* 16-bit DMA channel (SB16) */
static int sb_hw_dsp_ver = -1;            /* as reported by autodetect */
static int sb_dma_size = -1;              /* size of dma transfer in bytes */
static int sb_dma_mix_size = -1;          /* number of samples to mix */
static int sb_dma_count = 0;              /* need to resync with dma? */
static volatile int sb_semaphore = FALSE; /* reentrant interrupt? */

static int sb_sel[2];                     /* selectors for the buffers */
static unsigned long sb_buf[2];           /* pointers to the two buffers */
static int sb_bufnum = 0;                 /* the one currently in use */

static int sb_recbufnum = 0;              /* the one to be returned */

static int sb_master_vol = -1;            /* stored mixer settings */
static int sb_digi_vol = -1;
static int sb_fm_vol = -1;

static int sb_detecting_midi = FALSE;

static void sb_lock_mem(void);



/* sb_read_dsp:
 *  Reads a byte from the SB DSP chip. Returns -1 if it times out.
 */
static INLINE RET_VOLATILE int sb_read_dsp(void)
{
   int x;

   for (x=0; x<0xFFFF; x++)
      if (inportb(0x0E + _sound_port) & 0x80)
	 return inportb(0x0A+_sound_port);

   return -1; 
}



/* sb_write_dsp:
 *  Writes a byte to the SB DSP chip. Returns -1 if it times out.
 */
static INLINE RET_VOLATILE int sb_write_dsp(unsigned char byte)
{
   int x;

   for (x=0; x<0xFFFF; x++) {
      if (!(inportb(0x0C+_sound_port) & 0x80)) {
	 outportb(0x0C+_sound_port, byte);
	 return 0;
      }
   }
   return -1; 
}



/* _sb_voice:
 *  Turns the SB speaker on or off.
 */
void _sb_voice(int state)
{
   if (state) {
      sb_write_dsp(0xD1);

      if (sb_hw_dsp_ver >= 0x300) {          /* set up the mixer */

	 if (sb_master_vol < 0) {
	    outportb(_sound_port+4, 0x22);   /* store master volume */
	    sb_master_vol = inportb(_sound_port+5);
	 }

	 if (sb_digi_vol < 0) {
	    outportb(_sound_port+4, 4);      /* store DAC level */
	    sb_digi_vol = inportb(_sound_port+5);
	 }

	 if (sb_fm_vol < 0) {
	    outportb(_sound_port+4, 0x26);   /* store FM level */
	    sb_fm_vol = inportb(_sound_port+5);
	 }
      }
   }
   else {
      sb_write_dsp(0xD3);

      if (sb_hw_dsp_ver >= 0x300) {          /* reset previous mixer settings */

	 outportb(_sound_port+4, 0x22);      /* restore master volume */
	 outportb(_sound_port+5, sb_master_vol);

	 outportb(_sound_port+4, 4);         /* restore DAC level */
	 outportb(_sound_port+5, sb_digi_vol);

	 outportb(_sound_port+4, 0x26);      /* restore FM level */
	 outportb(_sound_port+5, sb_fm_vol);
      }
   }
}



/* _sb_set_mixer:
 *  Alters the SB-Pro hardware mixer.
 */
int _sb_set_mixer(int digi_volume, int midi_volume)
{
   if (sb_hw_dsp_ver < 0x300)
      return -1;

   if (digi_volume >= 0) {                   /* set DAC level */
      outportb(_sound_port+4, 4);
      outportb(_sound_port+5, (digi_volume & 0xF0) | (digi_volume >> 4));
   }

   if (midi_volume >= 0) {                   /* set FM level */
      outportb(_sound_port+4, 0x26);
      outportb(_sound_port+5, (midi_volume & 0xF0) | (midi_volume >> 4));
   }

   return 0;
}



/* sb_set_mixer_volume:
 *  Sets the SB mixer volume for playing digital samples.
 */
static int sb_set_mixer_volume(int volume)
{
   return _sb_set_mixer(volume, -1);
}



/* sb_stereo_mode:
 *  Enables or disables stereo output for SB-Pro.
 */
static void sb_stereo_mode(int enable)
{
   outportb(_sound_port+0x04, 0x0E); 
   outportb(_sound_port+0x05, (enable ? 2 : 0));
}



/* sb_input_stereo_mode:
 *  Enables or disables stereo input for SB-Pro.
 */
static void sb_input_stereo_mode(int enable)
{
   sb_write_dsp(enable ? 0xA8 : 0xA0);
}



/* sb_set_sample_rate:
 *  The parameter is the rate to set in Hz (samples per second).
 */
static void sb_set_sample_rate(unsigned int rate)
{
   if (sb_16bit) {
      sb_write_dsp(0x41);
      sb_write_dsp(rate >> 8);
      sb_write_dsp(rate & 0xff);
   }
   else {
      if (sb_stereo)
	 rate *= 2;

      sb_write_dsp(0x40);
      sb_write_dsp((unsigned char)(256-1000000/rate));
   }
}



/* sb_set_input_sample_rate:
 *  The parameter is the rate to set in Hz (samples per second).
 */
static void sb_set_input_sample_rate(unsigned int rate, int stereo)
{
   if (sb_16bit) {
      sb_write_dsp(0x42);
      sb_write_dsp(rate >> 8);
      sb_write_dsp(rate & 0xff);
   }
   else {
      if (stereo)
	 rate *= 2;

      sb_write_dsp(0x40);
      sb_write_dsp((unsigned char)(256-1000000/rate));
   }
}



/* _sb_reset_dsp:
 *  Resets the SB DSP chip, returning -1 on error.
 */
int _sb_reset_dsp(int data)
{
   int x;

   outportb(0x06+_sound_port, data);

   for (x=0; x<8; x++)
      inportb(0x06+_sound_port);

   outportb(0x06+_sound_port, 0);

   if (sb_read_dsp() != 0xAA)
      return -1;

   return 0;
}



/* _sb_read_dsp_version:
 *  Reads the version number of the SB DSP chip, returning -1 on error.
 */
int _sb_read_dsp_version(void)
{
   int x, y;

   if (sb_hw_dsp_ver > 0)
      return sb_hw_dsp_ver;

   if (_sound_port <= 0)
      _sound_port = 0x220;

   if (_sb_reset_dsp(1) != 0) {
      sb_hw_dsp_ver = -1;
   }
   else {
      sb_write_dsp(0xE1);
      x = sb_read_dsp();
      y = sb_read_dsp();
      sb_hw_dsp_ver = ((x << 8) | y);
   }

   return sb_hw_dsp_ver;
}



/* sb_buffer_size:
 *  Returns the current DMA buffer size, for use by the audiostream code.
 */
static int sb_buffer_size(void)
{
   return (sb_stereo ? sb_dma_mix_size/2 : sb_dma_mix_size);
}



/* sb_play_buffer:
 *  Starts a dma transfer of size bytes. On cards capable of it, the
 *  transfer will use auto-initialised dma, so there is no need to call
 *  this routine more than once. On older cards it must be called from
 *  the end-of-buffer handler to switch to the new buffer.
 */
static void sb_play_buffer(int size)
{
   if (sb_dsp_ver <= 0x200) {                /* 8 bit single-shot */
      sb_write_dsp(0x14);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
   }
   else if (sb_dsp_ver < 0x400) {            /* 8 bit auto-initialised */
      sb_write_dsp(0x48);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
      sb_write_dsp(0x90);
   }
   else {                                    /* 16 bit */
      size /= 2;
      sb_write_dsp(0xB6);
      sb_write_dsp(0x30);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
   }
}

END_OF_STATIC_FUNCTION(sb_play_buffer);



/* sb_record_buffer:
 *  Starts a dma transfer of size bytes. On cards capable of it, the
 *  transfer will use auto-initialised dma, so there is no need to call
 *  this routine more than once. On older cards it must be called from
 *  the end-of-buffer handler to switch to the new buffer.
 */
static void sb_record_buffer(int size, int stereo, int bits)
{
   if (sb_dsp_ver <= 0x200) {                /* 8 bit single-shot */
      sb_write_dsp(0x24);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
   }
   else if (sb_dsp_ver < 0x400) {            /* 8 bit auto-initialised */
      sb_write_dsp(0x48);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
      sb_write_dsp(0x98);
   }
   else if (bits <= 8) {                     /* 8 bit */
      sb_write_dsp(0xCE);
      sb_write_dsp((stereo) ? 0x20 : 0x00);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
   }
   else {                                    /* 16 bit */
      size /= 2;
      sb_write_dsp(0xBE);
      sb_write_dsp((stereo) ? 0x20 : 0x00);
      sb_write_dsp((size-1) & 0xFF);
      sb_write_dsp((size-1) >> 8);
   }
}

END_OF_STATIC_FUNCTION(sb_record_buffer);



/* sb_interrupt:
 *  The SB end-of-buffer interrupt handler. Swaps to the other buffer 
 *  if the card doesn't have auto-initialised dma, and then refills the
 *  buffer that just finished playing.
 */
static int sb_interrupt(void)
{
   unsigned char isr;

   if (sb_dsp_ver >= 0x400) {
      /* read SB16 ISR mask */
      outportb(_sound_port+4, 0x82);
      isr = inportb(_sound_port+5) & 7;

      if (isr & 4) {
	 /* MPU-401 interrupt */
	 _mpu_poll();
	 _eoi(_sound_irq);
	 return 0;
      }

      if (!(isr & 3)) {
	 /* unknown interrupt */
	 _eoi(_sound_irq);
	 return 0;
      }
   }

   if (sb_dsp_ver <= 0x200) {                /* not auto-initialised */
      _dma_start(_sound_dma, sb_buf[1-sb_bufnum], sb_dma_size, FALSE, FALSE);
      if (sb_recording)
	 sb_record_buffer(sb_dma_size, FALSE, 8);
      else
	 sb_play_buffer(sb_dma_size);
   }
   else {                                    /* poll dma position */
      sb_dma_count++;
      if (sb_dma_count > 16) {
	 sb_bufnum = (_dma_todo(_sound_dma) > (unsigned)sb_dma_size) ? 1 : 0;
	 sb_dma_count = 0;
      }
   }

   if ((!sb_semaphore) && (!sb_recording)) {
      sb_semaphore = TRUE;

      ENABLE();                              /* mix some more samples */
      _mix_some_samples(sb_buf[sb_bufnum], _dos_ds, TRUE);
      DISABLE();

      sb_semaphore = FALSE;
   } 

   sb_bufnum = 1 - sb_bufnum; 

   if ((sb_recording) && (digi_recorder)) {
      sb_semaphore = TRUE;

      ENABLE();                              /* sample input callback */
      digi_recorder();
      DISABLE();

      sb_semaphore = FALSE;
   }

   if (sb_16bit)                             /* acknowledge SB */
      inportb(_sound_port+0x0F);
   else
      inportb(_sound_port+0x0E);

   _eoi(_sound_irq);                         /* acknowledge interrupt */
   return 0;
}

END_OF_STATIC_FUNCTION(sb_interrupt);



/* sb_start:
 *  Starts up the sound output.
 */
static void sb_start(void)
{
   sb_bufnum = 0;

   _sb_voice(1);
   sb_set_sample_rate(_sound_freq);

   if ((sb_hw_dsp_ver >= 0x300) && (sb_dsp_ver < 0x400))
      sb_stereo_mode(sb_stereo);

   if (sb_dsp_ver <= 0x200)
      _dma_start(_sound_dma, sb_buf[0], sb_dma_size, FALSE, FALSE);
   else
      _dma_start(_sound_dma, sb_buf[0], sb_dma_size*2, TRUE, FALSE);

   sb_play_buffer(sb_dma_size);
}



/* sb_stop:
 *  Stops the sound output.
 */
static void sb_stop(void)
{
   /* halt sound output */
   _sb_voice(0);

   /* stop dma transfer */
   _dma_stop(_sound_dma);

   if (sb_dsp_ver <= 0x0200)
      sb_write_dsp(0xD0); 

   _sb_reset_dsp(1);
}



/* sb_detect:
 *  SB detection routine. Uses the BLASTER environment variable,
 *  or 'sensible' guesses if that doesn't exist.
 */
static int sb_detect(int input)
{
   char *blaster = getenv("BLASTER");
   char tmp[64], *msg;
   int cmask;
   int max_freq;
   int default_freq;

   if (!sb_detecting_midi) {
      /* input mode only works on the top of an existing output driver */
      if (input) {
	 if (digi_driver != digi_input_driver) {
	    ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("SB output driver must be installed before input can be read"));
	    return FALSE;
	 }
	 return TRUE;
      }

      /* what breed of SB are we looking for? */
      switch (digi_card) {

	 case DIGI_SB10:
	    sb_dsp_ver = 0x100;
	    break;

	 case DIGI_SB15:
	    sb_dsp_ver = 0x200;
	    break;

	 case DIGI_SB20:
	    sb_dsp_ver = 0x201;
	    break;

	 case DIGI_SBPRO:
	    sb_dsp_ver = 0x300;
	    break;

	 case DIGI_SB16:
	    sb_dsp_ver = 0x400;
	    break;

	 default:
	    sb_dsp_ver = -1;
	    break;
      } 
   }
   else
      sb_dsp_ver = -1;

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
		  sb_dma8 = strtol(blaster+1, NULL, 10);
		  break;

	       case 'h': case 'H':
		  sb_dma16 = strtol(blaster+1, NULL, 10);
		  break;
	    }

	    while ((*blaster) && (*blaster != ' ') && (*blaster != '\t'))
	       blaster++;
	 }
      }
   }

   if (_sound_port < 0)
      _sound_port = 0x220;

   /* make sure we got a good port address */
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
   _sb_read_dsp_version();
   if (sb_hw_dsp_ver < 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Sound Blaster not found"));
      return FALSE;
   }

   if (sb_dsp_ver < 0) {
      sb_dsp_ver = sb_hw_dsp_ver;
   }
   else {
      if (sb_dsp_ver > sb_hw_dsp_ver) {
	 sb_hw_dsp_ver = sb_dsp_ver = -1;
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Older SB version detected"));
	 return FALSE;
      }
   }

   if (sb_dsp_ver >= 0x400) {
      /* read configuration from SB16 card */
      if (_sound_irq < 0) {
	 outportb(_sound_port+4, 0x80);
	 cmask = inportb(_sound_port+5);
	 if (cmask&1) _sound_irq = 2; /* or 9? */
	 if (cmask&2) _sound_irq = 5;
	 if (cmask&4) _sound_irq = 7;
	 if (cmask&8) _sound_irq = 10;
      }
      if ((sb_dma8 < 0) || (sb_dma16 < 0)) {
	 outportb(_sound_port+4, 0x81);
	 cmask = inportb(_sound_port+5);
	 if (sb_dma8 < 0) {
	    if (cmask&1) sb_dma8 = 0;
	    if (cmask&2) sb_dma8 = 1;
	    if (cmask&8) sb_dma8 = 3;
	 }
	 if (sb_dma16 < 0) {
	    sb_dma16 = sb_dma8;
	    if (cmask&0x20) sb_dma16 = 5;
	    if (cmask&0x40) sb_dma16 = 6;
	    if (cmask&0x80) sb_dma16 = 7;
	 }
      }
   }

   /* if nothing else works */
   if (_sound_irq < 0)
      _sound_irq = 7;

   if (sb_dma8 < 0)
      sb_dma8 = 1;

   if (sb_dma16 < 0)
      sb_dma16 = 5;

   /* figure out the hardware interrupt number */
   sb_int = _map_irq(_sound_irq);

   if (!sb_detecting_midi) {
      /* what breed of SB? */
      if (sb_dsp_ver >= 0x400) {
	 msg = "SB 16";
	 max_freq = 45454;
	 default_freq = 22727;
      }
      else if (sb_dsp_ver >= 0x300) {
	 msg = "SB Pro";
	 max_freq = 22727;
	 default_freq = 22727;
      }
      else if (sb_dsp_ver >= 0x201) {
	 msg = "SB 2.0";
	 max_freq = 45454;
	 default_freq = 22727;
      }
      else if (sb_dsp_ver >= 0x200) {
	 msg = "SB 1.5";
	 max_freq = 16129;
	 default_freq = 16129;
      }
      else {
	 msg = "SB 1.0";
	 max_freq = 16129;
	 default_freq = 16129;
      }

      /* set up the playback frequency */
      if (_sound_freq <= 0)
	 _sound_freq = default_freq;

      if (_sound_freq < 15000) {
	 _sound_freq = 11906;
	 sb_dma_size = 128;
      }
      else if (MIN(_sound_freq, max_freq) < 20000) {
	 _sound_freq = 16129;
	 sb_dma_size = 128;
      }
      else if (MIN(_sound_freq, max_freq) < 40000) {
	 _sound_freq = 22727;
	 sb_dma_size = 256;
      }
      else {
	 _sound_freq = 45454;
	 sb_dma_size = 512;
      }

      if (sb_dsp_ver <= 0x200)
	 sb_dma_size *= 4;

      sb_dma_mix_size = sb_dma_size;

      /* can we handle 16 bit sound? */
      if (sb_dsp_ver >= 0x400) { 
	 if (_sound_dma < 0)
	    _sound_dma = sb_dma16;
	 else
	    sb_dma16 = _sound_dma;

	 sb_16bit = TRUE;
	 digi_driver->rec_cap_bits = 24;
	 sb_dma_size <<= 1;
      }
      else { 
	 if (_sound_dma < 0)
	    _sound_dma = sb_dma8;
	 else
	    sb_dma8 = _sound_dma;

	 sb_16bit = FALSE;
	 digi_driver->rec_cap_bits = 8;
      }

      /* can we handle stereo? */
      if (sb_dsp_ver >= 0x300) {
	 sb_stereo = TRUE;
	 digi_driver->rec_cap_stereo = TRUE;
	 sb_dma_size <<= 1;
	 sb_dma_mix_size <<= 1;
      }
      else {
	 sb_stereo = FALSE;
	 digi_driver->rec_cap_stereo = FALSE;
      }

      /* set up the card description */
      uszprintf(sb_desc, sizeof(sb_desc), get_config_text("%s (%d hz) on port %X, using IRQ %d and DMA channel %d"),
		uconvert_ascii(msg, tmp), _sound_freq, _sound_port, _sound_irq, _sound_dma);

      digi_driver->desc = sb_desc;
   }

   return TRUE;
}



/* sb_init:
 *  SB init routine: returns zero on success, -1 on failure.
 */
static int sb_init(int input, int voices)
{
   if (input)
      return 0;

   if (sb_in_use) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can't use SB MIDI interface and DSP at the same time"));
      return -1;
   }

   if (sb_dsp_ver <= 0x200) {       /* two conventional mem buffers */
      if ((_dma_allocate_mem(sb_dma_size, &sb_sel[0], &sb_buf[0]) != 0) ||
	  (_dma_allocate_mem(sb_dma_size, &sb_sel[1], &sb_buf[1]) != 0))
	 return -1;
   }
   else {                           /* auto-init dma, one big buffer */
      if (_dma_allocate_mem(sb_dma_size*2, &sb_sel[0], &sb_buf[0]) != 0)
	 return -1;

      sb_sel[1] = sb_sel[0];
      sb_buf[1] = sb_buf[0] + sb_dma_size;
   }

   sb_lock_mem();

   digi_driver->voices = voices;

   if (_mixer_init(sb_dma_mix_size, _sound_freq, sb_stereo, sb_16bit, &digi_driver->voices) != 0)
      return -1;

   _mix_some_samples(sb_buf[0], _dos_ds, TRUE);
   _mix_some_samples(sb_buf[1], _dos_ds, TRUE);

   _enable_irq(_sound_irq);
   _install_irq(sb_int, sb_interrupt);

   sb_start();

   sb_in_use = TRUE;
   return 0;
}



/* sb_exit:
 *  SB driver cleanup routine, removes ints, stops dma, frees buffers, etc.
 */
static void sb_exit(int input)
{
   if (input)
      return;

   sb_stop();
   _remove_irq(sb_int);
   _restore_irq(_sound_irq);

   __dpmi_free_dos_memory(sb_sel[0]);

   if (sb_sel[1] != sb_sel[0])
      __dpmi_free_dos_memory(sb_sel[1]);

   _mixer_exit();

   sb_hw_dsp_ver = sb_dsp_ver = -1;
   sb_in_use = FALSE;
}



/* sb_rec_cap_rate:
 *  Returns maximum input sampling rate.
 */
static int sb_rec_cap_rate(int bits, int stereo)
{
   if (sb_dsp_ver < 0)
      return 0;

   if (sb_dsp_ver >= 0x400)
      /* SB16 can handle 45kHz under all circumstances */
      return 45454;

   /* lesser SB cards can't handle 16-bit */
   if (bits != 8)
      return 0;

   if (sb_dsp_ver >= 0x300)
      /* SB Pro can handle 45kHz, but only half that in stereo */
      return (stereo) ? 22727 : 45454;

   /* lesser SB cards can't handle stereo */
   if (stereo)
      return 0;

   if (sb_dsp_ver >= 0x201)
      /* SB 2.0 supports 15kHz */
      return 15151;

   /* SB 1.x supports 13kHz */
   return 13157;
}



/* sb_rec_cap_parm:
 *  Returns whether the specified parameters can be set.
 */
static int sb_rec_cap_parm(int rate, int bits, int stereo)
{
   int c, r;

   if ((r = sb_rec_cap_rate(bits, stereo)) <= 0)
      return 0;

   if (r < rate)
      return -r;

   if (sb_dsp_ver >= 0x400) {
      /* if bits==8 and rate==_sound_freq, bidirectional is possible,
	 but that's not implemented yet */
      return 1;
   }

   if (stereo)
      rate *= 2;

   c = 1000000/rate;
   r = 1000000/c;
   if (r != rate)
      return -r;

   return 1;
}



/* sb_rec_source:
 *  Sets the sampling source for audio recording.
 */
static int sb_rec_source(int source)
{
   int v1, v2;

   if (sb_hw_dsp_ver >= 0x400) {
      /* SB16 */
      switch (source) {

	 case SOUND_INPUT_MIC:
	    v1 = 1;
	    v2 = 1;
	    break;

	 case SOUND_INPUT_LINE:
	    v1 = 16;
	    v2 = 8;
	    break;

	 case SOUND_INPUT_CD:
	    v1 = 4;
	    v2 = 2;
	    break;

	 default:
	    return -1;
      }

      outportb(_sound_port+4, 0x3D);
      outportb(_sound_port+5, v1);

      outportb(_sound_port+4, 0x3E);
      outportb(_sound_port+5, v2);

      return 0;
   }
   else if (sb_hw_dsp_ver >= 0x300) {
      /* SB Pro */
      outportb(_sound_port+4, 0xC);
      v1 = inportb(_sound_port+5);

      switch (source) {

	 case SOUND_INPUT_MIC:
	    v1 = (v1 & 0xF9);
	    break;

	 case SOUND_INPUT_LINE:
	    v1 = (v1 & 0xF9) | 6;
	    break;

	 case SOUND_INPUT_CD:
	    v1 = (v1 & 0xF9) | 2;
	    break;

	 default:
	    return -1;
      }

      outportb(_sound_port+4, 0xC);
      outportb(_sound_port+5, v1);

      return 0;
   }

   return -1;
}



/* sb_rec_start:
 *  Stops playback, switches the SB to A/D mode, and starts recording.
 *  Returns the DMA buffer size if successful.
 */
static int sb_rec_start(int rate, int bits, int stereo)
{
   if (sb_rec_cap_parm(rate, bits, stereo) <= 0)
      return 0;

   sb_stop();

   sb_16bit = (bits>8);
   _sound_dma = (sb_16bit) ? sb_dma16 : sb_dma8;
   sb_recording = TRUE;
   sb_recbufnum = sb_bufnum = 0;

   _sb_voice(1);
   sb_set_input_sample_rate(rate, stereo);

   if ((sb_hw_dsp_ver >= 0x300) && (sb_dsp_ver < 0x400))
      sb_input_stereo_mode(stereo);

   if (sb_dsp_ver <= 0x200)
      _dma_start(_sound_dma, sb_buf[0], sb_dma_size, FALSE, TRUE);
   else
      _dma_start(_sound_dma, sb_buf[0], sb_dma_size*2, TRUE, TRUE);

   sb_record_buffer(sb_dma_size, stereo, bits);

   return sb_dma_size;
}



/* sb_rec_stop:
 *  Stops recording, switches the SB back to D/A mode, and restarts playback.
 */
static void sb_rec_stop(void)
{
   if (!sb_recording)
      return;

   sb_stop();

   sb_recording = FALSE;
   sb_16bit = (sb_dsp_ver >= 0x400);
   _sound_dma = (sb_16bit) ? sb_dma16 : sb_dma8;

   _mix_some_samples(sb_buf[0], _dos_ds, TRUE);
   _mix_some_samples(sb_buf[1], _dos_ds, TRUE);

   sb_start();
}



/* sb_rec_read:
 *  Retrieves the just recorded DMA buffer, if there is one.
 */
static int sb_rec_read(void *buf)
{
   if (!sb_recording)
      return 0;

   if (sb_bufnum == sb_recbufnum)
      return 0;

   dosmemget(sb_buf[sb_recbufnum], sb_dma_size, buf);
   sb_recbufnum = 1-sb_recbufnum;

   return 1;
}

END_OF_STATIC_FUNCTION(sb_rec_read);



/* sb_midi_interrupt:
 *  Interrupt handler for the SB MIDI input.
 */
static int sb_midi_interrupt(void)
{
   int c = sb_read_dsp();

   if ((c >= 0) && (midi_recorder))
      midi_recorder(c);

   _eoi(_sound_irq);
   return 0;
}

END_OF_STATIC_FUNCTION(sb_midi_interrupt);



/* sb_midi_output:
 *  Writes a byte to the SB midi interface.
 */
static void sb_midi_output(int data)
{
   sb_write_dsp(data);
}

END_OF_STATIC_FUNCTION(sb_midi_output);



/* sb_midi_detect:
 *  Detection routine for the SB MIDI interface.
 */
static int sb_midi_detect(int input)
{
   int ret;

   if ((input) && (sb_midi_out_mode))
      return TRUE;

   sb_detecting_midi = TRUE;

   ret = sb_detect(FALSE);

   sb_detecting_midi = FALSE;

   return ret;
}



/* sb_midi_init:
 *  Initialises the SB midi interface, returning zero on success.
 */
static int sb_midi_init(int input, int voices)
{
   if ((sb_in_use) && (!sb_midi_out_mode)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can't use SB MIDI interface and DSP at the same time"));
      return -1;
   }

   sb_dsp_ver = -1;
   sb_lock_mem();

   uszprintf(sb_midi_desc, sizeof(sb_midi_desc), get_config_text("Sound Blaster MIDI interface on port %X"), _sound_port);
   midi_sb_out.desc = sb_midi_desc;

   if (input) {
      _enable_irq(_sound_irq);
      _install_irq(sb_int, sb_midi_interrupt);
      sb_midi_in_mode = TRUE;
   }
   else
      sb_midi_out_mode = TRUE;

   sb_write_dsp(0x35);

   sb_in_use = TRUE;
   return 0;
}



/* sb_midi_exit:
 *  Resets the SB midi interface when we are finished.
 */
static void sb_midi_exit(int input)
{
   if (input) {
      _remove_irq(sb_int);
      _restore_irq(_sound_irq);
      sb_midi_in_mode = FALSE;
   }
   else
      sb_midi_out_mode = FALSE;

   if ((!sb_midi_in_mode) && (!sb_midi_out_mode)) {
      _sb_reset_dsp(1);
      sb_in_use = FALSE;
   }
}



/* sb_lock_mem:
 *  Locks all the memory touched by parts of the SB code that are executed
 *  in an interrupt context.
 */
static void sb_lock_mem(void)
{
   extern void _mpu_poll_end(void);

   LOCK_VARIABLE(digi_sb10);
   LOCK_VARIABLE(digi_sb15);
   LOCK_VARIABLE(digi_sb20);
   LOCK_VARIABLE(digi_sbpro);
   LOCK_VARIABLE(digi_sb16);
   LOCK_VARIABLE(midi_sb_out);
   LOCK_VARIABLE(_sound_freq);
   LOCK_VARIABLE(_sound_port);
   LOCK_VARIABLE(_sound_dma);
   LOCK_VARIABLE(_sound_irq);
   LOCK_VARIABLE(sb_int);
   LOCK_VARIABLE(sb_in_use);
   LOCK_VARIABLE(sb_recording);
   LOCK_VARIABLE(sb_16bit);
   LOCK_VARIABLE(sb_midi_out_mode);
   LOCK_VARIABLE(sb_midi_in_mode);
   LOCK_VARIABLE(sb_dsp_ver);
   LOCK_VARIABLE(sb_hw_dsp_ver);
   LOCK_VARIABLE(sb_dma_size);
   LOCK_VARIABLE(sb_dma_mix_size);
   LOCK_VARIABLE(sb_sel);
   LOCK_VARIABLE(sb_buf);
   LOCK_VARIABLE(sb_bufnum);
   LOCK_VARIABLE(sb_recbufnum);
   LOCK_VARIABLE(sb_dma_count);
   LOCK_VARIABLE(sb_semaphore);
   LOCK_VARIABLE(sb_recording);
   LOCK_FUNCTION(sb_play_buffer);
   LOCK_FUNCTION(sb_interrupt);
   LOCK_FUNCTION(sb_rec_read);
   LOCK_FUNCTION(sb_midi_interrupt);
   LOCK_FUNCTION(sb_midi_output);
   LOCK_FUNCTION(sb_record_buffer);
   LOCK_FUNCTION(_mpu_poll);

   _dma_lock_mem();
}


