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
 *      Windows midi driver.
 *
 *      By Stefan Schimanski.
 *
 *      Midi input added by Daniel Verkamp.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>

   #ifdef ALLEGRO_MSVC
      #include <mmreg.h>
   #endif
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wmidi INFO: "
#define PREFIX_W                "al-wmidi WARNING: "
#define PREFIX_E                "al-wmidi ERROR: "


int midi_win32_detect(int input);
int midi_win32_init(int input, int voices);
void midi_win32_exit(int input);
int midi_win32_set_mixer_volume(int volume);
int midi_win32_get_mixer_volume(void);
void midi_win32_raw_midi(int data);

int midi_win32_in_detect(int input);
int midi_win32_in_init(int input, int voices);
void midi_win32_in_exit(int input);
static void CALLBACK midi_in_proc(HMIDIIN, UINT, DWORD, DWORD, DWORD);


/* driver globals */
static HMIDIOUT midi_device = NULL;
static HMIDIIN  midi_in_device = NULL;


/* dynamically generated driver list */
static _DRIVER_INFO *driver_list = NULL;


/* MIDI recording callback */
static void CALLBACK midi_in_proc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance, 
				  DWORD dwParam1, DWORD dwParam2)
{
   if ((midi_in_device == NULL) || (midi_recorder == NULL)) return;
   midi_recorder((unsigned char)(dwParam1 & 0xff));         /* status byte */
   midi_recorder((unsigned char)((dwParam1 >> 8) & 0xff));  /* data byte 1 */
   midi_recorder((unsigned char)((dwParam1 >> 16) & 0xff)); /* data byte 2 */
}


/* _get_win_midi_driver_list:
 *  System driver hook for listing the available MIDI drivers. This generates
 *  the device list at runtime, to match whatever Windows devices are 
 *  available.
 */
_DRIVER_INFO *_get_win_midi_driver_list(void)
{
   MIDI_DRIVER *driver;
   MIDIOUTCAPS caps;
   MIDIINCAPS  caps_in;
   int num_drivers, i;

   if (!driver_list) {
      num_drivers = midiOutGetNumDevs();

      /* include the MIDI mapper (id == -1) */
      if (num_drivers)
	 num_drivers++;

      driver_list = _create_driver_list();

      /* MidiOut drivers */
      for (i=0; i<num_drivers; i++) {
         driver = _AL_MALLOC(sizeof(MIDI_DRIVER));
         memcpy(driver, &_midi_none, sizeof(MIDI_DRIVER));

         if (i == 0)
            driver->id = MIDI_WIN32MAPPER;
         else
            driver->id = MIDI_WIN32(i-1);

         midiOutGetDevCaps(i-1, &caps, sizeof(caps));

	 driver->ascii_name = strdup(caps.szPname);

	 driver->detect = midi_win32_detect;
	 driver->init = midi_win32_init;
	 driver->exit = midi_win32_exit;
	 driver->set_mixer_volume = midi_win32_set_mixer_volume;
	 driver->get_mixer_volume = midi_win32_get_mixer_volume;
	 driver->raw_midi = midi_win32_raw_midi;

         _driver_list_append_driver(&driver_list, driver->id, driver, TRUE);
      }

      /* MidiIn drivers */
      num_drivers = midiInGetNumDevs();
      for (i=0; i<num_drivers; i++) {
	 driver = _AL_MALLOC(sizeof(MIDI_DRIVER));
	 memcpy(driver, &_midi_none, sizeof(MIDI_DRIVER));

	 driver->id = MIDI_WIN32_IN(i); /* added MIDI_WIN32_IN to alwin.h */

	 midiInGetDevCaps(i, &caps_in, sizeof(caps_in));

	 driver->ascii_name = strdup(caps_in.szPname);

	 driver->detect = midi_win32_in_detect;
	 driver->init = midi_win32_in_init;
	 driver->exit = midi_win32_in_exit;

	 _driver_list_append_driver(&driver_list, driver->id, driver, TRUE);
      }

      /* cross-platform DIGital MIDi driver */
      _driver_list_append_driver(&driver_list, MIDI_DIGMID, &midi_digmid, TRUE);
   }

   return driver_list;
}



/* _free_win_midi_driver_list:
 *  Helper function for freeing the dynamically generated driver list.
 */
void _free_win_midi_driver_list(void)
{
   int i = 0;

   if (driver_list) {
      while (driver_list[i].driver) {
         if (driver_list[i].id != MIDI_DIGMID) {
            _AL_FREE((char*)((MIDI_DRIVER*)driver_list[i].driver)->ascii_name);
            _AL_FREE(driver_list[i].driver);
	 }
         i++;
      }

      _destroy_driver_list(driver_list);
      driver_list = NULL;
   }
}



