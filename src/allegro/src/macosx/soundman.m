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
 *      MacOS X Sound Manager digital sound driver.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif


#define SAMPLES_PER_BUFFER     4096


static int osx_digi_sound_detect(int);
static int osx_digi_sound_init(int, int);
static void osx_digi_sound_exit(int);
static int osx_digi_sound_buffer_size();
static int osx_digi_sound_set_mixer_volume(int);


static SndChannel *sound_channel = NULL;
static ExtSoundHeader sound_header;
static unsigned char *sound_buffer[2] = { NULL, NULL };
static char sound_desc[256];


DIGI_DRIVER digi_sound_manager =
{
   DIGI_SOUND_MANAGER,
   empty_string,
   empty_string,
   "Sound Manager",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   osx_digi_sound_detect,
   osx_digi_sound_init,
   osx_digi_sound_exit,
   osx_digi_sound_set_mixer_volume,
   NULL,

   NULL,
   NULL,
   osx_digi_sound_buffer_size,
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



/* sound_callback:
 *  Sound Manager channel callback to update sound data.
 */
static void sound_callback(SndChannel *channel, SndCommand *command)
{
   SndCommand new_command;
   
   /* Send mixed buffer to sound card */
   sound_header.samplePtr = (Ptr)sound_buffer[command->param1];
   new_command.cmd = bufferCmd;
   new_command.param1 = 0;
   new_command.param2 = (long)&sound_header;
   if (SndDoCommand(channel, &new_command, FALSE) != noErr)
      return;
   
   /* Mix the other buffer */
   command->param1 ^= 1;
   _mix_some_samples((unsigned long)sound_buffer[command->param1], 0, (_sound_bits == 16) ? TRUE : FALSE);
   
   /* Reissue the callback */
   new_command.cmd = callBackCmd;
   new_command.param1 = command->param1;
   SndDoCommand(channel, &new_command, FALSE);
}



/* osx_digi_sound_detect.
 *  Returns TRUE if Sound Manager 3.x or newer is available.
 */
static int osx_digi_sound_detect(int input)
{
   NumVersion version;
   
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }
   version = SndSoundManagerVersion();
   if (version.majorRev < 3)
      return FALSE;
   return TRUE;
}



/* osx_digi_sound_init:
 *  Initializes the sound driver.
 */
static int osx_digi_sound_init(int input, int voices)
{
   SndCommand command;
   NumVersion version;
   long sound_caps = 0;
   char tmp[128];
   int i;

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }
   
   version = SndSoundManagerVersion();
   if (version.majorRev < 3) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Sound Manager version 3.0 or newer required"));
      return -1;
   }
   Gestalt(gestaltSoundAttr, &sound_caps);
   
   if (_sound_stereo >= 0) {
      if (_sound_stereo && (sound_caps & (1 << gestaltStereoCapability))) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Stereo output not supported"));
	 return -1;
      }
   }
   else
      _sound_stereo = (sound_caps & (1 << gestaltStereoCapability)) ? TRUE : FALSE;
   if (_sound_bits >= 0) {
      if (_sound_bits != ((sound_caps & (1 << gestalt16BitAudioSupport)) ? 16 : 8)) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("16 bit output not supported"));
	 return -1;
      }
   }
   else
      _sound_bits = (sound_caps & (1 << gestalt16BitAudioSupport)) ? 16 : 8;
   if (_sound_freq <= 0)
      _sound_freq = 44100;
   
   memset(&sound_header, 0, sizeof(sound_header));
   sound_header.numChannels = (_sound_stereo) ? 2 : 1;
   sound_header.sampleRate = _sound_freq << 16;
   sound_header.encode = extSH;
   sound_header.numFrames = SAMPLES_PER_BUFFER;
   sound_header.sampleSize = _sound_bits;
   
   for (i = 0; i < 2; i++) {
      sound_buffer[i] = (unsigned char *)calloc(1, SAMPLES_PER_BUFFER * (_sound_bits / 8) * (_sound_stereo ? 2 : 1));
      if (!sound_buffer[i]) {
         ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
         osx_digi_sound_exit(input);
	 return -1;
      }
   }
   
   digi_sound_manager.voices = voices;
   if (_mixer_init(SAMPLES_PER_BUFFER * (_sound_stereo ? 2 : 1), _sound_freq, _sound_stereo, ((_sound_bits == 16) ? TRUE: FALSE), &digi_sound_manager.voices)) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Error initializing mixer"));
      osx_digi_sound_exit(input);
      return -1;
   }
   
   sound_channel = (SndChannel *)malloc(sizeof(SndChannel));
   if (!sound_channel) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      osx_digi_sound_exit(input);
      return -1;
   }
   sound_channel->qLength = 128;
   
   if (SndNewChannel(&sound_channel, sampledSynth, (_sound_stereo ? initStereo : initMono), sound_callback) != noErr) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed creating sound channel"));
      free(sound_channel);
      return -1;
   }
   
   command.cmd = callBackCmd;
   command.param1 = 0;
   SndDoCommand(sound_channel, &command, FALSE);
   
   uszprintf(sound_desc, sizeof(sound_desc), get_config_text("Sound Manager version %d.%d, %d bits, %d bps, %s"),
      ((version.majorRev / 16) * 10) + (version.majorRev & 0xf),
      (version.minorAndBugRev / 16), _sound_bits, _sound_freq,
      uconvert_ascii(_sound_stereo ? "stereo" : "mono", tmp));
   digi_sound_manager.desc = sound_desc;
   
   return 0;
}



/* osx_digi_sound_exit:
 *  Shuts down the sound driver.
 */
static void osx_digi_sound_exit(int input)
{
   int i;
   
   if (input)
      return;
   
   if (sound_channel) {
      SndDisposeChannel(sound_channel, TRUE);
      free(sound_channel);
      sound_channel = NULL;
   }
   
   for (i = 0; i < 2; i++) {
      if (sound_buffer[i]) {
         free(sound_buffer[i]);
	 sound_buffer[i] = NULL;
      }
   }
   
   _mixer_exit();
}



/* osx_digi_sound_buffer_size:
 *  Returns the mixing buffer size, for use by the audiostream code.
 */
static int osx_digi_sound_buffer_size()
{
   return SAMPLES_PER_BUFFER;
}



/* osx_digi_sound_set_mixer_volume:
 *  Sets the sound channel volume.
 */
static int osx_digi_sound_set_mixer_volume(int volume)
{
   SndCommand command;
   
   if (!sound_channel)
      return -1;
   
   command.cmd = volumeCmd;
   command.param1 = 0;
   command.param2 = (volume << 16) | volume;
   return (SndDoCommand(sound_channel, &command, FALSE) != noErr);
}
