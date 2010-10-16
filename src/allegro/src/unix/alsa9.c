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
 *      ALSA 0.9 sound driver.
 *
 *      By Thomas Fjellstrom.
 *
 *      Extensively modified by Elias Pschernig.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#if (ALLEGRO_ALSA_VERSION == 9) && (defined ALLEGRO_WITH_ALSADIGI) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#include "allegro/internal/aintern.h"
#ifdef ALLEGRO_QNX
#include "allegro/platform/aintqnx.h"
#else
#include "allegro/platform/aintunix.h"
#endif

#ifndef SCAN_DEPEND
   #include <string.h>
   #define ALSA_PCM_NEW_HW_PARAMS_API 1
   #include <alsa/asoundlib.h>
   #include <math.h>
#endif


#ifndef SND_PCM_FORMAT_S16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define SND_PCM_FORMAT_S16_NE SND_PCM_FORMAT_S16_BE
   #else
      #define SND_PCM_FORMAT_S16_NE SND_PCM_FORMAT_S16_LE
   #endif
#endif
#ifndef SND_PCM_FORMAT_U16_NE
   #ifdef ALLEGRO_BIG_ENDIAN
      #define SND_PCM_FORMAT_U16_NE SND_PCM_FORMAT_U16_BE
   #else
      #define SND_PCM_FORMAT_U16_NE SND_PCM_FORMAT_U16_LE
   #endif
#endif

#define ALSA9_CHECK(a) do { \
   int err = (a); \
   if (err<0) { \
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, "ALSA: %s : %s", #a, get_config_text(snd_strerror(err))); \
      goto Error; \
   } \
} while(0)


#define PREFIX_I                "al-alsa9 INFO: "
#define PREFIX_W                "al-alsa9 WARNING: "
#define PREFIX_E                "al-alsa9 ERROR: "

static char const *alsa_device = "default";
static char const *alsa_mixer_device = "default";
static snd_pcm_hw_params_t *hwparams = NULL;
static snd_pcm_sw_params_t *swparams = NULL;
static snd_output_t *snd_output = NULL;
static snd_pcm_uframes_t alsa_bufsize;
static snd_mixer_t *alsa_mixer = NULL;
static snd_mixer_elem_t *alsa_mixer_elem = NULL;
static long alsa_mixer_elem_min, alsa_mixer_elem_max;
static double alsa_mixer_allegro_ratio = 0.0;

#define ALSA_DEFAULT_BUFFER_MS  100
#define ALSA_DEFAULT_NUMFRAGS   5

static snd_pcm_t *pcm_handle;
static unsigned char *alsa_bufdata;
static int alsa_bits, alsa_signed, alsa_stereo;
static unsigned int alsa_rate;
static unsigned int alsa_fragments;
static int alsa_sample_size;

static struct pollfd *ufds = NULL;
static int pdc = 0;
static int poll_next;

static char alsa_desc[256] = EMPTY_STRING;

static int alsa_detect(int input);
static int alsa_init(int input, int voices);
static void alsa_exit(int input);
static int alsa_set_mixer_volume(int volume);
static int alsa_get_mixer_volume(void);
static int alsa_buffer_size(void);



DIGI_DRIVER digi_alsa =
{
   DIGI_ALSA,
   empty_string,
   empty_string,
   "ALSA",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   alsa_detect,
   alsa_init,
   alsa_exit,
   alsa_set_mixer_volume,
   alsa_get_mixer_volume,

   NULL,
   NULL,
   alsa_buffer_size,
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
   0,
   0,
   0,
   0,
   0,
   0
};



/* alsa_buffer_size:
 *  Returns the current DMA buffer size, for use by the audiostream code.
 */
static int alsa_buffer_size(void)
{
   return alsa_bufsize;
}

/* xrun_recovery:
 *  Underrun and suspend recovery
 */
