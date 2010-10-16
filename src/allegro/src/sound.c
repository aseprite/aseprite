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
 *      Sound setup routines and API framework functions.
 *
 *      By Shawn Hargreaves.
 *
 *      Input routines added by Ove Kaaven.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



static DIGI_DRIVER digi_none;



/* dummy functions for the nosound drivers */
int  _dummy_detect(int input) { return TRUE; }
int  _dummy_init(int input, int voices) { digi_none.desc = _midi_none.desc = get_config_text("The sound of silence"); return 0; }
void _dummy_exit(int input) { }
int  _dummy_set_mixer_volume(int volume) { return 0; }
int  _dummy_get_mixer_volume(void) { return -1; }
void _dummy_init_voice(int voice, AL_CONST SAMPLE *sample) { }
void _dummy_noop1(int p) { }
void _dummy_noop2(int p1, int p2) { }
void _dummy_noop3(int p1, int p2, int p3) { }
int  _dummy_get_position(int voice) { return -1; }
int  _dummy_get(int voice) { return 0; }
void _dummy_raw_midi(int data) { }
int  _dummy_load_patches(AL_CONST char *patches, AL_CONST char *drums) { return 0; }
void _dummy_adjust_patches(AL_CONST char *patches, AL_CONST char *drums) { }
void _dummy_key_on(int inst, int note, int bend, int vol, int pan) { }

/* put this after all the dummy functions, so they will all get locked */
END_OF_FUNCTION(_dummy_detect); 



static DIGI_DRIVER digi_none =
{
   DIGI_NONE,
   empty_string,
   empty_string,
   "No sound", 
   0, 0, 0xFFFF, 0,
   _dummy_detect,
   _dummy_init,
   _dummy_exit,
   _dummy_set_mixer_volume,
   _dummy_get_mixer_volume,
   NULL,
   NULL,
   NULL,
   _dummy_init_voice,
   _dummy_noop1,
   _dummy_noop1,
   _dummy_noop1,
   _dummy_noop2,
   _dummy_get_position,
   _dummy_noop2,
   _dummy_get,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop1,
   _dummy_get,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop1,
   _dummy_get,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop1,
   _dummy_noop3,
   _dummy_noop3,
   _dummy_noop3,
   0, 0,
   NULL, NULL, NULL, NULL, NULL, NULL
};


MIDI_DRIVER _midi_none =
{
   MIDI_NONE,
   empty_string,
   empty_string,
   "No sound", 
   0, 0, 0xFFFF, 0, -1, -1,
   _dummy_detect,
   _dummy_init,
   _dummy_exit,
   _dummy_set_mixer_volume,
   _dummy_get_mixer_volume,
   _dummy_raw_midi,
   _dummy_load_patches,
   _dummy_adjust_patches,
   _dummy_key_on,
   _dummy_noop1,
   _dummy_noop2,
   _dummy_noop3,
   _dummy_noop2,
   _dummy_noop2
};


int digi_card = DIGI_AUTODETECT;          /* current driver ID numbers */
int midi_card = MIDI_AUTODETECT;

int digi_input_card = DIGI_AUTODETECT;
int midi_input_card = MIDI_AUTODETECT;

DIGI_DRIVER *digi_driver = &digi_none;    /* these things do all the work */
MIDI_DRIVER *midi_driver = &_midi_none;

DIGI_DRIVER *digi_input_driver = &digi_none;
MIDI_DRIVER *midi_input_driver = &_midi_none;

void (*digi_recorder)(void) = NULL;
void (*midi_recorder)(unsigned char data) = NULL;

int _sound_installed = FALSE;             /* are we installed? */
int _sound_input_installed = FALSE;

static int digi_reserve = -1;             /* how many voices to reserve */
static int midi_reserve = -1;

static VOICE virt_voice[VIRTUAL_VOICES];  /* list of active samples */

PHYS_VOICE _phys_voice[DIGI_VOICES];      /* physical -> virtual voice map */

int _digi_volume = -1;                    /* current volume settings */
int _midi_volume = -1;

int _sound_flip_pan = FALSE;              /* reverse l/r sample panning? */
int _sound_hq = 2;                        /* mixer speed vs. quality */

int _sound_freq = -1;                     /* common hardware parameters */
int _sound_stereo = -1;
int _sound_bits = -1;
int _sound_port = -1;
int _sound_dma = -1;
int _sound_irq = -1;

#define SWEEP_FREQ   50

static void update_sweeps(void);
static void sound_lock_mem(void);



/* read_sound_config:
 *  Helper for reading the sound hardware configuration data.
 */
static void read_sound_config(void)
{
   char tmp1[64], tmp2[64];
   char *sound = uconvert_ascii("sound", tmp1);

   _sound_flip_pan = get_config_int(sound, uconvert_ascii("flip_pan",     tmp2), FALSE);
   _sound_hq       = get_config_int(sound, uconvert_ascii("quality",      tmp2), _sound_hq);
   _sound_port     = get_config_hex(sound, uconvert_ascii("sound_port",   tmp2), -1);
   _sound_dma      = get_config_int(sound, uconvert_ascii("sound_dma",    tmp2), -1);
   _sound_irq      = get_config_int(sound, uconvert_ascii("sound_irq",    tmp2), -1);
   _sound_freq     = get_config_int(sound, uconvert_ascii("sound_freq",   tmp2), -1);
   _sound_bits     = get_config_int(sound, uconvert_ascii("sound_bits",   tmp2), -1);
   _sound_stereo   = get_config_int(sound, uconvert_ascii("sound_stereo", tmp2), -1);
   _digi_volume    = get_config_int(sound, uconvert_ascii("digi_volume",  tmp2), -1);
   _midi_volume    = get_config_int(sound, uconvert_ascii("midi_volume",  tmp2), -1);
}



/* detect_digi_driver:
 *  Detects whether the specified digital sound driver is available,
 *  returning the maximum number of voices that it can provide, or
 *  zero if the device is not present. This function must be called
 *  _before_ install_sound().
 */
int detect_digi_driver(int driver_id)
{
   _DRIVER_INFO *digi_drivers;
   int i, ret;

   if (_sound_installed)
      return 0;

   read_sound_config();

   if (system_driver->digi_drivers)
      digi_drivers = system_driver->digi_drivers();
   else
      digi_drivers = _digi_driver_list;

   for (i=0; digi_drivers[i].id; i++) {
      if (digi_drivers[i].id == driver_id) {
	 digi_driver = digi_drivers[i].driver;
	 digi_driver->name = digi_driver->desc = get_config_text(digi_driver->ascii_name);
	 digi_card = driver_id;
	 midi_card = MIDI_AUTODETECT;

	 if (digi_driver->detect(FALSE))
	    ret = digi_driver->max_voices;
	 else
	    ret = 0;

	 digi_driver = &digi_none;
	 return ret;
      }
   }

   return digi_none.max_voices;
}



/* detect_midi_driver:
 *  Detects whether the specified MIDI sound driver is available,
 *  returning the maximum number of voices that it can provide, or
 *  zero if the device is not present. If this routine returns -1,
 *  it is a note-stealing MIDI driver, which shares voices with the
 *  current digital driver. In this situation you can use the 
 *  reserve_voices() function to specify how the available voices are
 *  divided between the digital and MIDI playback routines. This function
 *  must be called _before_ install_sound().
 */
