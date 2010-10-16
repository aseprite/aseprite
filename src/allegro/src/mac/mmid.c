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
 *      MIDI driver routines for MacOS.
 *
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintmac.h"
#include <QuickTimeMusic.h>

#ifndef ALLEGRO_MPW
#error something is wrong with the makefile
#endif                

static NoteAllocator qtna=0;
static struct{
NoteChannel channel;
int note;
int inst;
int vol;
int pan;
}macmidivoices[17];

static int mac_midi_detect(int input);
static int mac_midi_init(int input, int voices);
static void mac_midi_exit(int input);
static int mac_midi_set_mixer_volume(int volume);
static void mac_midi_key_on(int inst, int note, int bend, int vol, int pan);
static void mac_midi_key_off(int voice);
static void mac_midi_set_volume(int voice, int vol);
static void mac_midi_set_pitch(int voice, int note, int bend);
static void mac_midi_set_pan(int voice, int pan);

MIDI_DRIVER midi_quicktime =
{
   MIDI_QUICKTIME,          /* driver ID code */
   empty_string,            /* driver name */
   empty_string,            /* description string */
   "QuickTime Midi",        /* ASCII format name string */
   16,                      /* available voices */
   0,                       /* voice number offset */
   16,                      /* maximum voices we can support */
   0,                       /* default number of voices to use */
   10, 10,                  /* reserved voice range */
   mac_midi_detect,          /* AL_METHOD(int,  detect, (int input)); */
   mac_midi_init,            /* AL_METHOD(int,  init, (int input, int voices)); */
   mac_midi_exit,            /* AL_METHOD(void, exit, (int input)); */
   mac_midi_set_mixer_volume,/* AL_METHOD(int,  mixer_set_volume, (int volume)); */
   NULL,                     /* AL_METHOD(int,  mixer_get_volume, (void)); */
   NULL,                    /* AL_METHOD(void, raw_midi, (int data)); */
   _dummy_load_patches,     /* AL_METHOD(int,  load_patches, (AL_CONST char *patches, AL_CONST char *drums)); */
   _dummy_adjust_patches,   /* AL_METHOD(void, adjust_patches, (AL_CONST char *patches, AL_CONST char *drums)); */
   mac_midi_key_on,          /* AL_METHOD(void, key_on, (int inst, int note, int bend, int vol, int pan)); */
   mac_midi_key_off,         /* AL_METHOD(void, key_off, (int voice)); */
   mac_midi_set_volume,      /* AL_METHOD(void, set_volume, (int voice, int vol)); */
   mac_midi_set_pitch,       /* AL_METHOD(void, set_pitch, (int voice, int note, int bend)); */
   mac_midi_set_pan,         /* AL_METHOD(void, set_pan, (int voice, int pan)); */
   _dummy_noop2,            /* AL_METHOD(void, set_vibrato, (int voice, int amount)); */
};


/* mac_midi_detect:
 *  MacOS MIDI detection.
 */
static int mac_midi_detect(int input)
{
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }
   return TRUE;
}



/* mac_midi_init:
 *  Initializes the MacOS MIDI driver.
 */
static int mac_midi_init(int input, int voices)
{
   NoteRequest nr;
   ComponentResult thisError;
   int i;
   qtna = OpenDefaultComponent(kNoteAllocatorComponentType, 0);
   printf("na=%d",qtna);
   if(!qtna){
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("open NoteAllocator failed"));
      printf("open NoteAllocator failed\n");
      return -1; 
   }
   for(i=1;i<17;i++){
      nr.info.flags = 0;
      nr.info.reserved = 0;
      nr.info.polyphony = 2;
      nr.info.typicalPolyphony = 0x00010000;
      macmidivoices[i].inst = (i==10)?16385:1;
      macmidivoices[i].vol = -1;
      macmidivoices[i].pan = -1;
      macmidivoices[i].note = 0;
      thisError = NAStuffToneDescription(qtna, macmidivoices[i].inst, &nr.tone);
      thisError = NANewNoteChannel(qtna, &nr, &macmidivoices[i].channel);
      if(thisError || !macmidivoices[i].channel){
          mac_midi_exit(input);
          ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("failed on channel initialization"));
	  printf("failed on init %s channel",i);
          return -1;
      }
   }
   return 0;
}



/* mac_midi_exit:
 *  Shuts down MIDI subsystem.
 */
static void mac_midi_exit(int input)
{
   int i;
   if (qtna){
      for(i=1; (i<17) && macmidivoices[i].channel;i++){
            mac_midi_key_off(i);
            NADisposeNoteChannel(qtna, macmidivoices[i].channel);
            macmidivoices[i].channel = 0;
      }
      CloseComponent(qtna);
      qtna=0;
   }
}



/* mac_midi_set_mixer_volume:
 *  Sets MIDI mixer output volume.
 */
static int mac_midi_set_mixer_volume(int volume)
{
   return 0;
}



/* mac_midi_key_on:
 *  Triggers a specified voice.
 */
static void mac_midi_key_on(int inst, int note, int bend, int vol, int pan)
{
   int voice;
   NoteChannel ch;
   ComponentResult thisError;
   printf("keyon(%d,%d,%d,%d,%d)\n",inst, note, bend, vol,pan);
   if (inst < 128) {
      voice = _midi_allocate_voice(1, 16);
   }
   else {
      inst = inst+16385-128;
      voice = _midi_allocate_voice(10,10);
   }
   ch = macmidivoices[voice].channel;
   if(!ch)
      return;
   mac_midi_key_off(voice);
   if(macmidivoices[voice].inst!=inst){
      thisError = NASetInstrumentNumber/*InterruptSafe*/(qtna,ch,inst);
      macmidivoices[voice].inst=inst;
   };
   if(macmidivoices[voice].vol!=vol){
      thisError = NASetNoteChannelVolume(qtna,ch,vol<<8);
      macmidivoices[voice].vol=vol;
   };
   if(macmidivoices[voice].pan!=pan){
      thisError = NASetNoteChannelBalance(qtna,ch,pan-128);
      macmidivoices[voice].pan=pan;
   };
   
   thisError = NAPlayNote(qtna,ch,note,80);
   macmidivoices[voice].note=note;
}



/* mac_midi_key_off:
 *  Turns off specified voice.
 */
static void mac_midi_key_off(int voice)
{
   if(macmidivoices[voice].channel)
      NAPlayNote(qtna,macmidivoices[voice].channel,macmidivoices[voice].note,0);
}



/* mac_midi_set_volume:
 *  Sets volume for a specified voice.
 */
static void mac_midi_set_volume(int voice, int vol)
{
}



/* mac_midi_set_pitch:
 *  Sets pitch of specified voice.
 */
static void mac_midi_set_pitch(int voice, int note, int bend)
{
}



/* mac_midi_set_pan:
 *  Sets pan value on specified voice.
 */
static void mac_midi_set_pan(int voice, int pan)
{
}
