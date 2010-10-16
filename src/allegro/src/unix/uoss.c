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
 *      Open Sound System driver. Supports for /dev/dsp and /dev/audio.
 *
 *      By Joshua Heyer.
 *
 *      Modified by Michael Bukin.
 * 
 *      Input code by Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#ifdef ALLEGRO_WITH_OSSDIGI

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#if defined(ALLEGRO_HAVE_SOUNDCARD_H)
   #include <soundcard.h>
#elif defined(ALLEGRO_HAVE_SYS_SOUNDCARD_H)
   #include <sys/soundcard.h>
#elif defined(ALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
   #include <machine/soundcard.h>
#elif defined(ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
   #include <linux/soundcard.h>
#endif
#include <sys/ioctl.h>


#ifndef AFMT_S16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define AFMT_S16_NE AFMT_S16_BE
   #else
      #define AFMT_S16_NE AFMT_S16_LE
   #endif
#endif
#ifndef AFMT_U16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define AFMT_U16_NE AFMT_U16_BE
   #else
      #define AFMT_U16_NE AFMT_U16_LE
   #endif
#endif


#define OSS_DEFAULT_FRAGBITS 9
#define OSS_DEFAULT_NUMFRAGS 8

int _oss_fragsize;
int _oss_numfrags;

char _oss_driver[256] = EMPTY_STRING;
static char _oss_mixer_driver[256] = EMPTY_STRING;

static int oss_fd;
static int oss_bufsize;
static unsigned char *oss_bufdata;
static int oss_signed, oss_format;

static int oss_save_bits, oss_save_stereo, oss_save_freq;
static int oss_rec_bufsize;

static int oss_detect(int input);
static int oss_init(int input, int voices);
static void oss_exit(int input);
static int oss_set_mixer_volume(int volume);
static int oss_get_mixer_volume(void);
static int oss_buffer_size(void);
static int oss_rec_cap_rate(int bits, int stereo);
static int oss_rec_cap_parm(int rate, int bits, int stereo);
static int oss_rec_source(int source);
static int oss_rec_start(int rate, int bits, int stereo);
static void oss_rec_stop(void);
static int oss_rec_read(void *buf);

static char oss_desc[256] = EMPTY_STRING;

DIGI_DRIVER digi_oss =
{
   DIGI_OSS,
   empty_string,
   empty_string,
   "Open Sound System",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   oss_detect,
   oss_init,
   oss_exit,
   oss_set_mixer_volume,
   oss_get_mixer_volume,

   NULL,
   NULL,
   oss_buffer_size,
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
   oss_rec_cap_rate,
   oss_rec_cap_parm,
   oss_rec_source,
   oss_rec_start,
   oss_rec_stop,
   oss_rec_read
};



/* oss_buffer_size:
 *  Returns the current DMA buffer size, for use by the audiostream code.
 */
static int oss_buffer_size(void)
{
   return oss_bufsize / (_sound_bits / 8) / (_sound_stereo ? 2 : 1);
}



/* oss_update:
 *  Update data.
 */
static void oss_update(int threaded)
{
   int i;
   audio_buf_info bufinfo;

   if (ioctl(oss_fd, SNDCTL_DSP_GETOSPACE, &bufinfo) != -1) {
      /* Write fragments.  */
      for (i = 0; i < bufinfo.fragments; i++) {
	 write(oss_fd, oss_bufdata, oss_bufsize);
	 _mix_some_samples((uintptr_t) oss_bufdata, 0, oss_signed);
      }
   }
}



/* open_oss_device:
 *  Shared helper for the detect and init functions, which opens the
 *  audio device and sets the sample mode parameters.
 */