int detect_midi_driver(int driver_id)
{
   _DRIVER_INFO *midi_drivers;
   int i, ret;

   if (_sound_installed)
      return 0;

   read_sound_config();

   if (system_driver->midi_drivers)
      midi_drivers = system_driver->midi_drivers();
   else
      midi_drivers = _midi_driver_list;

   for (i=0; midi_drivers[i].id; i++) {
      if (midi_drivers[i].id == driver_id) {
	 midi_driver = midi_drivers[i].driver;
	 midi_driver->name = midi_driver->desc = get_config_text(midi_driver->ascii_name);
	 digi_card = DIGI_AUTODETECT;
	 midi_card = driver_id;

	 if (midi_driver->detect(FALSE))
	    ret = midi_driver->max_voices;
	 else
	    ret = 0;

	 midi_driver = &_midi_none;
	 return ret;
      }
   }

   return _midi_none.max_voices;
}



/* reserve_voices:
 *  Reserves a number of voices for the digital and MIDI sound drivers
 *  respectively. This must be called _before_ install_sound(). If you 
 *  attempt to reserve too many voices, subsequent calls to install_sound()
 *  will fail. Note that depending on the driver you may actually get 
 *  more voices than you reserve: these values just specify the minimum
 *  that is appropriate for your application. Pass a negative reserve value 
 *  to use the default settings.
 */
void reserve_voices(int digi_voices, int midi_voices)
{
   digi_reserve = digi_voices;
   midi_reserve = midi_voices;
}



/* install_sound:
 *  Initialises the sound module, returning zero on success. The two card 
 *  parameters should use the DIGI_* and MIDI_* constants defined in 
 *  allegro.h. Pass DIGI_AUTODETECT and MIDI_AUTODETECT if you don't know 
 *  what the soundcard is.
 */
int install_sound(int digi, int midi, AL_CONST char *cfg_path)
{
   char tmp1[64], tmp2[64];
   char *sound = uconvert_ascii("sound", tmp1);
   _DRIVER_INFO *digi_drivers, *midi_drivers;
   int digi_voices, midi_voices;
   int c;

   if (_sound_installed)
      return 0;

   for (c=0; c<VIRTUAL_VOICES; c++) {
      virt_voice[c].sample = NULL;
      virt_voice[c].num = -1;
   }

   for (c=0; c<DIGI_VOICES; c++)
      _phys_voice[c].num = -1;

   /* initialise the MIDI file player */
   if (_al_linker_midi)
      if (_al_linker_midi->init() != 0)
	 return -1;

   usetc(allegro_error, 0);

   register_datafile_object(DAT_SAMPLE, NULL, (void (*)(void *))destroy_sample);

   digi_card = digi;
   midi_card = midi;

   /* read config information */
   if (digi_card == DIGI_AUTODETECT)
      digi_card = get_config_id(sound, uconvert_ascii("digi_card", tmp2), DIGI_AUTODETECT);

   if (midi_card == MIDI_AUTODETECT)
      midi_card = get_config_id(sound, uconvert_ascii("midi_card", tmp2), MIDI_AUTODETECT);

   if (digi_reserve < 0)
      digi_reserve = get_config_int(sound, uconvert_ascii("digi_voices", tmp2), -1);

   if (midi_reserve < 0)
      midi_reserve = get_config_int(sound, uconvert_ascii("midi_voices", tmp2), -1);

   read_sound_config();

   sound_lock_mem();

   /* set up digital sound driver */
   if (system_driver->digi_drivers)
      digi_drivers = system_driver->digi_drivers();
   else
      digi_drivers = _digi_driver_list;

   for (c=0; digi_drivers[c].driver; c++) {
      digi_driver = digi_drivers[c].driver;
      digi_driver->name = digi_driver->desc = get_config_text(digi_driver->ascii_name);
   }

   digi_driver = NULL;

   /* search table for a specific digital driver */
   for (c=0; digi_drivers[c].driver; c++) { 
      if (digi_drivers[c].id == digi_card) {
	 digi_driver = digi_drivers[c].driver;
	 if (!digi_driver->detect(FALSE)) {
	    digi_driver = &digi_none; 
	    if (_al_linker_midi)
	       _al_linker_midi->exit();
	    if (!ugetc(allegro_error))
	       ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Digital sound driver not found"));
	    return -1;
	 }
	 break;
      }
   }

   if (digi_card == DIGI_NONE)
      digi_driver = &digi_none;

   /* autodetect digital driver */
   if (!digi_driver) {
      for (c=0; digi_drivers[c].driver; c++) {
	 if (digi_drivers[c].autodetect) {
	    digi_card = digi_drivers[c].id;
	    digi_driver = digi_drivers[c].driver;
	    if (digi_driver->detect(FALSE))
	       break;
	    digi_driver = NULL;
	 }
      }

      if (!digi_driver) {
	 digi_card = DIGI_NONE;
	 digi_driver = &digi_none;
      }
   }

   /* set up midi sound driver */
   if (system_driver->midi_drivers)
      midi_drivers = system_driver->midi_drivers();
   else
      midi_drivers = _midi_driver_list;

   for (c=0; midi_drivers[c].driver; c++) {
      midi_driver = midi_drivers[c].driver;
      midi_driver->name = midi_driver->desc = get_config_text(midi_driver->ascii_name);
   }

   midi_driver = NULL;

   /* search table for a specific MIDI driver */
   for (c=0; midi_drivers[c].driver; c++) { 
      if (midi_drivers[c].id == midi_card) {
	 midi_driver = midi_drivers[c].driver;
	 if (!midi_driver->detect(FALSE)) {
	    digi_driver = &digi_none; 
	    midi_driver = &_midi_none; 
	    if (_al_linker_midi)
	       _al_linker_midi->exit();
	    if (!ugetc(allegro_error))
	       ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("MIDI music driver not found"));
	    return -1;
	 }
	 break;
      }
   }

   if (midi_card == MIDI_NONE)
      midi_driver = &_midi_none;

   /* autodetect MIDI driver */
   if (!midi_driver) {
      for (c=0; midi_drivers[c].driver; c++) {
	 if (midi_drivers[c].autodetect) {
	    midi_card = midi_drivers[c].id;
	    midi_driver = midi_drivers[c].driver;
	    if (midi_driver->detect(FALSE))
	       break;
	    midi_driver = NULL;
	 }
      }

      if (!midi_driver) {
	 midi_card = MIDI_NONE;
	 midi_driver = &_midi_none;
      }
   }

   /* work out how many voices to allocate for each driver */
   if (digi_reserve >= 0)
      digi_voices = digi_reserve;
   else
      digi_voices = digi_driver->def_voices;

   if (midi_driver->max_voices < 0) {
      /* MIDI driver steals voices from the digital player */
      if (midi_reserve >= 0)
	 midi_voices = midi_reserve;
      else
	 midi_voices = CLAMP(0, digi_driver->max_voices - digi_voices, midi_driver->def_voices);

      digi_voices += midi_voices;
   }
   else {
      /* MIDI driver has voices of its own */
      if (midi_reserve >= 0)
	 midi_voices = midi_reserve;
      else
	 midi_voices = midi_driver->def_voices;
   }

   /* make sure this is a reasonable number of voices to use */
   if ((digi_voices > DIGI_VOICES) || (midi_voices > MIDI_VOICES)) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Insufficient %s voices available"),
		(digi_voices > DIGI_VOICES) ? get_config_text("digital") : get_config_text("MIDI"));
      digi_driver = &digi_none; 
      midi_driver = &_midi_none; 
      if (_al_linker_midi)
	 _al_linker_midi->exit();
      return -1;
   }

   /* initialise the digital sound driver */
   if (digi_driver->init(FALSE, digi_voices) != 0) {
      digi_driver = &digi_none; 
      midi_driver = &_midi_none; 
      if (_al_linker_midi)
	 _al_linker_midi->exit();
      if (!ugetc(allegro_error))
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to init digital sound driver"));
      return -1;
   }

   /* initialise the MIDI driver */
   if (midi_driver->init(FALSE, midi_voices) != 0) {
      digi_driver->exit(FALSE);
      digi_driver = &digi_none; 
      midi_driver = &_midi_none; 
      if (_al_linker_midi)
	 _al_linker_midi->exit();
      if (!ugetc(allegro_error))
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to init MIDI music driver"));
      return -1;
   }

   digi_driver->voices = MIN(digi_driver->voices, DIGI_VOICES);
   midi_driver->voices = MIN(midi_driver->voices, MIDI_VOICES);

   /* check that we actually got enough voices */
   if ((digi_driver->voices < digi_voices) || 
       ((midi_driver->voices < midi_voices) && (!midi_driver->raw_midi))) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Insufficient %s voices available"),
                (digi_driver->voices < digi_voices) ? get_config_text("digital") : get_config_text("MIDI"));
      midi_driver->exit(FALSE);
      digi_driver->exit(FALSE);
      digi_driver = &digi_none; 
      midi_driver = &_midi_none; 
      if (_al_linker_midi)
	 _al_linker_midi->exit();
      return -1;
   }

   /* adjust for note-stealing MIDI drivers */
   if (midi_driver->max_voices < 0) {
      midi_voices += (digi_driver->voices - digi_voices) * 3/4;
      digi_driver->voices -= midi_voices;
      midi_driver->basevoice = VIRTUAL_VOICES - midi_voices;
      midi_driver->voices = midi_voices;

      for (c=0; c<midi_voices; c++) {
	 virt_voice[midi_driver->basevoice+c].num = digi_driver->voices+c;
	 _phys_voice[digi_driver->voices+c].num = midi_driver->basevoice+c;
      }
   }

   /* simulate ramp/sweep effects for drivers that don't do it directly */
   if ((!digi_driver->ramp_volume) ||
       (!digi_driver->sweep_frequency) ||
       (!digi_driver->sweep_pan))
      install_int_ex(update_sweeps, BPS_TO_TIMER(SWEEP_FREQ));

   /* set the global sound volume */
   if ((_digi_volume >= 0) || (_midi_volume >= 0))
      set_volume(_digi_volume, _midi_volume);

   _add_exit_func(remove_sound, "remove_sound");
   _sound_installed = TRUE;
   return 0;
}