static int xrun_recovery(snd_pcm_t *handle, int err)
{
   if (err == -EPIPE) {  /* under-run */
      err = snd_pcm_prepare(pcm_handle);
      if (err < 0)
	 fprintf(stderr, "Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
      return 0;
   }
   /* TODO: Can't wait here like that - we are inside an 'interrupt' after all. */
#if 0
   else if (err == -ESTRPIPE) {
      while ((err = snd_pcm_resume(pcm_handle)) == -EAGAIN)
	 sleep(1);  /* wait until the suspend flag is released */

      if (err < 0) {
	 err = snd_pcm_prepare(pcm_handle);
	 if (err < 0)
	    fprintf(stderr, "Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
      }
      return 0;
   }
#endif

   return err;
}



/* alsa_mix
 *  Mix and send some samples to ALSA.
 */
static void alsa_mix(void)
{
   int ret, samples = alsa_bufsize;
   unsigned char *ptr = alsa_bufdata;

   while (samples > 0) {
      ret = snd_pcm_writei(pcm_handle, ptr, samples);
      if (ret == -EAGAIN)
	 continue;

      if (ret < 0) {
	 if (xrun_recovery(pcm_handle, ret) < 0)
	    fprintf(stderr, "Write error: %s\n", snd_strerror(ret));
	 poll_next = 0;
	 break;  /* skip one period */
      }
      if (snd_pcm_state(pcm_handle) == SND_PCM_STATE_RUNNING)
	 poll_next = 1;
      samples -= ret;
      ptr += ret * alsa_sample_size;
   }

   _mix_some_samples((uintptr_t)alsa_bufdata, 0, alsa_signed);
}



/* alsa_update:
 *  Updates main buffer in case ALSA is ready.
 */
static void alsa_update(int threaded)
{
   unsigned short revents;

   if (poll_next) {
      poll(ufds, pdc, 0);
      snd_pcm_poll_descriptors_revents(pcm_handle, ufds, pdc, &revents);
      if (revents & POLLERR) {
	 if (snd_pcm_state(pcm_handle) == SND_PCM_STATE_XRUN ||
	    snd_pcm_state(pcm_handle) == SND_PCM_STATE_SUSPENDED) {
	    int err = snd_pcm_state(pcm_handle) == SND_PCM_STATE_XRUN ? -EPIPE : -ESTRPIPE;
	    if (xrun_recovery(pcm_handle, err) < 0) {
	       fprintf(stderr, "Write error: %s\n", snd_strerror(err));
	    }
	    poll_next = 0;
         }
	 else {
	    fprintf(stderr, "Wait for poll failed\n");
	 }
         return;
      }
      if (!(revents & POLLOUT))
	 return;
   }
   alsa_mix();
}



/* alsa_detect:
 *  Detects driver presence.
 */
static int alsa_detect(int input)
{
   int ret = FALSE;
   char tmp1[128], tmp2[128];

   alsa_device = get_config_string(uconvert_ascii("sound", tmp1),
				   uconvert_ascii("alsa_device", tmp2),
				   alsa_device);

   ret = snd_pcm_open(&pcm_handle, alsa_device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
   if (ret < 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not open card/pcm device"));
      return FALSE;
   }

   snd_pcm_close(pcm_handle);
   pcm_handle = NULL;
   return TRUE;
}



/* alsa_init:
 *  ALSA init routine.
 */
static int alsa_init(int input, int voices)
{
   int ret = 0;
   char tmp1[128], tmp2[128];
   int format = 0;
   unsigned int numfrags = 0;
   snd_pcm_uframes_t fragsize;

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }

   ALSA9_CHECK(snd_output_stdio_attach(&snd_output, stdout, 0));

   alsa_device = get_config_string(uconvert_ascii("sound", tmp1),
				   uconvert_ascii("alsa_device", tmp2),
				   alsa_device);

   alsa_mixer_device = get_config_string(uconvert_ascii("sound", tmp1),
				   uconvert_ascii("alsa_mixer_device", tmp2),
				   alsa_mixer_device);

   fragsize = get_config_int(uconvert_ascii("sound", tmp1),
			     uconvert_ascii("alsa_fragsize", tmp2), 0);

   numfrags = get_config_int(uconvert_ascii("sound", tmp1),
			     uconvert_ascii("alsa_numfrags", tmp2),
			     ALSA_DEFAULT_NUMFRAGS);

   ret = snd_pcm_open(&pcm_handle, alsa_device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
   if (ret < 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not open card/pcm device"));
      return -1;
   }

   snd_mixer_open(&alsa_mixer, 0);

   if (alsa_mixer
       && snd_mixer_attach(alsa_mixer, alsa_mixer_device) >= 0
       && snd_mixer_selem_register (alsa_mixer, NULL, NULL) >= 0
       && snd_mixer_load(alsa_mixer) >= 0) {
      const char *alsa_mixer_elem_name = get_config_string(uconvert_ascii("sound", tmp1),
							   uconvert_ascii("alsa_mixer_elem", tmp2),
							   "PCM");

      alsa_mixer_elem = snd_mixer_first_elem(alsa_mixer);

      while (alsa_mixer_elem) {
	 const char *name = snd_mixer_selem_get_name(alsa_mixer_elem);

	 if (strcasecmp(name, alsa_mixer_elem_name) == 0) {
	    snd_mixer_selem_get_playback_volume_range(alsa_mixer_elem, &alsa_mixer_elem_min, &alsa_mixer_elem_max);
	    alsa_mixer_allegro_ratio = (double) (alsa_mixer_elem_max - alsa_mixer_elem_min) / (double) 255;
	    break;
	 }

	 alsa_mixer_elem = snd_mixer_elem_next(alsa_mixer_elem);
      }
   }

   /* Set format variables. */
   alsa_bits = (_sound_bits == 8) ? 8 : 16;
   alsa_stereo = (_sound_stereo) ? 1 : 0;
   alsa_rate = (_sound_freq > 0) ? _sound_freq : 44100;

   snd_pcm_hw_params_malloc(&hwparams);
   ALSA9_CHECK(snd_pcm_hw_params_any(pcm_handle, hwparams));

   if (alsa_bits == 8) {
      if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_U8) == 0) {
         format = SND_PCM_FORMAT_U8;
         alsa_signed = 0;
      }
      else if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_S8) == 0) {
         format = SND_PCM_FORMAT_S8;
         alsa_signed = 1;
      }
      else {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported sample format"));
         goto Error;
      }
   }
   else if (alsa_bits == 16) {
      if (sizeof(short) != 2) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported sample format"));
         goto Error;
      }

      if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_U16_NE) == 0) {
         format = SND_PCM_FORMAT_U16_NE;
         alsa_signed = 0;
      }
      else if (snd_pcm_hw_params_test_format(pcm_handle, hwparams, SND_PCM_FORMAT_S16_NE) == 0) {
         format = SND_PCM_FORMAT_S16_NE;
         alsa_signed = 1;
      }
      else {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unsupported sample format"));
         goto Error;
      }
   }

   alsa_sample_size = (alsa_bits / 8) * (alsa_stereo ? 2 : 1);

   if (fragsize == 0) {
      unsigned int size = alsa_rate * ALSA_DEFAULT_BUFFER_MS / 1000 / numfrags;
      fragsize = 1;
      while (fragsize < size)
	 fragsize <<= 1;
   }

   ALSA9_CHECK(snd_pcm_hw_params_set_access(pcm_handle, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED));
   ALSA9_CHECK(snd_pcm_hw_params_set_format(pcm_handle, hwparams, format));
   ALSA9_CHECK(snd_pcm_hw_params_set_channels(pcm_handle, hwparams, alsa_stereo + 1));

   ALSA9_CHECK(snd_pcm_hw_params_set_rate_near(pcm_handle, hwparams, &alsa_rate, NULL));
   ALSA9_CHECK(snd_pcm_hw_params_set_period_size_near(pcm_handle, hwparams, &fragsize, NULL));
   ALSA9_CHECK(snd_pcm_hw_params_set_periods_near(pcm_handle, hwparams, &numfrags, NULL)); 

   ALSA9_CHECK(snd_pcm_hw_params(pcm_handle, hwparams));

   ALSA9_CHECK(snd_pcm_hw_params_get_period_size(hwparams, &alsa_bufsize, NULL));
   ALSA9_CHECK(snd_pcm_hw_params_get_periods(hwparams, &alsa_fragments, NULL));

   TRACE (PREFIX_I "alsa_bufsize = %ld, alsa_fragments = %d\n", alsa_bufsize, alsa_fragments);

   snd_pcm_sw_params_malloc(&swparams);
   ALSA9_CHECK(snd_pcm_sw_params_current(pcm_handle, swparams));
   ALSA9_CHECK(snd_pcm_sw_params_set_start_threshold(pcm_handle, swparams, alsa_bufsize));
   ALSA9_CHECK(snd_pcm_sw_params_set_avail_min(pcm_handle, swparams, fragsize));
   ALSA9_CHECK(snd_pcm_sw_params(pcm_handle, swparams));

   /* Allocate mixing buffer. */
   alsa_bufdata = _AL_MALLOC_ATOMIC(alsa_bufsize * alsa_sample_size);
   if (!alsa_bufdata) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not allocate audio buffer"));
      goto Error;
   }

   /* Initialise mixer. */
   digi_alsa.voices = voices;

   if (_mixer_init(alsa_bufsize * (alsa_stereo ? 2 : 1), alsa_rate,
		   alsa_stereo, ((alsa_bits == 16) ? 1 : 0),
		   &digi_alsa.voices) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not init software mixer"));
      goto Error;
   }

   snd_pcm_prepare(pcm_handle);
   pdc = snd_pcm_poll_descriptors_count (pcm_handle);
   if (pdc <= 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Invalid poll descriptors count"));
      goto Error;
   }

   ufds = _AL_MALLOC(sizeof(struct pollfd) * pdc);
   if (ufds == NULL) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory for poll descriptors"));
      goto Error;
   }
   ALSA9_CHECK(snd_pcm_poll_descriptors(pcm_handle, ufds, pdc));

   poll_next = 0;

   _mix_some_samples((uintptr_t) alsa_bufdata, 0, alsa_signed);

   /* Add audio interrupt. */
   _unix_bg_man->register_func(alsa_update);

   uszprintf(alsa_desc, sizeof(alsa_desc),
	     get_config_text
	     ("Alsa 0.9, Device '%s': %d bits, %s, %d bps, %s"),
	     alsa_device, alsa_bits,
	     uconvert_ascii((alsa_signed ? "signed" : "unsigned"), tmp1),
	     alsa_rate, uconvert_ascii((alsa_stereo ? "stereo" : "mono"), tmp2));

   digi_driver->desc = alsa_desc;
   return 0;

 Error:
   if (pcm_handle) {
      snd_pcm_close(pcm_handle);
      pcm_handle = NULL;
   }

   return -1;
}



