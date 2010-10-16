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
 *      Jack sound driver.
 *
 *      By Elias Pschernig.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#if (defined ALLEGRO_WITH_JACKDIGI) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

#ifndef SCAN_DEPEND
   #include <jack/jack.h>
#endif

/* This still uses Allegro's mixer, and mixes into an intermediate buffer, which
 * then is transferred to Jack. Another possibility would be to completely
 * circumvent Allegro's mixer and send each single voice to jack, letting Jack
 * take care of the mixing. I didn't care about some things in the Jack docs,
 * like the possibility of buffer sizes changing, or that no mutex_lock function
 * should be called (inside mix_some_samples).
 */
#define JACK_DEFAULT_BUFFER_SIZE -1
#define JACK_DEFAULT_CLIENT_NAME "allegro"
#define AMP16 ((sample_t) 32768)
#define AMP8 ((sample_t) 128)

#define PREFIX_I                "al-jack INFO: "
#define PREFIX_W                "al-jack WARNING: "
#define PREFIX_E                "al-jack ERROR: "

typedef jack_default_audio_sample_t sample_t;

static int jack_bufsize = JACK_DEFAULT_BUFFER_SIZE;
static char const *jack_client_name = JACK_DEFAULT_CLIENT_NAME;
static int jack_16bit;
static int jack_stereo;
static int jack_signed;
static jack_nframes_t jack_rate;
static char jack_desc[256] = EMPTY_STRING;
static jack_client_t *jack_client = NULL;
static jack_port_t *output_left, *output_right;
static void *jack_buffer;

static int jack_detect(int input);
static int jack_init(int input, int voices);
static void jack_exit(int input);
static int jack_buffer_size(void);
static int jack_set_mixer_volume(int volume);

DIGI_DRIVER digi_jack =
{
   DIGI_JACK,
   empty_string,
   empty_string,
   "JACK",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   jack_detect,
   jack_init,
   jack_exit,
   jack_set_mixer_volume,
   NULL,

   NULL,
   NULL,
   jack_buffer_size,
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



/* jack_buffer_size:
 *  Returns the current buffer size, for use by the audiostream code.
 */
static int jack_buffer_size(void)
{
   return jack_bufsize;
}



/* jack_process:
 *  The JACK processing functions.
 */
static int jack_process (jack_nframes_t nframes, void *arg)
{
   jack_nframes_t i;
   /* TODO: Should be uint16_t and uint8_t? Endianess? */
   unsigned short *buffer16 = jack_buffer;
   unsigned char *buffer8 = jack_buffer;
   jack_default_audio_sample_t *out_left;

   _mix_some_samples((uintptr_t) jack_buffer, 0, jack_signed);
   
   out_left = (jack_default_audio_sample_t *)
      jack_port_get_buffer (output_left, nframes);

   if (jack_stereo) {
      jack_default_audio_sample_t *out_right = (jack_default_audio_sample_t *)
	 jack_port_get_buffer (output_right, nframes);

      if (jack_16bit) {
	 for (i = 0; i < nframes; i++) {
	    out_left[i] = ((sample_t) buffer16[i * 2] - AMP16) / AMP16;
	    out_right[i] = ((sample_t) buffer16[i * 2 + 1] - AMP16) / AMP16;
	 }
      }
      else {
	 for (i = 0; i < nframes; i++) {
	    out_left[i] = ((sample_t) buffer8[i * 2] - AMP8) / (sample_t) AMP8;
	    out_right[i] = ((sample_t) buffer8[i * 2 + 1] - AMP8) / (sample_t) AMP8;
	 }
      }
   }
   else
   {
      if (jack_16bit) {
	 for (i = 0; i < nframes; i++) {
	    out_left[i] = ((sample_t) buffer16[i] - AMP16) / AMP16;
	 }
      }
      else {
	 for (i = 0; i < nframes; i++) {
	    out_left[i] = ((sample_t) buffer8[i] - AMP8) / AMP8;
	 }
      }
   }

   return 0;
}



/* jack_detect:
 *  Detects driver presence.
 */
static int jack_detect(int input)
{ 
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(
         "Input is not supported"));
      return FALSE;
   }

   if (!jack_client)
   {
      jack_client_name = get_config_string("sound", "jack_client_name",
	 jack_client_name); 
      jack_client = jack_client_new(jack_client_name);
      if (!jack_client)
	 return FALSE;
   }
   return TRUE;
}