/* install_sound_input:
 *  Initialises the sound recorder module, returning zero on success. The
 *  two card parameters should use the DIGI_* and MIDI_* constants defined
 *  in allegro.h. Pass DIGI_AUTODETECT and MIDI_AUTODETECT if you don't know
 *  what the soundcard is.
 */
int install_sound_input(int digi, int midi)
{
   char tmp1[64], tmp2[64];
   char *sound = uconvert_ascii("sound", tmp1);
   _DRIVER_INFO *digi_drivers, *midi_drivers;
   int c;

   if (_sound_input_installed)
      return 0;

   if (!_sound_installed) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Sound system not installed"));
      return -1;
   }

   digi_recorder = NULL;
   midi_recorder = NULL;

   digi_input_card = digi;
   midi_input_card = midi;

   /* read config information */
   if (digi_input_card == DIGI_AUTODETECT)
      digi_input_card = get_config_id(sound, uconvert_ascii("digi_input_card", tmp2), DIGI_AUTODETECT);

   if (midi_input_card == MIDI_AUTODETECT)
      midi_input_card = get_config_id(sound, uconvert_ascii("midi_input_card", tmp2), MIDI_AUTODETECT);

   /* search table for a good digital input driver */
   usetc(allegro_error, 0);

   if (system_driver->digi_drivers)
      digi_drivers = system_driver->digi_drivers();
   else
      digi_drivers = _digi_driver_list;

   for (c=0; digi_drivers[c].driver; c++) {
      if ((digi_drivers[c].id == digi_input_card) || (digi_input_card == DIGI_AUTODETECT)) {
	 digi_input_driver = digi_drivers[c].driver;
	 if (digi_input_driver->detect(TRUE)) {
	    digi_input_card = digi_drivers[c].id;
	    break;
	 }
	 else {
	    digi_input_driver = &digi_none;
	    if (digi_input_card != DIGI_AUTODETECT) {
	       if (!ugetc(allegro_error))
		  uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s does not support audio input"),
			    ((DIGI_DRIVER *)digi_drivers[c].driver)->name);
	       break;
	    }
	 }
      }
   }

   /* did we find one? */
   if ((digi_input_driver == &digi_none) && (digi_input_card != DIGI_NONE)) {
      if (!ugetc(allegro_error))
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Digital input driver not found"));
      return -1;
   }

   /* search table for a good MIDI input driver */
   usetc(allegro_error, 0);

   if (system_driver->midi_drivers)
      midi_drivers = system_driver->midi_drivers();
   else
      midi_drivers = _midi_driver_list;

   for (c=0; midi_drivers[c].driver; c++) { 
      if ((midi_drivers[c].id == midi_input_card) || (midi_input_card == MIDI_AUTODETECT)) {
	 midi_input_driver = midi_drivers[c].driver;
	 if (midi_input_driver->detect(TRUE)) {
	    midi_input_card = midi_drivers[c].id;
	    break;
	 }
	 else {
	    midi_input_driver = &_midi_none;
	    if (midi_input_card != MIDI_AUTODETECT) {
	       if (!ugetc(allegro_error))
		  uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s does not support MIDI input"),
			    ((MIDI_DRIVER *)midi_drivers[c].driver)->name);
	       break;
	    }
	 }
      }
   }

   /* did we find one? */
   if ((midi_input_driver == &_midi_none) && (midi_input_card != MIDI_NONE)) {
      digi_input_driver = &digi_none;
      if (!ugetc(allegro_error))
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("MIDI input driver not found"));
      return -1;
   }

   /* initialise the digital input driver */
   if (digi_input_driver->init(TRUE, 0) != 0) {
      digi_input_driver = &digi_none;
      midi_input_driver = &_midi_none;
      if (!ugetc(allegro_error))
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to init digital input driver"));
      return -1;
   }

   /* initialise the MIDI input driver */
   if (midi_input_driver->init(TRUE, 0) != 0) {
      digi_input_driver->exit(TRUE);
      digi_input_driver = &digi_none;
      midi_input_driver = &_midi_none;
      if (!ugetc(allegro_error))
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Failed to init MIDI input driver"));
      return -1;
   }

   _sound_input_installed = TRUE;
   return 0;
}



/* remove_sound:
 *  Sound module cleanup routine.
 */
void remove_sound(void)
{
   int c;

   if (_sound_installed) {
      remove_sound_input();

      remove_int(update_sweeps);

      for (c=0; c<VIRTUAL_VOICES; c++)
	 if (virt_voice[c].sample)
	    deallocate_voice(c);

      if (_al_linker_midi)
	 _al_linker_midi->exit();

      midi_driver->exit(FALSE);
      midi_driver = &_midi_none; 

      digi_driver->exit(FALSE);
      digi_driver = &digi_none; 

      _remove_exit_func(remove_sound);
      _sound_installed = FALSE;
   }
}



/* remove_sound_input:
 *  Sound input module cleanup routine.
 */
void remove_sound_input(void)
{
   if (_sound_input_installed) {
      digi_input_driver->exit(TRUE);
      digi_input_driver = &digi_none;

      midi_input_driver->exit(TRUE);
      midi_input_driver = &_midi_none;

      digi_recorder = NULL;
      midi_recorder = NULL;

      _sound_input_installed = FALSE;
   }
}



/* set_volume:
 *  Alters the global sound output volume. Specify volumes for both digital
 *  samples and MIDI playback, as integers from 0 to 255. This routine will
 *  not alter the volume of the hardware mixer if it exists.
 */