static int open_oss_device(int input)
{
   char tmp1[128], tmp2[128], tmp3[128];
   int fragsize, fragbits, bits, stereo, freq;

   ustrzcpy(_oss_driver, sizeof(_oss_driver), get_config_string(uconvert_ascii("sound", tmp1),
					                        uconvert_ascii("oss_driver", tmp2),
					                        uconvert_ascii("/dev/dsp", tmp3)));

   ustrzcpy(_oss_mixer_driver, sizeof(_oss_mixer_driver), get_config_string(uconvert_ascii("sound", tmp1),
					                                    uconvert_ascii("oss_mixer_driver", tmp2),
					                                    uconvert_ascii("/dev/mixer", tmp3)));

   oss_fd = open(uconvert_toascii(_oss_driver, tmp1), (input ? O_RDONLY : O_WRONLY) | O_NONBLOCK);

   if (oss_fd < 0) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s: %s"), _oss_driver, ustrerror(errno));
      return -1;
   }

   _oss_fragsize = get_config_int(uconvert_ascii("sound", tmp1),
				  uconvert_ascii("oss_fragsize", tmp2),
				  -1);

   _oss_numfrags = get_config_int(uconvert_ascii("sound", tmp1),
				  uconvert_ascii("oss_numfrags", tmp2),
				  -1);

   if (_oss_fragsize < 0)
      _oss_fragsize = 1 << OSS_DEFAULT_FRAGBITS;

   if (_oss_numfrags < 0)
      _oss_numfrags = OSS_DEFAULT_NUMFRAGS;

   /* try to detect whether the DSP can do 16 bit sound or not */
   if (_sound_bits == -1) {
      /* ask supported formats */
      if (ioctl(oss_fd, SNDCTL_DSP_GETFMTS, &oss_format) != -1) {
         if (oss_format & AFMT_S16_NE) _sound_bits = 16;
         else if (oss_format & AFMT_U16_NE) _sound_bits = 16;
         else if (oss_format & AFMT_S8) _sound_bits = 8;
         else if (oss_format & AFMT_U8) _sound_bits = 8;
         else {
            /* ask current format */
            oss_format = 0;
            if (ioctl(oss_fd, SNDCTL_DSP_SETFMT, &oss_format) != -1) {
               switch (oss_format) {
                  case AFMT_S16_NE:
                  case AFMT_U16_NE:
                     _sound_bits = 16;
                     break;
                  case AFMT_S8:
                  case AFMT_U8:
                     _sound_bits = 8;
                     break;
               }
            }
         }
      }
   }

   bits = (_sound_bits == 8) ? 8 : 16;
   stereo = (_sound_stereo) ? 1 : 0;
   freq = (_sound_freq > 0) ? _sound_freq : 45454;

   /* fragment size is specified in samples, not in bytes */
   fragsize = _oss_fragsize * (bits / 8) * (stereo ? 2 : 1);
   fragsize += fragsize - 1;

   for (fragbits = 0; (fragbits < 16) && (fragsize > 1); fragbits++)
      fragsize /= 2;

   fragbits = CLAMP(4, fragbits, 16);
   _oss_numfrags = CLAMP(2, _oss_numfrags, 0x7FFF);

   fragsize = (_oss_numfrags << 16) | fragbits;

   if (ioctl(oss_fd, SNDCTL_DSP_SETFRAGMENT, &fragsize) == -1) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Setting fragment size: %s"), ustrerror(errno));
      close(oss_fd);
      return -1;
   }

   _oss_fragsize = (1 << (fragsize & 0xFFFF)) / (bits / 8) / (stereo ? 2 : 1);
   _oss_numfrags = fragsize >> 16;

   oss_format = ((bits == 16) ? AFMT_S16_NE : AFMT_U8);

   if ((ioctl(oss_fd, SNDCTL_DSP_SETFMT, &oss_format) == -1) || 
       (ioctl(oss_fd, SNDCTL_DSP_STEREO, &stereo) == -1) || 
       (ioctl(oss_fd, SNDCTL_DSP_SPEED, &freq) == -1)) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Setting DSP parameters: %s"), ustrerror(errno));
      close(oss_fd);
      return -1;
   }

   oss_signed = 0;

   switch (oss_format) {

      case AFMT_S8:
	 oss_signed = 1;
	 /* fallthrough */

      case AFMT_U8:
	 bits = 8;
	 break;

      case AFMT_S16_NE:
	 oss_signed = 1;
	 /* fallthrough */

      case AFMT_U16_NE:
	 bits = 16;
	 if (sizeof(short) != 2) {
	    ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported sample format"));
	    close(oss_fd);
	    return -1;
	 }
	 break;

      default:
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported sample format"));
	 close(oss_fd);
	 return -1;
   }

   if ((stereo != 0) && (stereo != 1)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not in stereo or mono mode"));
      close(oss_fd);
      return -1;
   }

   _sound_bits = bits;
   _sound_stereo = stereo;
   _sound_freq = freq;

   return 0;
}



/* oss_detect:
 *  Detect driver presence.
 */
static int oss_detect(int input)
{
   if (input) {
      if (digi_driver != digi_input_driver) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("OSS output driver must be installed before input can be read"));
	 return FALSE;
      }
      return TRUE;
   }

   if (open_oss_device(0) != 0)
      return FALSE;

   close(oss_fd);
   return TRUE;
}



/* oss_init:
 *  OSS init routine.
 */
static int oss_init(int input, int voices)
{
   char tmp1[128], tmp2[128];
   audio_buf_info bufinfo;

   if (input) {
      digi_driver->rec_cap_bits = 16;
      digi_driver->rec_cap_stereo = TRUE;
      return 0;
   }

   if (open_oss_device(0) != 0)
      return -1;

   if (ioctl(oss_fd, SNDCTL_DSP_GETOSPACE, &bufinfo) == -1) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Getting buffer size: %s"), ustrerror(errno));
      close(oss_fd);
      return -1;
   }

   oss_bufsize = bufinfo.fragsize;
   oss_bufdata = _AL_MALLOC_ATOMIC(oss_bufsize);

   if (oss_bufdata == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not allocate audio buffer"));
      close(oss_fd);
      return -1;
   }

   digi_oss.voices = voices;

   if (_mixer_init(oss_bufsize / (_sound_bits / 8), _sound_freq,
		   _sound_stereo, ((_sound_bits == 16) ? 1 : 0),
		   &digi_oss.voices) != 0) {

      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not init software mixer"));
      close(oss_fd);
      return -1;
   }

   _mix_some_samples((uintptr_t) oss_bufdata, 0, oss_signed);

   /* Add audio interrupt.  */
   _unix_bg_man->register_func(oss_update);

   uszprintf(oss_desc, sizeof(oss_desc), get_config_text("%s: %d bits, %s, %d bps, %s"),
		      _oss_driver, _sound_bits,
		      uconvert_ascii((oss_signed ? "signed" : "unsigned"), tmp1), _sound_freq,
		      uconvert_ascii((_sound_stereo ? "stereo" : "mono"), tmp2));

   digi_driver->desc = oss_desc;

   return 0;
}



