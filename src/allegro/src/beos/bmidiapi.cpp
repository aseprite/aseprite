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
 *      MIDI driver routines for BeOS.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif                


BMidiSynth *_be_midisynth = NULL;
static char be_midi_driver_desc[256] = EMPTY_STRING;
static int cur_patch[17];
static int cur_note[17];
static int cur_vol[17];



/* be_midi_detect:
 *  BeOS MIDI detection.
 */
extern "C" int be_midi_detect(int input)
{
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }
   
   return TRUE;
}



/* be_midi_init:
 *  Initializes the BeOS MIDI driver.
 */
extern "C" int be_midi_init(int input, int voices)
{
   char tmp[256], tmp2[128], tmp3[128] = EMPTY_STRING;
   char *sound = uconvert_ascii("sound", tmp);
   int mode, freq, quality, reverb;
   synth_mode sm = B_BIG_SYNTH;
   interpolation_mode im = B_2_POINT_INTERPOLATION;
   char *reverb_name[] =
      { "no", "closet", "garage", "ballroom", "cavern", "dungeon" };
   
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }
         
   _be_midisynth = new BMidiSynth();
   if (!_be_midisynth) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Not enough memory"));
      return -1;
   }
   
   /* Checks if instruments are available */
   mode = CLAMP(0, get_config_int(sound, uconvert_ascii("be_midi_quality", tmp), 1), 1);
   if (mode)
      sm = B_BIG_SYNTH;
   else
      sm = B_LITTLE_SYNTH;
   if ((be_synth->LoadSynthData(sm) != B_OK) ||
       (!be_synth->IsLoaded())) {
      delete _be_midisynth;
      _be_midisynth = NULL;
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not load MIDI instruments data file"));
      return -1;
   }
   
   /* Sets up synthetizer and loads instruments */
   _be_midisynth->EnableInput(true, true);
   
   /* Prevents other apps from changing instruments on the fly */
   _be_midisynth->FlushInstrumentCache(true);
   
   /* Reverberation is cool */
   reverb = CLAMP(0, get_config_int(sound, uconvert_ascii("be_midi_reverb", tmp), 0), 5);
   if (reverb) {
      be_synth->SetReverb((reverb_mode)reverb);
      be_synth->EnableReverb(true);
   }
   else
      be_synth->EnableReverb(false);
         
   /* Sets sampling rate and sample interpolation method */
   freq = get_config_int(sound, uconvert_ascii("be_midi_freq", tmp), 22050);
   quality = CLAMP(0, get_config_int(sound, uconvert_ascii("be_midi_interpolation", tmp), 1), 2);
   be_synth->SetSamplingRate(freq);
   switch (quality) {
      case 0:
         im = B_DROP_SAMPLE;
         break;
      case 1:
         im = B_2_POINT_INTERPOLATION;
         do_uconvert("fast", U_ASCII, tmp3, U_CURRENT, sizeof(tmp3));
         break;
      case 2:
         im = B_LINEAR_INTERPOLATION;
         do_uconvert("linear", U_ASCII, tmp3, U_CURRENT, sizeof(tmp3));
         break;
   }
   be_synth->SetInterpolation(im);
   
   /* Sets up driver description */
   uszprintf(be_midi_driver_desc, sizeof(be_midi_driver_desc),
             uconvert_ascii("BeOS %s quality synth, %s %d kHz, %s reverberation", tmp),
             uconvert_ascii(mode ? "high" : "low", tmp2), tmp3,
             (be_synth->SamplingRate() / 1000), reverb_name[reverb]);
   midi_beos.desc = be_midi_driver_desc;

   return 0;
}



/* be_midi_exit:
 *  Shuts down MIDI subsystem.
 */
extern "C" void be_midi_exit(int input)
{
   if (_be_midisynth) {
      _be_midisynth->AllNotesOff(false);
      delete _be_midisynth;
      _be_midisynth = NULL;
   }
}



/* be_midi_set_mixer_volume:
 *  Sets MIDI mixer output volume.
 */
extern "C" int be_midi_set_mixer_volume(int volume)
{
   _be_midisynth->SetVolume((double)volume / 255.0);
   return 0;
}



/* be_midi_get_mixer_volume:
 *  Returns MIDI mixer output volume.
 */
extern "C" int be_midi_get_mixer_volume(void)
{
   return (int)(_be_midisynth->Volume() * 255);
}



/* be_midi_key_on:
 *  Triggers a specified voice.
 */
extern "C" void be_midi_key_on(int inst, int note, int bend, int vol, int pan)
{
   int voice;
   
   if (inst > 127) {
      /* percussion */
      
      /* hack to use channel 10 only */
      midi_beos.xmin = midi_beos.xmax = -1;
      voice = _midi_allocate_voice(10, 10);
      midi_beos.xmin = midi_beos.xmax = 10;
      
      if (voice < 0)
         return;
      cur_note[10] = inst - 128;
      cur_vol[10] = vol;
      be_midi_set_pan(voice, pan);
      _be_midisynth->NoteOn(10, inst - 128, vol, B_NOW);
   }
   else {
      /* normal instrument */
      voice = _midi_allocate_voice(1, 16);
      if (voice < 0)
         return;
      if (inst != cur_patch[voice]) {
         _be_midisynth->ProgramChange(voice, inst);
         cur_patch[voice] = inst;
      }
 
      cur_note[voice] = note;
      cur_vol[voice] = vol;
      be_midi_set_pitch(voice, note, bend);
      be_midi_set_pan(voice, pan);
      _be_midisynth->NoteOn(voice, note, vol, B_NOW);
   }
}



/* be_midi_key_off:
 *  Turns off specified voice.
 */
extern "C" void be_midi_key_off(int voice)
{
   _be_midisynth->NoteOff(voice, cur_note[voice], cur_vol[voice], B_NOW);
}



/* be_midi_set_volume:
 *  Sets volume for a specified voice.
 */
extern "C" void be_midi_set_volume(int voice, int vol)
{
   /* This seems to work */
   _be_midisynth->ChannelPressure(voice, vol, B_NOW);
}



/* be_midi_set_pitch:
 *  Sets pitch of specified voice.
 */
extern "C" void be_midi_set_pitch(int voice, int note, int bend)
{
   /* ?? Is this correct? */
   _be_midisynth->PitchBend(voice, bend & 0x7F, bend >> 7, B_NOW);
}



/* be_midi_set_pan:
 *  Sets pan value on specified voice.
 */
extern "C" void be_midi_set_pan(int voice, int pan)
{
   _be_midisynth->ControlChange(voice, B_PAN, pan);
}