void set_volume(int digi_volume, int midi_volume)
{
   int *voice_vol;
   int i;

   if (digi_volume >= 0) {
      voice_vol = _AL_MALLOC_ATOMIC(sizeof(int)*VIRTUAL_VOICES);

      /* Retrieve the (relative) volume of each voice. */
      for (i=0; i<VIRTUAL_VOICES; i++)
	 voice_vol[i] = voice_get_volume(i);

      _digi_volume = CLAMP(0, digi_volume, 255);

      /* Set the new (relative) volume for each voice. */
      for (i=0; i<VIRTUAL_VOICES; i++) {
	 if (voice_vol[i] >= 0)
	    voice_set_volume(i, voice_vol[i]);
      }

      _AL_FREE(voice_vol);
   }

   if (midi_volume >= 0)
      _midi_volume = CLAMP(0, midi_volume, 255);
}



/* get_volume:
 * Retrieves the global sound output volume, both for digital samples and MIDI
 * playback, as integers from 0 to 255. Parameters digi_volume and midi_volume
 * must be valid pointers to int, or NULL if not interested in specific value.
 */
void get_volume(int *digi_volume, int *midi_volume)
{
   if (digi_volume)
      (*digi_volume) = _digi_volume;
   if (midi_volume)
      (*midi_volume) = _midi_volume;
}



/* set_hardware_volume:
 *  Alters the hardware sound output volume. Specify volumes for both digital
 *  samples and MIDI playback, as integers from 0 to 255. This routine will
 *  use the hardware mixer to control the volume if it exists, or do nothing.
 */
void set_hardware_volume(int digi_volume, int midi_volume)
{
   if (digi_volume >= 0) {
      digi_volume = CLAMP(0, digi_volume, 255);

      if (digi_driver->set_mixer_volume)
	 digi_driver->set_mixer_volume(digi_volume);
   }

   if (midi_volume >= 0) {
      midi_volume = CLAMP(0, midi_volume, 255);

      if (midi_driver->set_mixer_volume)
	 midi_driver->set_mixer_volume(midi_volume);
   }
}



/* get_hardware_volume:
 * Retrieves the hardware sound output volume, both for digital samples and MIDI
 * playback, as integers from 0 to 255, or -1 if the information is not
 * available. Parameters digi_volume and midi_volume must be valid pointers to
 * int, or NULL if not interested in specific value.
 */
void get_hardware_volume(int *digi_volume, int *midi_volume)
{
   if (digi_volume) {
      if (digi_driver->get_mixer_volume) {
	 (*digi_volume) = digi_driver->get_mixer_volume();
      }
      else {
	 (*digi_volume) = -1;
      }
   }

   if (midi_volume) {
      if (midi_driver->get_mixer_volume) {
	 (*midi_volume) = midi_driver->get_mixer_volume();
      }
      else {
	 (*midi_volume) = -1;
      }
   }
}



/* lock_sample:
 *  Locks a SAMPLE struct into physical memory. Pretty important, since 
 *  they are mostly accessed inside interrupt handlers.
 */
void lock_sample(SAMPLE *spl)
{
   ASSERT(spl);
   LOCK_DATA(spl, sizeof(SAMPLE));
   LOCK_DATA(spl->data, spl->len * ((spl->bits==8) ? 1 : sizeof(short)) * ((spl->stereo) ? 2 : 1));
}



/* load_voc:
 *  Reads a mono VOC format sample file, returning a SAMPLE structure, 
 *  or NULL on error.
 */
SAMPLE *load_voc(AL_CONST char *filename)
{
   PACKFILE *f;
   SAMPLE *spl;
   ASSERT(filename);

   f = pack_fopen(filename, F_READ);
   if (!f) 
      return NULL;

   spl = load_voc_pf(f);

   pack_fclose(f);

   return spl;
}



/* load_voc_pf:
 *  Reads a mono VOC format sample from the packfile given, returning a
 *  SAMPLE structure, or NULL on error. If successful the offset into the
 *  file will be left just after the sample data. If unsuccessful the offset
 *  into the file is unspecified, i.e. you must either reset the offset to
 *  some known place or close the packfile. The packfile is not closed by
 *  this function.
 */
SAMPLE *load_voc_pf(PACKFILE *f)
{
   char buffer[30];
   int freq = 22050;
   int bits = 8;
   SAMPLE *spl = NULL;
   int len;
   int x, ver;
   int s;
   ASSERT(f);

   memset(buffer, 0, sizeof buffer);

   pack_fread(buffer, 0x16, f);

   if (memcmp(buffer, "Creative Voice File", 0x13))
      goto getout;

   ver = pack_igetw(f);
   if (ver != 0x010A && ver != 0x0114) /* version: should be 0x010A or 0x0114 */
      goto getout;

   ver = pack_igetw(f);
   if (ver != 0x1129 && ver != 0x111f) /* subversion: should be 0x1129 or 0x111f */
      goto getout;

   ver = pack_getc(f);
   if (ver != 0x01 && ver != 0x09)     /* sound data: should be 0x01 or 0x09 */
      goto getout;

   len = pack_igetw(f);                /* length is three bytes long: two */
   x = pack_getc(f);                   /* .. and one byte */
   x <<= 16;
   len += x;

   if (ver == 0x01) {                  /* block type 1 */
      len -= 2;                        /* sub. size of the rest of block header */
      x = pack_getc(f);                /* one byte of frequency */
      freq = 1000000 / (256-x);

      x = pack_getc(f);                /* skip one byte */

      spl = create_sample(8, FALSE, freq, len);

      if (spl) {
	 if (pack_fread(spl->data, len, f) < len) {
	    destroy_sample(spl);
	    spl = NULL;
	 }
      }
   }
   else {                              /* block type 9 */
      len -= 12;                       /* sub. size of the rest of block header */
      freq = pack_igetw(f);            /* two bytes of frequency */

      x = pack_igetw(f);               /* skip two bytes */

      bits = pack_getc(f);             /* # of bits per sample */
      if (bits != 8 && bits != 16)
	 goto getout;

      x = pack_getc(f);
      if (x != 1)                      /* # of channels: should be mono */
	 goto getout;

      pack_fread(buffer, 0x6, f);      /* skip 6 bytes of unknown data */

      spl = create_sample(bits, FALSE, freq, len*8/bits);

      if (spl) {
	 if (bits == 8) {
	    if (pack_fread(spl->data, len, f) < len) {
	       destroy_sample(spl);
	       spl = NULL;
	    }
	 }
	 else {
	    len /= 2;
	    for (x=0; x<len; x++) {
	       if ((s = pack_igetw(f)) == EOF) {
		  destroy_sample(spl);
		  spl = NULL;
		  break;
	       }
	       ((signed short *)spl->data)[x] = (signed short)s^0x8000;
	    }
	 }
      }
   }

   getout: 
   return spl;
}



/* load_wav:
 *  Reads a RIFF WAV format sample file, returning a SAMPLE structure, 
 *  or NULL on error.
 */
SAMPLE *load_wav(AL_CONST char *filename)
{
   PACKFILE *f;
   SAMPLE *spl;
   ASSERT(filename);

   f = pack_fopen(filename, F_READ);
   if (!f)
      return NULL;

   spl = load_wav_pf(f);

   pack_fclose(f);

   return spl;
}



/* load_wav_pf:
 *  Reads a RIFF WAV format sample from the packfile given, returning a
 *  SAMPLE structure, or NULL on error.
 *
 *  Note that the loader does not stop reading bytes from the PACKFILE until
 *  it sees the EOF. If you want to embed a WAV file into another file, you
 *  will have to implement your own chunking system that stops reading at the
 *  end of the chunk.
 *
 *  If unsuccessful the offset into the file is unspecified, i.e. you must
 *  either reset the offset to some known place or close the packfile. The
 *  packfile is not closed by this function.
 */
