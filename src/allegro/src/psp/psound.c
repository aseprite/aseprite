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
 *      PSP digital sound driver using the Allegro mixer.
 *      TODO: Audio input support.
 *
 *      By diedel.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintpsp.h"
#include <pspaudio.h>
#include <pspthreadman.h>

#ifndef ALLEGRO_PSP
#error something is wrong with the makefile
#endif


#define IS_SIGNED TRUE           /* The PSP DSP reads signed sampled data */
#define SAMPLES_PER_BUFFER 1024


static int psp_audio_channel_thread();
static int digi_psp_detect(int);
static int digi_psp_init(int, int);
static void digi_psp_exit(int);
static int digi_psp_buffer_size();
//static int digi_psp_set_mixer_volume(int);


static int psp_audio_on = FALSE;
static SceUID audio_thread_UID;
static int hw_channel;           /* The active PSP hardware output channel */
static short sound_buffer[2][SAMPLES_PER_BUFFER][2];
int curr_buffer=0;


DIGI_DRIVER digi_psp =
{
   DIGI_PSP,
   empty_string,
   empty_string,
   "PSP digital sound driver",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   digi_psp_detect,
   digi_psp_init,
   digi_psp_exit,
   NULL,     //digi_psp_set_mixer_volume,
   NULL,

   NULL,
   NULL,
   digi_psp_buffer_size,
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



/* psp_audio_channel_thread:
 *  This PSP thread manages the audio playing.
 */
static int psp_audio_channel_thread()
{
   while (psp_audio_on) {
      void *bufptr = &sound_buffer[curr_buffer];
      /* Asks to the Allegro mixer to fill the buffer */
      _mix_some_samples((uintptr_t)bufptr, 0, IS_SIGNED);
      /* Send mixed buffer to sound card */
      sceAudioOutputPannedBlocking(hw_channel, PSP_AUDIO_VOLUME_MAX,
         PSP_AUDIO_VOLUME_MAX, bufptr);
      curr_buffer = !curr_buffer;
   }

   sceKernelExitThread(0);

   return 0;
}



/* digi_psp_detect:
 *  Returns TRUE if the audio hardware is present.
 */
static int digi_psp_detect(int input)
{
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
         get_config_text("Input is not supported"));
      return FALSE;
   }

   return TRUE;
}



/* digi_psp_init:
 *  Initializes the PSP digital sound driver.
 */
static int digi_psp_init(int input, int voices)
{
   char digi_psp_desc[512] = EMPTY_STRING;
   char tmp1[256];

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
         get_config_text("Input is not supported"));
      return -1;
   }

   hw_channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, SAMPLES_PER_BUFFER,
      PSP_AUDIO_FORMAT_STEREO);
   if (hw_channel < 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
         get_config_text("Failed reserving hardware sound channel"));
      return -1;
   }

   psp_audio_on = TRUE;

   audio_thread_UID = sceKernelCreateThread("psp_audio_thread",
      (void *)&psp_audio_channel_thread, 0x19, 0x10000, 0, NULL);

   if (audio_thread_UID < 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
         get_config_text("Cannot create audio thread"));
      digi_psp_exit(FALSE);
      return -1;
   }

   if (sceKernelStartThread(audio_thread_UID, 0, NULL) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
         get_config_text("Cannot start audio thread"));
      digi_psp_exit(FALSE);
      return -1;
   }

   /* Allegro sound state variables */
   _sound_bits = 16;
   _sound_stereo = TRUE;
   _sound_freq = 44100;

   digi_psp.voices = voices;
   if (_mixer_init(SAMPLES_PER_BUFFER * 2, _sound_freq, _sound_stereo,
         (_sound_bits == 16), &digi_psp.voices))
   {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE,
         get_config_text("Error initializing mixer"));
      digi_psp_exit(FALSE);
      return -1;
   }

   uszprintf(digi_psp_desc, sizeof(digi_psp_desc),
      get_config_text("%d bits, %d bps, %s"), _sound_bits,
      _sound_freq, uconvert_ascii(_sound_stereo ? "stereo" : "mono", tmp1));

   digi_psp.desc = digi_psp_desc;

   return 0;
}


/* digi_psp_exit:
 *  Shuts down the sound driver.
 */
static void digi_psp_exit(int input)
{
   if (input)
      return;

   psp_audio_on = FALSE;
   sceKernelDeleteThread(audio_thread_UID);
   sceAudioChRelease(hw_channel);

   _mixer_exit();
}



static int digi_psp_buffer_size()
{
   return SAMPLES_PER_BUFFER;
}


/*
static int digi_set_mixer_volume(int volume)
{
   return sceAudioChangeChannelVolume(int channel, int leftvol, int rightvol);
}
*/