/* oss_exit:
 *  Shutdown OSS driver.
 */
static void oss_exit(int input)
{
   if (input) {
      return;
   }

   _unix_bg_man->unregister_func(oss_update);

   _AL_FREE(oss_bufdata);
   oss_bufdata = 0;

   _mixer_exit();

   close(oss_fd);
}



/* oss_set_mixer_volume:
 *  Set mixer volume.
 */
static int oss_set_mixer_volume(int volume)
{
   int fd, vol, ret;
   char tmp[128];

   fd = open(uconvert_toascii(_oss_mixer_driver, tmp), O_WRONLY);
   if (fd < 0)
      return -1;

   vol = (volume * 100) / 255;
   vol = (vol << 8) | (vol);
   ret = ioctl(fd, MIXER_WRITE(SOUND_MIXER_PCM), &vol);
   close(fd);

   return ret;
}



/* oss_get_mixer_volume:
 *  Return mixer volume.
 */
static int oss_get_mixer_volume(void)
{
   int fd, vol;
   char tmp[128];

   fd = open(uconvert_toascii(_oss_mixer_driver, tmp), O_RDONLY);
   if (fd < 0)
      return -1;

   if (ioctl(fd, MIXER_READ(SOUND_MIXER_PCM), &vol) != 0)
      return -1;
   close(fd);

   vol = ((vol & 0xff) + (vol >> 8)) / 2;
   vol = vol * 255 / 100;
   return vol;
}



/* oss_rec_cap_rate:
 *  Returns maximum input sampling rate.
 */
static int oss_rec_cap_rate(int bits, int stereo)
{
   return 44100;
}



/* oss_rec_cap_parm:
 *  Returns whether the specified parameters can be set.
 */
static int oss_rec_cap_parm(int rate, int bits, int stereo)
{
   return 1;
}



/* oss_rec_source:
 *  Sets the sampling source for audio recording.
 */
static int oss_rec_source(int source)
{
   int fd, src, ret;
   char tmp[128];

   fd = open(uconvert_toascii(_oss_mixer_driver, tmp), O_WRONLY);
   if (fd < 0)
      return -1;

   switch (source) {
      case SOUND_INPUT_MIC: 
	 src = SOUND_MASK_MIC;
	 break;

      case SOUND_INPUT_LINE: 
	 src = SOUND_MASK_LINE;
	 break;

      case SOUND_INPUT_CD:
	 src = SOUND_MASK_CD;
	 break;

      default:
	 return -1;
   }

   ret = ioctl(fd, SOUND_MIXER_WRITE_RECSRC, &src);
   close(fd);

   return ret;
}



/* oss_rec_start:
 *  Re-opens device with read-mode and starts recording (half-duplex).
 *  Returns the DMA buffer size if successful.
 */

static int oss_rec_start(int rate, int bits, int stereo)
{
   audio_buf_info bufinfo;

   /* Save current settings, and close playback device.  */
   oss_save_bits = _sound_bits;
   oss_save_stereo = _sound_stereo;
   oss_save_freq = _sound_freq;

   _unix_bg_man->unregister_func(oss_update);

   close(oss_fd);

   /* Reopen device for recording.  */
   _sound_bits = bits;
   _sound_stereo = stereo;
   _sound_freq = rate;

   if (open_oss_device(1) != 0)
      return 0;

   if (ioctl(oss_fd, SNDCTL_DSP_GETISPACE, &bufinfo) == -1) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Getting buffer size: %s"), ustrerror(errno));
      close(oss_fd);
      return 0;
   }

   oss_rec_bufsize = bufinfo.fragsize;
   return oss_rec_bufsize;
}



/* oss_rec_stop:
 *  Stops recording and switches the device back to the original mode.
 */
static void oss_rec_stop(void)
{
   close(oss_fd);

   /* Reopen for playback with saved settings.  */
   _sound_bits = oss_save_bits;
   _sound_stereo = oss_save_stereo;
   _sound_freq = oss_save_freq;

   open_oss_device(0);

   _unix_bg_man->register_func(oss_update);
}



/* oss_rec_read:
 *  Retrieves the just recorded buffer, if there is one.
 */
static int oss_rec_read(void *buf)
{
   char *p;
   int i;

   if (read(oss_fd, buf, oss_rec_bufsize) != oss_rec_bufsize)
      return 0;

   /* Convert signedness.  */
   if ((_sound_bits == 16) && (oss_signed)) {
      p = buf;
      for (i = 0; i < oss_rec_bufsize; i++)
	 p[i] ^= 0x80;
   }

   return 1;
}


#endif