SAMPLE *load_wav_pf(PACKFILE *f)
{
   char buffer[25];
   int i;
   int length, len;
   int freq = 22050;
   int bits = 8;
   int channels = 1;
   int s;
   SAMPLE *spl = NULL;
   ASSERT(f);

   memset(buffer, 0, sizeof buffer);

   pack_fread(buffer, 12, f);          /* check RIFF header */
   if (memcmp(buffer, "RIFF", 4) || memcmp(buffer+8, "WAVE", 4))
      goto getout;

   while (TRUE) {
      if (pack_fread(buffer, 4, f) != 4)
	 break;

      length = pack_igetl(f);          /* read chunk length */

      if (memcmp(buffer, "fmt ", 4) == 0) {
	 i = pack_igetw(f);            /* should be 1 for PCM data */
	 length -= 2;
	 if (i != 1) 
	    goto getout;

	 channels = pack_igetw(f);     /* mono or stereo data */
	 length -= 2;
	 if ((channels != 1) && (channels != 2))
	    goto getout;

	 freq = pack_igetl(f);         /* sample frequency */
	 length -= 4;

	 pack_igetl(f);                /* skip six bytes */
	 pack_igetw(f);
	 length -= 6;

	 bits = pack_igetw(f);         /* 8 or 16 bit data? */
	 length -= 2;
	 if ((bits != 8) && (bits != 16))
	    goto getout;
      }
      else if (memcmp(buffer, "data", 4) == 0) {
	 if (channels == 2) {
	    /* allocate enough space even if length is odd for some reason */
	    len = (length + 1) / 2;
	 }
	 else {
	    ASSERT(channels == 1);
	    len = length;
	 }

	 if (bits == 16)
	    len /= 2;

	 spl = create_sample(bits, ((channels == 2) ? TRUE : FALSE), freq, len);

	 if (spl) {
	    if (bits == 8) {
	       if (pack_fread(spl->data, length, f) < length) {
		  destroy_sample(spl);
		  spl = NULL;
	       }
	    }
	    else {
	       for (i=0; i<len*channels; i++) {
		  if ((s = pack_igetw(f)) == EOF) {
		     destroy_sample(spl);
		     spl = NULL;
		     break;
		  }
		  ((signed short *)spl->data)[i] = (signed short)s^0x8000;
	       }
	    }

	    length = 0;
	 }
      }

      while (length > 0) {             /* skip the remainder of the chunk */
	 if (pack_getc(f) == EOF)
	    break;

	 length--;
      }
   }

   getout:
   return spl;
}



/* create_sample:
 *  Constructs a new sample structure of the specified type.
 */
SAMPLE *create_sample(int bits, int stereo, int freq, int len)
{
   SAMPLE *spl;
   ASSERT(freq > 0);
   ASSERT(len > 0);

   spl = _AL_MALLOC(sizeof(SAMPLE)); 
   if (!spl)
      return NULL;

   spl->bits = bits;
   spl->stereo = stereo;
   spl->freq = freq;
   spl->priority = 128;
   spl->len = len;
   spl->loop_start = 0;
   spl->loop_end = len;
   spl->param = 0;

   spl->data = _AL_MALLOC_ATOMIC(len * ((bits==8) ? 1 : sizeof(short)) * ((stereo) ? 2 : 1));
   if (!spl->data) {
      _AL_FREE(spl);
      return NULL;
   }

   lock_sample(spl);
   return spl;
}



/* destroy_sample:
 *  Frees a SAMPLE struct, checking whether the sample is currently playing, 
 *  and stopping it if it is.
 */
void destroy_sample(SAMPLE *spl)
{
   if (spl) {
      stop_sample(spl);

      if (spl->data) {
	 UNLOCK_DATA(spl->data, spl->len * ((spl->bits==8) ? 1 : sizeof(short)) * ((spl->stereo) ? 2 : 1));
	 _AL_FREE(spl->data);
      }

      UNLOCK_DATA(spl, sizeof(SAMPLE));
      _AL_FREE(spl);
   }
}



/* absolute_freq:
 *  Converts a pitch from the relative 1000 = original format to an absolute
 *  value in hz.
 */
static INLINE int absolute_freq(int freq, AL_CONST SAMPLE *spl)
{
   ASSERT(spl);
   if (freq == 1000)
      return spl->freq;
   else
      return (spl->freq * freq) / 1000;
}



/* play_sample:
 *  Triggers a sample at the specified volume, pan position, and frequency.
 *  The volume and pan range from 0 (min/left) to 255 (max/right), although
 *  the resolution actually used by the playback routines is likely to be
 *  less than this. Frequency is relative rather than absolute: 1000 
 *  represents the frequency that the sample was recorded at, 2000 is 
 *  twice this, etc. If loop is true the sample will repeat until you call 
 *  stop_sample(), and can be manipulated while it is playing by calling
 *  adjust_sample().
 */
int play_sample(AL_CONST SAMPLE *spl, int vol, int pan, int freq, int loop)
{
   int voice;
   ASSERT(spl);
   ASSERT(vol >= 0 && vol <= 255);
   ASSERT(pan >= 0 && pan <= 255);
   ASSERT(freq > 0);

   voice = allocate_voice(spl);
   if (voice >= 0) {
      voice_set_volume(voice, vol);
      voice_set_pan(voice, pan);
      voice_set_frequency(voice, absolute_freq(freq, spl));
      voice_set_playmode(voice, (loop ? PLAYMODE_LOOP : PLAYMODE_PLAY));
      voice_start(voice);
      release_voice(voice);
   }

   return voice;
}

END_OF_FUNCTION(play_sample);



/* adjust_sample:
 *  Alters the parameters of a sample while it is playing, useful for
 *  manipulating looped sounds. You can alter the volume, pan, and
 *  frequency, and can also remove the looping flag, which will stop
 *  the sample when it next reaches the end of its loop. If there are
 *  several copies of the same sample playing, this will adjust the
 *  first one it comes across. If the sample is not playing it has no
 *  effect.
 */
void adjust_sample(AL_CONST SAMPLE *spl, int vol, int pan, int freq, int loop)
{
   int c;
   ASSERT(spl);

   for (c=0; c<VIRTUAL_VOICES; c++) { 
      if (virt_voice[c].sample == spl) {
	 voice_set_volume(c, vol);
	 voice_set_pan(c, pan);
	 voice_set_frequency(c, absolute_freq(freq, spl));
	 voice_set_playmode(c, (loop ? PLAYMODE_LOOP : PLAYMODE_PLAY));
	 return;
      }
   }
}

END_OF_FUNCTION(adjust_sample);



/* stop_sample:
 *  Kills off a sample, which is required if you have set a sample going 
 *  in looped mode. If there are several copies of the sample playing,
 *  it will stop them all.
 */
void stop_sample(AL_CONST SAMPLE *spl)
{
   int c;
   ASSERT(spl);

   for (c=0; c<VIRTUAL_VOICES; c++)
      if (virt_voice[c].sample == spl)
	 deallocate_voice(c);
}

END_OF_FUNCTION(stop_sample);



/* allocate_physical_voice:
 *  Allocates a physical voice, killing off others as required in order
 *  to make room for it.
 */
