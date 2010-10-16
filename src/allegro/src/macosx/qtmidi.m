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
 *      QuickTime MIDI driver routines for MacOS X.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintosx.h"

#ifndef ALLEGRO_MACOSX
#error something is wrong with the makefile
#endif                


typedef struct MIDI_VOICE
{
   NoteChannel channel;
   int note;
   int bend;
   int inst;
   int vol;
   int pan;
} MIDI_VOICE;


static NoteAllocator note_allocator = NULL;
static MIDI_VOICE voice[17];
static char driver_desc[256];


static int osx_midi_detect(int input);
static int osx_midi_init(int input, int voices);
static void osx_midi_exit(int input);
static int osx_midi_set_mixer_volume(int volume);
static void osx_midi_key_on(int inst, int note, int bend, int vol, int pan);
static void osx_midi_key_off(int voice);
static void osx_midi_set_volume(int voice, int vol);
static void osx_midi_set_pitch(int voice, int note, int bend);
static void osx_midi_set_pan(int voice, int pan);


MIDI_DRIVER midi_quicktime =
{
   MIDI_QUICKTIME,          /* driver ID code */
   empty_string,            /* driver name */
   empty_string,            /* description string */
   "QuickTime MIDI",        /* ASCII format name string */
   16,                      /* available voices */
   0,                       /* voice number offset */
   16,                      /* maximum voices we can support */
   0,                       /* default number of voices to use */
   10, 10,                  /* reserved voice range */
   osx_midi_detect,         /* AL_METHOD(int,  detect, (int input)); */
   osx_midi_init,           /* AL_METHOD(int,  init, (int input, int voices)); */
   osx_midi_exit,           /* AL_METHOD(void, exit, (int input)); */
   osx_midi_set_mixer_volume,  /* AL_METHOD(int,  set_mixer_volume, (int volume)); */
   NULL,                       /* AL_METHOD(int,  get_mixer_volume, (void)); */
   NULL,                    /* AL_METHOD(void, raw_midi, (int data)); */
   _dummy_load_patches,     /* AL_METHOD(int,  load_patches, (AL_CONST char *patches, AL_CONST char *drums)); */
   _dummy_adjust_patches,   /* AL_METHOD(void, adjust_patches, (AL_CONST char *patches, AL_CONST char *drums)); */
   osx_midi_key_on,         /* AL_METHOD(void, key_on, (int inst, int note, int bend, int vol, int pan)); */
   osx_midi_key_off,        /* AL_METHOD(void, key_off, (int voice)); */
   osx_midi_set_volume,     /* AL_METHOD(void, set_volume, (int voice, int vol)); */
   osx_midi_set_pitch,      /* AL_METHOD(void, set_pitch, (int voice, int note, int bend)); */
   osx_midi_set_pan,        /* AL_METHOD(void, set_pan, (int voice, int pan)); */
   _dummy_noop2,            /* AL_METHOD(void, set_vibrato, (int voice, int amount)); */
};



/* osx_midi_detect:
 *  QuickTime Music MIDI detection.
 */
static int osx_midi_detect(int input)
{
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }
   note_allocator = OpenDefaultComponent(kNoteAllocatorComponentType, 0);
   if (!note_allocator)
      return FALSE;
   CloseComponent(note_allocator);
   return TRUE;
}



/* osx_midi_init:
 *  Initializes the QuickTime Music MIDI driver.
 */
