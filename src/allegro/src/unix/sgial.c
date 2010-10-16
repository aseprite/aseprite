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
 *      SGI AL sound driver.
 *
 *      By Lisa Parratt.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#if (defined ALLEGRO_WITH_SGIALDIGI) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <dmedia/audio.h>

#ifdef ALLEGRO_HAVE_LIBPTHREAD
#include <pthread.h>
#endif

#define _AL_SGIAL_PORTSIZE 12288
#ifdef ALLEGRO_HAVE_LIBPTHREAD
   #define _AL_SGIAL_BUFFERSIZE 2048
#else
   #define _AL_SGIAL_BUFFERSIZE 4096
#endif

static ALconfig _al_sgial_config;
static ALport _al_sgial_port;
static int _al_sgial_bufsize;
static unsigned char *_al_sgial_bufdata;
static int _al_sgial_signed;

static int _al_sgial_detect(int input);
static int _al_sgial_init(int input, int voices);
static void _al_sgial_exit(int input);
static int _al_sgial_mixer_volume(int volume);
static int _al_sgial_buffer_size(void);

static char _al_sgial_desc[256] = EMPTY_STRING;

#ifdef ALLEGRO_HAVE_LIBPTHREAD
static pthread_t thread;
#endif

DIGI_DRIVER digi_sgial =
{
   DIGI_SGIAL,
   empty_string,
   empty_string,
   "Silicon Graphics Audio",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   _al_sgial_detect,
   _al_sgial_init,
   _al_sgial_exit,
   _al_sgial_mixer_volume,
   NULL,

   NULL,
   NULL,
   _al_sgial_buffer_size,
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



/* _al_sgial_buffer_size:
 *  Returns the current DMA buffer size, for use by the audiostream code.
 */
static int _al_sgial_buffer_size(void)
{
   return _al_sgial_bufsize;
}



#ifdef ALLEGRO_HAVE_LIBPTHREAD

/* _al_sgial_puller_thread_func: [dedicated thread]
 *  We have threads, therefore we can use a thread to pull sound data
 *  as required.
 */
static void *_al_sgial_puller_thread_func(void *arg)
{
   int fd;
   fd_set fds;
   fd = alGetFD(_al_sgial_port);

   while (1) {
      alSetFillPoint(_al_sgial_port, _AL_SGIAL_PORTSIZE - _AL_SGIAL_BUFFERSIZE);
      FD_ZERO(&fds);
      FD_SET(fd, &fds);
      select(FD_SETSIZE, NULL, &fds, NULL, NULL);
      alWriteFrames(_al_sgial_port, _al_sgial_bufdata, _al_sgial_bufsize);
      _mix_some_samples((uintptr_t) _al_sgial_bufdata, 0, _al_sgial_signed);
   }
}

#else

/* _al_sgial_update: [SIGALRM callback]
 *  Updates data.
 */
static void _al_sgial_update(int threaded)
{
   if (alGetFillable(_al_sgial_port) > _al_sgial_bufsize) {
      alWriteFrames(_al_sgial_port, _al_sgial_bufdata, _al_sgial_bufsize);
      _mix_some_samples((uintptr_t) _al_sgial_bufdata, 0, _al_sgial_signed);
   }
}

#endif



/* _al_sgial_detect:
 *  Detects driver presence.
 */
static int _al_sgial_detect(int input)
{
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }

   /* A link error would have occured if the audio library was unavailable */
   return TRUE;
}



/* _al_sgial_init:
 *  SGI AL init routine.
 *  This is different from many systems. Whilst SGI AL supports simultaneous
 *  output, it is assumed that all streams will be at the same sample rate.
 *  This driver is well behaved - as such it does *not* honour requested sample
 *  rates, and always uses the system sample rate.
 */