/* alsa_exit:
 *  Shuts down ALSA driver.
 */
static void alsa_exit(int input)
{
   if (input)
      return;

   _unix_bg_man->unregister_func(alsa_update);

   _AL_FREE(alsa_bufdata);
   alsa_bufdata = NULL;

   _mixer_exit();

   if (alsa_mixer)
      snd_mixer_close(alsa_mixer);

   snd_pcm_close(pcm_handle);

   snd_pcm_hw_params_free(hwparams);
   snd_pcm_sw_params_free(swparams);
}



/* alsa_set_mixer_volume:
 *  Set mixer volume (0-255)
 */
static int alsa_set_mixer_volume(int volume)
{
   if (alsa_mixer && alsa_mixer_elem) {
      snd_mixer_selem_set_playback_volume(alsa_mixer_elem, 0, volume * alsa_mixer_allegro_ratio);
      snd_mixer_selem_set_playback_volume(alsa_mixer_elem, 1, volume * alsa_mixer_allegro_ratio);
   }
   return 0;
}



/* alsa_get_mixer_volume:
 *  Return mixer volume (0-255)
 */
static int alsa_get_mixer_volume(void)
{
   if (alsa_mixer && alsa_mixer_elem) {
      long vol1, vol2;

      snd_mixer_handle_events(alsa_mixer);

      if (snd_mixer_selem_get_playback_volume(alsa_mixer_elem, 0, &vol1) < 0)
	 return -1;
      if (snd_mixer_selem_get_playback_volume(alsa_mixer_elem, 1, &vol2) < 0)
         return -1;

      vol1 /= alsa_mixer_allegro_ratio;
      vol2 /= alsa_mixer_allegro_ratio;

      return (vol1 + vol2) / 2;
   }

   return -1;
}



#ifdef ALLEGRO_MODULE

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   _unix_register_digi_driver(DIGI_ALSA, &digi_alsa, TRUE, TRUE);
}

#endif

#endif