static INLINE int allocate_physical_voice(int priority)
{
   VOICE *voice;
   int best = -1;
   int best_score = 0;
   int score;
   int c;

   /* look for a free voice */
   for (c=0; c<digi_driver->voices; c++)
      if (_phys_voice[c].num < 0)
	 return c;

   /* look for an autokill voice that has stopped */
   for (c=0; c<digi_driver->voices; c++) {
      voice = virt_voice + _phys_voice[c].num;
      if ((voice->autokill) && (digi_driver->get_position(c) < 0)) {
	 digi_driver->release_voice(c);
	 voice->sample = NULL;
	 voice->num = -1;
	 _phys_voice[c].num = -1;
	 return c;
      }
   }

   /* ok, we're going to have to get rid of something to make room... */
   for (c=0; c<digi_driver->voices; c++) {
      voice = virt_voice + _phys_voice[c].num;

      /* sort by voice priorities */
      if (voice->priority <= priority) {
	 score = 65536 - voice->priority * 256;

	 /* bias with a least-recently-used counter */
	 score += CLAMP(0, retrace_count - voice->time, 32768);

	 /* bias according to whether the voice is looping or not */
	 if (!(_phys_voice[c].playmode & PLAYMODE_LOOP))
	    score += 32768;

	 if (score > best_score) {
	    best = c;
	    best_score = score;
	 }
      }
   }

   if (best >= 0) {
      /* kill off the old voice */
      digi_driver->stop_voice(best);
      digi_driver->release_voice(best);
      virt_voice[_phys_voice[best].num].num = -1;
      _phys_voice[best].num = -1;
      return best;
   }

   return -1;
}



/* allocate_virtual_voice:
 *  Allocates a virtual voice. This doesn't need to worry about killing off 
 *  others to make room, as we allow up to 256 virtual voices to be used
 *  simultaneously.
 */
static INLINE int allocate_virtual_voice(void)
{
   int num_virt_voices, c;

   num_virt_voices = VIRTUAL_VOICES;
   if (midi_driver->max_voices < 0)
      num_virt_voices -= midi_driver->voices;

   /* look for a free voice */
   for (c=0; c<num_virt_voices; c++)
      if (!virt_voice[c].sample)
	 return c;

   /* look for a stopped autokill voice */
   for (c=0; c<num_virt_voices; c++) {
      if (virt_voice[c].autokill) {
	 if (virt_voice[c].num < 0) {
	    virt_voice[c].sample = NULL;
	    return c;
	 }
	 else {
	    if (digi_driver->get_position(virt_voice[c].num) < 0) {
	       digi_driver->release_voice(virt_voice[c].num);
	       _phys_voice[virt_voice[c].num].num = -1;
	       virt_voice[c].sample = NULL;
	       virt_voice[c].num = -1;
	       return c;
	    }
	 }
      }
   }

   return -1;
}



/* allocate_voice:
 *  Allocates a voice ready for playing the specified sample, returning
 *  the voice number (note this is not the same as the physical voice
 *  number used by the sound drivers, and must only be used with the other
 *  voice functions, _not_ passed directly to the driver routines).
 *  Returns -1 if there is no voice available (this should never happen,
 *  since there are 256 virtual voices and anyone who needs more than that
 *  needs some urgent repairs to their brain :-)
 */
int allocate_voice(AL_CONST SAMPLE *spl)
{
   int phys, virt;
   ASSERT(spl);
   
   phys = allocate_physical_voice(spl->priority);
   virt = allocate_virtual_voice();

   if (virt >= 0) {
      virt_voice[virt].sample = spl;
      virt_voice[virt].num = phys;
      virt_voice[virt].autokill = FALSE;
      virt_voice[virt].time = retrace_count;
      virt_voice[virt].priority = spl->priority;

      if (phys >= 0) {
	 _phys_voice[phys].num = virt;
	 _phys_voice[phys].playmode = 0;
	 _phys_voice[phys].vol = ((_digi_volume >= 0) ? _digi_volume : 255) << 12;
	 _phys_voice[phys].pan = 128 << 12;
	 _phys_voice[phys].freq = spl->freq << 12;
	 _phys_voice[phys].dvol = 0;
	 _phys_voice[phys].dpan = 0;
	 _phys_voice[phys].dfreq = 0;

	 digi_driver->init_voice(phys, spl);
      }
   }

   return virt;
}

END_OF_FUNCTION(allocate_voice);



/* deallocate_voice:
 *  Releases a voice that was previously returned by allocate_voice().
 */
void deallocate_voice(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   
   if (virt_voice[voice].num >= 0) {
      digi_driver->stop_voice(virt_voice[voice].num);
      digi_driver->release_voice(virt_voice[voice].num);
      _phys_voice[virt_voice[voice].num].num = -1;
      virt_voice[voice].num = -1;
   }

   virt_voice[voice].sample = NULL;
}

END_OF_FUNCTION(deallocate_voice);



/* reallocate_voice:
 *  Switches an already-allocated voice to use a different sample.
 */
void reallocate_voice(int voice, AL_CONST SAMPLE *spl)
{
   int phys;
   ASSERT(spl);
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);

   phys =  virt_voice[voice].num;

   if (phys >= 0) {
      digi_driver->stop_voice(phys);
      digi_driver->release_voice(phys);
   }

   virt_voice[voice].sample = spl;
   virt_voice[voice].autokill = FALSE;
   virt_voice[voice].time = retrace_count;
   virt_voice[voice].priority = spl->priority;

   if (phys >= 0) {
      _phys_voice[phys].playmode = 0;
      _phys_voice[phys].vol = ((_digi_volume >= 0) ? _digi_volume : 255) << 12;
      _phys_voice[phys].pan = 128 << 12;
      _phys_voice[phys].freq = spl->freq << 12;
      _phys_voice[phys].dvol = 0;
      _phys_voice[phys].dpan = 0;
      _phys_voice[phys].dfreq = 0;

      digi_driver->init_voice(phys, spl);
   }
}

END_OF_FUNCTION(reallocate_voice);



/* release_voice:
 *  Flags that a voice is no longer going to be updated, so it can 
 *  automatically be freed as soon as the sample finishes playing.
 */
void release_voice(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   virt_voice[voice].autokill = TRUE;
}

END_OF_FUNCTION(release_voice);



/* voice_start:
 *  Starts a voice playing.
 */
void voice_start(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].num >= 0)
      digi_driver->start_voice(virt_voice[voice].num);

   virt_voice[voice].time = retrace_count;
}

END_OF_FUNCTION(voice_start);



/* voice_stop:
 *  Stops a voice from playing.
 */
void voice_stop(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].num >= 0)
      digi_driver->stop_voice(virt_voice[voice].num);
}

END_OF_FUNCTION(voice_stop);



/* voice_set_priority:
 *  Adjusts the priority of a voice (0-255).
 */
void voice_set_priority(int voice, int priority)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   ASSERT(priority >= 0 && priority <= 255);
   virt_voice[voice].priority = priority;
}

END_OF_FUNCTION(voice_set_priority);



/* voice_check:
 *  Checks whether a voice is playing, returning the sample if it is,
 *  or NULL if it has finished or been preempted by a different sound.
 */
SAMPLE *voice_check(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].sample) {
      if (virt_voice[voice].num < 0)
	 return NULL;

      if (virt_voice[voice].autokill)
	 if (voice_get_position(voice) < 0)
	    return NULL;

      return (SAMPLE*)virt_voice[voice].sample;
   }
   else
      return NULL;
}

END_OF_FUNCTION(voice_check);



/* voice_get_position:
 *  Returns the current play position of a voice, or -1 if that cannot
 *  be determined (because it has finished or been preempted by a
 *  different sound).
 */
int voice_get_position(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].num >= 0)
      return digi_driver->get_position(virt_voice[voice].num);
   else
      return -1;
}

END_OF_FUNCTION(voice_get_position);



/* voice_set_position:
 *  Sets the play position of a voice.
 */
void voice_set_position(int voice, int position)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].num >= 0)
      digi_driver->set_position(virt_voice[voice].num, position);
}

END_OF_FUNCTION(voice_set_position);



/* voice_set_playmode:
 *  Sets the loopmode of a voice.
 */
void voice_set_playmode(int voice, int playmode)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].num >= 0) {
      _phys_voice[virt_voice[voice].num].playmode = playmode;
      digi_driver->loop_voice(virt_voice[voice].num, playmode);

      if (playmode & PLAYMODE_BACKWARD)
	 digi_driver->set_position(virt_voice[voice].num, virt_voice[voice].sample->len-1);
   }
}