static int osx_midi_init(int input, int voices)
{
   NoteRequest note_request;
   ComponentResult result;
   char tmp[256];
   int i;
   
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }
   note_allocator = OpenDefaultComponent(kNoteAllocatorComponentType, 0);
   if(!note_allocator) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Cannot open the NoteAllocator QuickTime component"));
      return -1; 
   }
   memset(voice, 0, 17 * sizeof(MIDI_VOICE));
   for(i = 1; i <= 16; i++) {
      voice[i].note = -1;
      voice[i].bend = -1;
      voice[i].inst = -1;
      voice[i].vol = -1;
      voice[i].pan = -1;
      memset(&note_request, 0, sizeof(note_request));
      #if TARGET_RT_BIG_ENDIAN
	note_request.info.polyphony = 8;
	note_request.info.typicalPolyphony = 0x00010000;
      #else
      	note_request.info.polyphony.bigEndianValue = EndianU16_NtoB(8);
      	note_request.info.typicalPolyphony.bigEndianValue = EndianS32_NtoB(X2Fix(1.0));
      #endif
      result = NAStuffToneDescription(note_allocator, 1, &note_request.tone);
      result |= NANewNoteChannel(note_allocator, &note_request, &voice[i].channel);
      result |= NAResetNoteChannel(note_allocator, voice[i].channel);
      if ((result) || (!voice[i].channel)) {
          osx_midi_exit(input);
          ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed initializing MIDI channels"));
          return -1;
      }
   }
   
   uszprintf(driver_desc, sizeof(driver_desc),  uconvert_ascii("QuickTime Music MIDI synthesizer", tmp));
   midi_quicktime.desc = driver_desc;
   
   return 0;
}



/* osx_midi_exit:
 *  Shuts down QuickTime Music MIDI subsystem.
 */
static void osx_midi_exit(int input)
{
   int i;
   
   if (input)
      return;
   
   if (note_allocator) {
      for(i = 1; i <= 16; i++) {
         if (voice[i].channel) {
            NAPlayNote(note_allocator, voice[i].channel, voice[i].note, 0);
            NADisposeNoteChannel(note_allocator, voice[i].channel);
	    voice[i].channel = 0;
	 }
      }
      CloseComponent(note_allocator);
   }
}



/* osx_midi_set_mixer_volume:
 *  Sets QuickTime Music MIDI volume multiplier for all channels.
 */
static int osx_midi_set_mixer_volume(int volume)
{
   int i;
   
   for (i = 1; i <= 16; i++) {
      if (NASetNoteChannelVolume(note_allocator, voice[i].channel, volume << 8))
         return -1;
   }
   return 0;
}



/* osx_midi_key_on:
 *  Triggers a specified voice.
 */
static void osx_midi_key_on(int inst, int note, int bend, int vol, int pan)
{
   int voice_id;
   NoteChannel channel;
   
   if (inst > 127) {
      /* Percussion */
      note = inst - 128;
      inst = kFirstDrumkit + 1;
   }
   else
      inst = kFirstGMInstrument + inst;
   voice_id = _midi_allocate_voice(1, 16);
   if (voice_id < 0)
      return;
   channel = voice[voice_id].channel;
   if (voice[voice_id].inst != inst) {
      if (NASetInstrumentNumber(note_allocator, channel, inst) != noErr)
         return;
      voice[voice_id].inst = inst;
   }
   NAPlayNote(note_allocator, channel, voice[voice_id].note, 0);
   if (NAPlayNote(note_allocator, channel, note, vol) != noErr)
      return;
   voice[voice_id].note = note;
   osx_midi_set_pitch(voice_id, note, bend);
   osx_midi_set_pan(voice_id, pan);
}



/* osx_midi_key_off:
 *  Turns off specified voice.
 */
static void osx_midi_key_off(int voice_id)
{
   NAPlayNote(note_allocator, voice[voice_id].channel, voice[voice_id].note, 0);
}



/* osx_midi_set_volume:
 *  Sets volume for a specified voice.
 */
static void osx_midi_set_volume(int voice_id, int vol)
{
   if (voice[voice_id].vol != vol) {
      if (NASetController(note_allocator, voice[voice_id].channel, kControllerVolume, vol << 7) == noErr)
         voice[voice_id].vol = vol;
   }
}



/* osx_midi_set_pitch:
 *  Sets pitch of specified voice.
 */
static void osx_midi_set_pitch(int voice_id, int note, int bend)
{
   bend >>= 5;
   if (voice[voice_id].bend != bend) {
      if (NASetController(note_allocator, voice[voice_id].channel, kControllerPitchBend, bend) == noErr)
	 voice[voice_id].bend = bend;
   }
}



/* osx_midi_set_pan:
 *  Sets pan value on specified voice.
 */
static void osx_midi_set_pan(int voice_id, int pan)
{
   if (voice[voice_id].pan != pan) {
      if (NASetNoteChannelBalance(note_allocator, voice[voice_id].channel, pan - 127) == noErr)
	 voice[voice_id].pan = pan;
   }
}