static int _al_sgial_init(int input, int voices)
{
   char tmp1[128], tmp2[128];
   ALpv	pv;
   int _al_sgial_bits;
   int _al_sgial_stereo;
   int _al_sgial_rate;

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }

   /* SGI AL will down/up mix as appropriate. This is a crying waste of
    * Silicon Graphics audio hardware - in all likelihood it will be
    * upmixing to 24 bits.
    */
   _al_sgial_bits = (_sound_bits == 8) ? AL_SAMPLE_8: AL_SAMPLE_16;
   _al_sgial_stereo = (_sound_stereo) ? AL_STEREO : AL_MONO;
   _al_sgial_signed = 1;

   pv.param = AL_RATE;
   if (alGetParams(AL_DEFAULT_OUTPUT, &pv, 1) < 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not read SGI AL parameters"));
      return -1;
   }

   _al_sgial_rate = (int)alFixedToDouble(pv.value.ll);
   if (_al_sgial_rate < 0) {
      /* SGI AL couldn't tell us the sample rate: assume 44100 */
      _al_sgial_rate = 44100;
   }

   _al_sgial_config = alNewConfig();
   alSetSampFmt(_al_sgial_config, AL_SAMPFMT_TWOSCOMP);
   alSetQueueSize(_al_sgial_config, _AL_SGIAL_PORTSIZE);
   alSetWidth(_al_sgial_config, _al_sgial_bits);
   alSetChannels(_al_sgial_config, _al_sgial_stereo);

   _al_sgial_port = alOpenPort("Allegro", "w", _al_sgial_config);
   if (!_al_sgial_port) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not open SGI AL port"));
      return -1;
   }

   _al_sgial_bufsize = _AL_SGIAL_BUFFERSIZE;
   _al_sgial_bufdata = _AL_MALLOC_ATOMIC(_al_sgial_bufsize*((_al_sgial_bits==AL_SAMPLE_16) ? 2 : 1)*((_al_sgial_stereo==AL_STEREO) ? 2 : 1));
   if (!_al_sgial_bufdata) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not allocate audio buffer"));
      alClosePort(_al_sgial_port);
      alFreeConfig(_al_sgial_config);
      return -1;
   }

   digi_sgial.voices = voices;

   if (_mixer_init(_al_sgial_bufsize*((_al_sgial_stereo==AL_STEREO) ? 2 :1 ), _al_sgial_rate,
		   ((_al_sgial_stereo == AL_STEREO) ? 1 : 0), ((_al_sgial_bits == AL_SAMPLE_16) ? 1 : 0),
		   &digi_sgial.voices) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not init software mixer"));
      _AL_FREE(_al_sgial_bufdata);
      alClosePort(_al_sgial_port);
      alFreeConfig(_al_sgial_config);
      return -1;
   }

   _mix_some_samples((uintptr_t) _al_sgial_bufdata, 0, _al_sgial_signed);

#ifdef ALLEGRO_HAVE_LIBPTHREAD
   /* Add audio thread. */
   pthread_create(&thread, NULL, _al_sgial_puller_thread_func, NULL);
#else
   /* Add audio interrupt.  */
   _unix_bg_man->register_func(_al_sgial_update);
#endif

   uszprintf(_al_sgial_desc, sizeof(_al_sgial_desc), get_config_text("SGI AL: %d bits, %s, %d bps, %s"),
	     _al_sgial_bits,
	     uconvert_ascii((_al_sgial_signed ? "signed" : "unsigned"), tmp1), _al_sgial_rate,
	     uconvert_ascii((_al_sgial_stereo ? "stereo" : "mono"), tmp2));

   digi_driver->desc = _al_sgial_desc;

   return 0;
}



/* _al_sgial_exit:
 *  Shutdown SGI AL driver.
 */
static void _al_sgial_exit(int input)
{
   if (input)
      return;

#ifdef ALLEGRO_HAVE_LIBPTHREAD
   pthread_cancel(thread);
#else
   _unix_bg_man->unregister_func(_al_sgial_update);
#endif

   _AL_FREE(_al_sgial_bufdata);
   _al_sgial_bufdata = NULL;

   _mixer_exit();

   alClosePort(_al_sgial_port);
   alFreeConfig(_al_sgial_config);
}



/* _al_sgial_mixer_volume:
 *  Set mixer volume.
 */
static int _al_sgial_mixer_volume(int volume)
{
   return 0;
}



#ifdef ALLEGRO_MODULE

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   _unix_register_digi_driver(DIGI_SGIAL, &digi_sgial, TRUE, TRUE);
}

#endif

#endif