END_OF_FUNCTION(voice_set_playmode);



/* voice_get_volume:
 *  Returns the current volume of a voice, or -1 if that cannot
 *  be determined (because it has finished or been preempted by a
 *  different sound).
 */
int voice_get_volume(int voice)
{
   int vol;
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);

   if (virt_voice[voice].num >= 0)
      vol = digi_driver->get_volume(virt_voice[voice].num);
   else
      vol = -1;

   if ((vol >= 0) && (_digi_volume >= 0)) {
      if (_digi_volume > 0)
	 vol = CLAMP(0, (vol * 255) / _digi_volume, 255);
      else
	 vol = 0;
   }

   return vol;
}

END_OF_FUNCTION(voice_get_volume);



/* voice_set_volume:
 *  Sets the current volume of a voice.
 */
void voice_set_volume(int voice, int volume)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   ASSERT(volume >= 0 && volume <= 255);

   if (_digi_volume >= 0)
      volume = (volume * _digi_volume) / 255;

   if (virt_voice[voice].num >= 0) {
      _phys_voice[virt_voice[voice].num].vol = volume << 12;
      _phys_voice[virt_voice[voice].num].dvol = 0;

      digi_driver->set_volume(virt_voice[voice].num, volume);
   }
}

END_OF_FUNCTION(voice_set_volume);



/* voice_ramp_volume:
 *  Begins a volume ramp operation.
 */
void voice_ramp_volume(int voice, int time, int endvol)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   ASSERT(endvol >= 0 && endvol <= 255);
   ASSERT(time >= 0);
   
   if (_digi_volume >= 0)
      endvol = (endvol * _digi_volume) / 255;

   if (virt_voice[voice].num >= 0) {
      if (digi_driver->ramp_volume) {
	 digi_driver->ramp_volume(virt_voice[voice].num, time, endvol);
      }
      else {
	 int d = (endvol << 12) - _phys_voice[virt_voice[voice].num].vol;
	 time = MAX(time * SWEEP_FREQ / 1000, 1);
	 _phys_voice[virt_voice[voice].num].target_vol = endvol << 12;
	 _phys_voice[virt_voice[voice].num].dvol = d / time;
      }
   }
}

END_OF_FUNCTION(voice_ramp_volume);



/* voice_stop_volumeramp:
 *  Ends a volume ramp operation.
 */
void voice_stop_volumeramp(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].num >= 0) {
      _phys_voice[virt_voice[voice].num].dvol = 0;

      if (digi_driver->stop_volume_ramp)
	 digi_driver->stop_volume_ramp(virt_voice[voice].num);
   }
}

END_OF_FUNCTION(voice_stop_volumeramp);



/* voice_get_frequency:
 *  Returns the current frequency of a voice, or -1 if that cannot
 *  be determined (because it has finished or been preempted by a
 *  different sound).
 */
int voice_get_frequency(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].num >= 0)
      return digi_driver->get_frequency(virt_voice[voice].num);
   else
      return -1;
}

END_OF_FUNCTION(voice_get_frequency);



/* voice_set_frequency:
 *  Sets the pitch of a voice.
 */
void voice_set_frequency(int voice, int frequency)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   ASSERT(frequency > 0);
   
   if (virt_voice[voice].num >= 0) {
      _phys_voice[virt_voice[voice].num].freq = frequency << 12;
      _phys_voice[virt_voice[voice].num].dfreq = 0;

      digi_driver->set_frequency(virt_voice[voice].num, frequency);
   }
}

END_OF_FUNCTION(voice_set_frequency);



/* voice_sweep_frequency:
 *  Begins a frequency sweep (glissando) operation.
 */
void voice_sweep_frequency(int voice, int time, int endfreq)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   ASSERT(endfreq > 0);
   ASSERT(time >= 0);
   
   if (virt_voice[voice].num >= 0) {
      if (digi_driver->sweep_frequency) {
	 digi_driver->sweep_frequency(virt_voice[voice].num, time, endfreq);
      }
      else {
	 int d = (endfreq << 12) - _phys_voice[virt_voice[voice].num].freq;
	 time = MAX(time * SWEEP_FREQ / 1000, 1);
	 _phys_voice[virt_voice[voice].num].target_freq = endfreq << 12;
	 _phys_voice[virt_voice[voice].num].dfreq = d / time;
      }
   }
}

END_OF_FUNCTION(voice_sweep_frequency);



/* voice_stop_frequency_sweep:
 *  Ends a frequency sweep.
 */
void voice_stop_frequency_sweep(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].num >= 0) {
      _phys_voice[virt_voice[voice].num].dfreq = 0;

      if (digi_driver->stop_frequency_sweep)
	 digi_driver->stop_frequency_sweep(virt_voice[voice].num);
   }
}

END_OF_FUNCTION(voice_stop_frequency_sweep);



/* voice_get_pan:
 *  Returns the current pan position of a voice, or -1 if that cannot
 *  be determined (because it has finished or been preempted by a
 *  different sound).
 */
int voice_get_pan(int voice)
{
   int pan;
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);

   if (virt_voice[voice].num >= 0)
      pan = digi_driver->get_pan(virt_voice[voice].num);
   else
      pan = -1;

   if ((pan >= 0) && (_sound_flip_pan))
      pan = 255 - pan;

   return pan;
}

END_OF_FUNCTION(voice_get_pan);



/* voice_set_pan:
 *  Sets the pan position of a voice.
 */
void voice_set_pan(int voice, int pan)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   ASSERT(pan >= 0 && pan <= 255);
   if (_sound_flip_pan)
      pan = 255 - pan;

   if (virt_voice[voice].num >= 0) {
      _phys_voice[virt_voice[voice].num].pan = pan << 12;
      _phys_voice[virt_voice[voice].num].dpan = 0;

      digi_driver->set_pan(virt_voice[voice].num, pan);
   }
}

END_OF_FUNCTION(voice_set_pan);



/* voice_sweep_pan:
 *  Begins a pan sweep (left <-> right movement) operation.
 */
void voice_sweep_pan(int voice, int time, int endpan)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   ASSERT(endpan >= 0 && endpan <= 255);
   ASSERT(time >= 0);
   
   if (_sound_flip_pan)
      endpan = 255 - endpan;

   if (virt_voice[voice].num >= 0) {
      if (digi_driver->sweep_pan) {
	 digi_driver->sweep_pan(virt_voice[voice].num, time, endpan);
      }
      else {
	 int d = (endpan << 12) - _phys_voice[virt_voice[voice].num].pan;
	 time = MAX(time * SWEEP_FREQ / 1000, 1);
	 _phys_voice[virt_voice[voice].num].target_pan = endpan << 12;
	 _phys_voice[virt_voice[voice].num].dpan = d / time;
      }
   }
}

END_OF_FUNCTION(voice_sweep_pan);



/* voice_stop_pan_sweep:
 *  Ends a pan sweep.
 */
void voice_stop_pan_sweep(int voice)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if (virt_voice[voice].num >= 0) {
      _phys_voice[virt_voice[voice].num].dpan = 0;

      if (digi_driver->stop_pan_sweep)
	 digi_driver->stop_pan_sweep(virt_voice[voice].num);
   }
}

END_OF_FUNCTION(voice_stop_pan_sweep);



/* voice_set_echo:
 *  Sets the echo parameters of a voice.
 */
void voice_set_echo(int voice, int strength, int delay)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if ((virt_voice[voice].num >= 0) && (digi_driver->set_echo))
      digi_driver->set_echo(virt_voice[voice].num, strength, delay);
}

END_OF_FUNCTION(voice_set_echo);



/* voice_set_tremolo:
 *  Sets the tremolo parameters of a voice.
 */