/* jack_init:
 *  JACK init routine.
 */
static int jack_init(int input, int voices)
{
   const char **ports;
   char tmp[128];

   if (!jack_detect(input))
      return -1;

   jack_bufsize = get_config_int("sound", "jack_buffer_size",
      jack_bufsize);

   if (jack_bufsize == -1)
      jack_bufsize = jack_get_buffer_size (jack_client);

   /* Those are already read in from the config file by Allegro. */
   jack_16bit = (_sound_bits == 16 ? 1 : 0);
   jack_stereo = (_sound_stereo ? 1 : 0);

   /* Let Allegro mix in its native unsigned format. */
   jack_signed = 0;

   jack_set_process_callback (jack_client, jack_process, NULL);

   output_left = jack_port_register (jack_client, jack_stereo ? "left" : "mono",
      JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

   if (jack_stereo)
      output_right = jack_port_register (jack_client, "right",
         JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);

   jack_rate = jack_get_sample_rate (jack_client);

   jack_buffer = _AL_MALLOC_ATOMIC(jack_bufsize * (1 + jack_16bit) * (1 + jack_stereo));
   if (!jack_buffer) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(
         "Cannot allocate audio buffer"));
      jack_exit (input);
      return -1;
   }

   digi_jack.voices = voices;

   if (_mixer_init(jack_bufsize *  (1 + jack_stereo), jack_rate,
      jack_stereo, jack_16bit, &digi_jack.voices)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(
         "Cannot init software mixer"));
      jack_exit (input);
      return -1;
   }

   _mix_some_samples((uintptr_t) jack_buffer, 0, jack_signed);

   if (jack_activate (jack_client)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(
         "Cannot activate Jack client"));
      jack_exit (input);
      return 1;
   }

   /* Try to connect the ports. Failure to connect is not critical, since with
    * JACK, users may connect/disconnect ports anytime, without Allegro caring.
    */
   if ((ports = jack_get_ports (jack_client, NULL, NULL,
      JackPortIsPhysical|JackPortIsInput)) == NULL) {
      TRACE (PREFIX_I "Cannot find any physical playback ports");
   }

   if (ports) {
      if (ports[0]) {
	 if (jack_connect (jack_client, jack_port_name (output_left), ports[0]) == 0)
	    TRACE (PREFIX_I "Connected left playback port to %s", ports[0]);
      }
      if (jack_stereo && ports[1]) {
	 if (jack_connect (jack_client, jack_port_name (output_right), ports[1]) == 0)
	    TRACE (PREFIX_I "Connected right playback port to %s", ports[1]);
      }
      _AL_FREE (ports);
   }

   uszprintf(jack_desc, sizeof(jack_desc),
      get_config_text ("Jack, client '%s': %d bits, %s, %d bps, %s"),
      jack_client_name, jack_16bit ? 16 : 8,
      uconvert_ascii((jack_signed ? "signed" : "unsigned"), tmp),
      jack_rate, uconvert_ascii((jack_stereo ? "stereo" : "mono"), tmp));

   return 0;
}



/* jack_exit:
 *  Shuts down the JACK driver.
 */
static void jack_exit(int input)
{
   jack_client_close (jack_client);
   jack_client = NULL;
}



/* jack_set_mixer_volume:
 *  Set mixer volume (0-255)
 */
static int jack_set_mixer_volume(int volume)
{
   /* Not implemented */
   return 0;
}



#ifdef ALLEGRO_MODULE

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{ 
   _unix_register_digi_driver(DIGI_JACK, &digi_jack, TRUE, TRUE);
}

#endif

#endif
