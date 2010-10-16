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
 *      aRts sound driver (using artsc).
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#if (defined ALLEGRO_WITH_ARTSDIGI) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

#include <artsc.h>

#ifdef ALLEGRO_MODULE
int _module_has_registered_via_atexit = 0;
#endif

static int _al_arts_bits, _al_arts_rate, _al_arts_stereo;
#define _al_arts_signed (TRUE)

static arts_stream_t _al_arts_stream = NULL;
static int _al_arts_bufsize;
static int _al_arts_fragments;
static unsigned char *_al_arts_bufdata = NULL;

static int _al_arts_detect(int input);
static int _al_arts_init(int input, int voices);
static void _al_arts_exit(int input);
static int _al_arts_buffer_size(void);

static char _al_arts_desc[256] = EMPTY_STRING;

DIGI_DRIVER digi_arts =
{
   DIGI_ARTS,
   empty_string,
   empty_string,
   "aRts",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   _al_arts_detect,
   _al_arts_init,
   _al_arts_exit,
   NULL,
   NULL,

   NULL,
   NULL,
   _al_arts_buffer_size,
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



/* _al_arts_buffer_size:
 *  Returns the current DMA buffer size, for use by the audiostream code.
 */
static int _al_arts_buffer_size(void)
{
   return _al_arts_bufsize / (_al_arts_bits / 8) / (_al_arts_stereo ? 2 : 1);
}



/* _al_arts_update:
 *  Update data.
 */
static void _al_arts_update(int threaded)
{
   int i;

   for (i = 0; i < _al_arts_fragments; i++) {
      if (arts_write(_al_arts_stream, _al_arts_bufdata, _al_arts_bufsize)
	  < _al_arts_bufsize)
	 break;
      _mix_some_samples((uintptr_t) _al_arts_bufdata, 0, _al_arts_signed);
   }
}



#define copy_error_text(code, tmp)			\
ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,		\
	 uconvert_ascii(arts_error_text(code), (tmp)));



/* _al_arts_detect:
 *  Detect driver presence.
 */
static int _al_arts_detect(int input)
{
   char tmp[ALLEGRO_ERROR_SIZE];
   int code;

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
	       get_config_text("Input is not supported"));
      return FALSE;
   }

   code = arts_init();
   if (code != 0) {
      copy_error_text(code, tmp);
      return FALSE;
   }

   arts_free();
   return TRUE;
}



/* _al_arts_init:
 *  Init routine.
 */
static int _al_arts_init(int input, int voices)
{
   char tmp1[ALLEGRO_ERROR_SIZE];
   char tmp2[128];
   int code;

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
	       get_config_text("Input is not supported"));
      return -1;
   }

   /* Initialise artsc library.  */
   code = arts_init();
   if (code != 0) {
      copy_error_text(code, tmp1);
      return -1;
   }

#ifdef ALLEGRO_MODULE
   /* A side-effect of arts_init() is that it will register an
    * atexit handler.  See umodules.c for this problem.
    * ??? this seems to be the case only for recent versions.
    */
   _module_has_registered_via_atexit = 1;
#endif

   /* Make a copy of the global sound settings.  */
   _al_arts_bits = (_sound_bits == 8) ? 8 : 16;
   _al_arts_stereo = (_sound_stereo) ? 1 : 0;
   _al_arts_rate = (_sound_freq > 0) ? _sound_freq : 44100;

   /* Open a stream for playback.  */
   _al_arts_stream = arts_play_stream(_al_arts_rate, _al_arts_bits,
				      _al_arts_stereo ? 2 : 1,
				      "allegro");
   if (!_al_arts_stream) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
	       get_config_text("Can not open audio stream"));
      goto error;
   }

   /* Need non-blocking writes.  */
   code = arts_stream_set(_al_arts_stream, ARTS_P_BLOCKING, 0);
   if (code != 0) {
      copy_error_text(code, tmp1);
      goto error;
   }

   /* Try to reduce the latency of our stream.  */
   if (arts_stream_get(_al_arts_stream, ARTS_P_BUFFER_TIME) > 100)
      arts_stream_set(_al_arts_stream, ARTS_P_BUFFER_TIME, 100);

   /* Read buffer parameters and allocate buffer space.  */
   _al_arts_bufsize = arts_stream_get(_al_arts_stream, ARTS_P_PACKET_SIZE);
   _al_arts_fragments = arts_stream_get(_al_arts_stream, ARTS_P_PACKET_COUNT);
   _al_arts_bufdata = _AL_MALLOC_ATOMIC(_al_arts_bufsize);
   if (!_al_arts_bufdata) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
	       get_config_text("Can not allocate audio buffer"));
      goto error;
   }

   /* Start up mixer.  */
   digi_arts.voices = voices;

   if (_mixer_init(_al_arts_bufsize / (_al_arts_bits / 8), _al_arts_rate,
		   _al_arts_stereo, ((_al_arts_bits == 16) ? 1 : 0),
		   &digi_arts.voices) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
	       get_config_text("Can not init software mixer"));
      goto error;
   }

   _mix_some_samples((uintptr_t) _al_arts_bufdata, 0, _al_arts_signed);

   /* Add audio interrupt.  */
   _unix_bg_man->register_func(_al_arts_update);

   /* Decribe ourself.  */
   uszprintf(_al_arts_desc, sizeof(_al_arts_desc),
	     get_config_text("%s: %d bits, %s, %d bps, %s"),
	     "aRts", _al_arts_bits,
	     uconvert_ascii("signed", tmp1), _al_arts_rate,
	     uconvert_ascii((_al_arts_stereo ? "stereo" : "mono"), tmp2));
   digi_driver->desc = _al_arts_desc;

   return 0;

  error:

   if (_al_arts_bufdata) {
      _AL_FREE(_al_arts_bufdata);
      _al_arts_bufdata = NULL;
   }

   if (_al_arts_stream) {
      arts_close_stream(_al_arts_stream);
      _al_arts_stream = NULL;
   }

   arts_free();

   return -1;
}



/* _al_arts_exit:
 *  Shutdown routine.
 */
static void _al_arts_exit(int input)
{
   if (input)
      return;

   _unix_bg_man->unregister_func(_al_arts_update);

   _mixer_exit();

   _AL_FREE(_al_arts_bufdata);
   _al_arts_bufdata = NULL;

   /* Do not call the cleanup routines if we are being
    * called by the exit mechanism because they may have
    * already been called by it (see above).
    */
   if (!_allegro_in_exit) {
      arts_close_stream(_al_arts_stream);
      _al_arts_stream = NULL;

      arts_free();
   }
}



#ifdef ALLEGRO_MODULE

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   _unix_register_digi_driver(DIGI_ARTS, &digi_arts, TRUE, TRUE);
}

#endif

#endif