void voice_set_tremolo(int voice, int rate, int depth)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if ((virt_voice[voice].num >= 0) && (digi_driver->set_tremolo))
      digi_driver->set_tremolo(virt_voice[voice].num, rate, depth);
}

END_OF_FUNCTION(voice_set_tremolo);



/* voice_set_vibrato:
 *  Sets the vibrato parameters of a voice.
 */
void voice_set_vibrato(int voice, int rate, int depth)
{
   ASSERT(voice >= 0 && voice < VIRTUAL_VOICES);
   if ((virt_voice[voice].num >= 0) && (digi_driver->set_vibrato))
      digi_driver->set_vibrato(virt_voice[voice].num, rate, depth);
}

END_OF_FUNCTION(voice_set_vibrato);



/* update_sweeps:
 *  Timer callback routine used to implement volume/frequency/pan sweep 
 *  effects, for those drivers that can't do them directly.
 */
static void update_sweeps(void)
{
   int phys_voices, i;

   phys_voices = digi_driver->voices;
   if (midi_driver->max_voices < 0)
      phys_voices += midi_driver->voices;

   for (i=0; i<phys_voices; i++) {
      if (_phys_voice[i].num >= 0) {
	 /* update volume ramp */
	 if ((!digi_driver->ramp_volume) && (_phys_voice[i].dvol)) {
	    _phys_voice[i].vol += _phys_voice[i].dvol;

	    if (((_phys_voice[i].dvol > 0) && (_phys_voice[i].vol >= _phys_voice[i].target_vol)) ||
		((_phys_voice[i].dvol < 0) && (_phys_voice[i].vol <= _phys_voice[i].target_vol))) {
	       _phys_voice[i].vol = _phys_voice[i].target_vol;
	       _phys_voice[i].dvol = 0;
	    }

	    digi_driver->set_volume(i, _phys_voice[i].vol >> 12);
	 }

	 /* update frequency sweep */
	 if ((!digi_driver->sweep_frequency) && (_phys_voice[i].dfreq)) {
	    _phys_voice[i].freq += _phys_voice[i].dfreq;

	    if (((_phys_voice[i].dfreq > 0) && (_phys_voice[i].freq >= _phys_voice[i].target_freq)) ||
		((_phys_voice[i].dfreq < 0) && (_phys_voice[i].freq <= _phys_voice[i].target_freq))) {
	       _phys_voice[i].freq = _phys_voice[i].target_freq;
	       _phys_voice[i].dfreq = 0;
	    }

	    digi_driver->set_frequency(i, _phys_voice[i].freq >> 12);
	 }

	 /* update pan sweep */
	 if ((!digi_driver->sweep_pan) && (_phys_voice[i].dpan)) {
	    _phys_voice[i].pan += _phys_voice[i].dpan;

	    if (((_phys_voice[i].dpan > 0) && (_phys_voice[i].pan >= _phys_voice[i].target_pan)) ||
		((_phys_voice[i].dpan < 0) && (_phys_voice[i].pan <= _phys_voice[i].target_pan))) {
	       _phys_voice[i].pan = _phys_voice[i].target_pan;
	       _phys_voice[i].dpan = 0;
	    }

	    digi_driver->set_pan(i, _phys_voice[i].pan >> 12);
	 }
      }
   }
}

END_OF_STATIC_FUNCTION(update_sweeps);



/* get_sound_input_cap_bits:
 *  Recording capabilities: number of bits
 */
int get_sound_input_cap_bits(void)
{
   return digi_input_driver->rec_cap_bits;
}



/* get_sound_input_cap_stereo:
 *  Recording capabilities: stereo
 */
int get_sound_input_cap_stereo(void)
{
   return digi_input_driver->rec_cap_stereo;
}



/* get_sound_input_cap_rate:
 *  Recording capabilities: maximum sampling rate
 */
int get_sound_input_cap_rate(int bits, int stereo)
{
   if (digi_input_driver->rec_cap_rate)
      return digi_input_driver->rec_cap_rate(bits, stereo);
   else
      return 0;
}



/* get_sound_input_cap_parm:
 *  Checks whether given parameters can be set.
 */
int get_sound_input_cap_parm(int rate, int bits, int stereo)
{
   if (digi_input_driver->rec_cap_parm)
      return digi_input_driver->rec_cap_parm(rate, bits, stereo);
   else
      return 0;
}



/* set_sound_input_source:
 *  Selects the sampling source for audio recording.
 */
int set_sound_input_source(int source)
{
   if (digi_input_driver->rec_source)
      return digi_input_driver->rec_source(source);
   else
      return -1;
}



/* start_sound_input:
 *  Starts recording, and returns recording buffer size.
 */
int start_sound_input(int rate, int bits, int stereo)
{
   if (digi_input_driver->rec_start)
      return digi_input_driver->rec_start(rate, bits, stereo);
   else
      return 0;
}



/* stop_sound_input:
 *  Ends recording.
 */
void stop_sound_input(void)
{
   if (digi_input_driver->rec_stop)
      digi_input_driver->rec_stop();
}



/* read_sound_input:
 *  Reads audio data.
 */
int read_sound_input(void *buffer)
{
   if (digi_input_driver->rec_read)
      return digi_input_driver->rec_read(buffer);
   else
      return 0;
}

END_OF_FUNCTION(read_sound_input);



/* sound_lock_mem:
 *  Locks memory used by the functions in this file.
 */
static void sound_lock_mem(void)
{
   LOCK_VARIABLE(digi_none);
   LOCK_VARIABLE(_midi_none);
   LOCK_VARIABLE(digi_card);
   LOCK_VARIABLE(midi_card);
   LOCK_VARIABLE(digi_driver);
   LOCK_VARIABLE(midi_driver);
   LOCK_VARIABLE(digi_recorder);
   LOCK_VARIABLE(midi_recorder);
   LOCK_VARIABLE(virt_voice);
   LOCK_VARIABLE(_phys_voice);
   LOCK_VARIABLE(_digi_volume);
   LOCK_VARIABLE(_midi_volume);
   LOCK_VARIABLE(_sound_flip_pan);
   LOCK_VARIABLE(_sound_hq);
   LOCK_FUNCTION(_dummy_detect);
   LOCK_FUNCTION(play_sample);
   LOCK_FUNCTION(adjust_sample);
   LOCK_FUNCTION(stop_sample);
   LOCK_FUNCTION(allocate_voice);
   LOCK_FUNCTION(deallocate_voice);
   LOCK_FUNCTION(reallocate_voice);
   LOCK_FUNCTION(voice_start);
   LOCK_FUNCTION(voice_stop);
   LOCK_FUNCTION(voice_set_priority);
   LOCK_FUNCTION(voice_check);
   LOCK_FUNCTION(voice_get_position);
   LOCK_FUNCTION(voice_set_position);
   LOCK_FUNCTION(voice_set_playmode);
   LOCK_FUNCTION(voice_get_volume);
   LOCK_FUNCTION(voice_set_volume);
   LOCK_FUNCTION(voice_ramp_volume);
   LOCK_FUNCTION(voice_stop_volumeramp);
   LOCK_FUNCTION(voice_get_frequency);
   LOCK_FUNCTION(voice_set_frequency);
   LOCK_FUNCTION(voice_sweep_frequency);
   LOCK_FUNCTION(voice_stop_frequency_sweep);
   LOCK_FUNCTION(voice_get_pan);
   LOCK_FUNCTION(voice_set_pan);
   LOCK_FUNCTION(voice_sweep_pan);
   LOCK_FUNCTION(voice_stop_pan_sweep);
   LOCK_FUNCTION(voice_set_echo);
   LOCK_FUNCTION(voice_set_tremolo);
   LOCK_FUNCTION(voice_set_vibrato);
   LOCK_FUNCTION(update_sweeps);
   LOCK_FUNCTION(read_sound_input);
}