/* midi_win32_detect:
 */
int midi_win32_detect(int input)
{
   /* the input drivers are separate from the output drivers */
   if (input)
      return FALSE;

   return TRUE;
}



/* midi_win32_in_detect:
 */
int midi_win32_in_detect(int input)
{
   if (input)
      return TRUE;

   return FALSE;
}



/* midi_win32_init:
 */
int midi_win32_init(int input, int voices)
{
   MMRESULT hr;
   int id;

   /* deduce our device number from the driver ID code */
   if ((midi_driver->id & 0xFF) == 'M')
      /* we are using the midi mapper (driver id is WIN32M) */
      id = MIDI_MAPPER;
   else
      /* actual driver */
      id = (midi_driver->id & 0xFF) - 'A';

   /* open midi mapper */
   hr = midiOutOpen(&midi_device, id, 0, 0, CALLBACK_NULL);
   if (hr != MMSYSERR_NOERROR) {
      _TRACE(PREFIX_E "midiOutOpen failed (%x)\n", hr);
      goto Error;
   }

   /* resets midi mapper */
   midiOutReset(midi_device);

   return 0;

 Error:
   midi_win32_exit(input);
   return -1;
}



/* midi_win32_in_init:
 */
int midi_win32_in_init(int input, int voices)
{
   MMRESULT hr;
   int id;

   /* deduce our device number from the driver ID code */
   id = (midi_input_driver->id & 0xFF) - 'A';

   /* open midi input device */
   hr = midiInOpen(&midi_in_device, id, (DWORD)midi_in_proc,
		   (DWORD)NULL, CALLBACK_FUNCTION);
   if (hr != MMSYSERR_NOERROR) {
      _TRACE(PREFIX_E "midiInOpen failed (%x)\n", hr);
      midi_win32_in_exit(input);
      return -1;
   }


   midiInReset(midi_in_device);
   midiInStart(midi_in_device);

   return 0;
}



/* midi_win32_exit:
 */
void midi_win32_exit(int input)
{
   /* close midi stream and release device */
   if (midi_device != NULL) {
      midiOutReset(midi_device);

      midiOutClose(midi_device);
      midi_device = NULL;
   }
}



/* midi_win32_in_exit:
 */
void midi_win32_in_exit(int input)
{
   if (midi_in_device != NULL) {
      midiInStop(midi_in_device);
      midiInReset(midi_in_device);
      midiInClose(midi_in_device);
      midi_in_device = NULL;
   }
}



/* midi_win32_set_mixer_volume:
 */
int midi_win32_set_mixer_volume(int volume)
{
   unsigned long win32_midi_vol = (volume << 8) + (volume << 24);
   midiOutSetVolume(midi_device, win32_midi_vol);
   return 1;
}



/* midi_win32_get_mixer_volume:
 */
int midi_win32_get_mixer_volume(void)
{
   DWORD vol;
   
   if (!midi_device)
      return -1;

   if (midiOutGetVolume(midi_device, &vol) != MMSYSERR_NOERROR)
      return -1;

   vol &= 0xffff;
   return vol / (0xffff / 255);
}



/* midi_switch_out:
 */
void midi_switch_out(void)
{
   if (midi_device)
      midiOutReset(midi_device);
}


/* midi_win32_raw_midi:
 */
void midi_win32_raw_midi(int data)
{
   static int msg_lengths[8] =
   {3, 3, 3, 3, 2, 2, 3, 0};
   static unsigned long midi_msg;
   static int midi_msg_len;
   static int midi_msg_pos;

   if (data >= 0x80) {
      midi_msg_len = msg_lengths[(data >> 4) & 0x07];
      midi_msg = 0;
      midi_msg_pos = 0;
   }

   if (midi_msg_len > 0) {
      midi_msg |= ((unsigned long)data) << (midi_msg_pos * 8);
      midi_msg_pos++;

      if (midi_msg_pos == midi_msg_len) {
	 if (midi_device != NULL) {
            switch (get_display_switch_mode()) {
               case SWITCH_AMNESIA:
               case SWITCH_PAUSE:
	          if (_win_app_foreground)
	             midiOutShortMsg(midi_device, midi_msg);
	          else
	             midiOutReset(midi_device);
                  break;
               default:
                  midiOutShortMsg(midi_device, midi_msg);
            }
	 }
      }
   }
}
