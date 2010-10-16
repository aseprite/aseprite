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
 *      AWE32/EMU8000 driver for the MIDI player.
 *
 *      By George Foot.
 *
 *      Modified by J. Flynn to remove floating point log calculations
 *      during interrupt - replaced by precomputed lookup table.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"
#include "emu8k.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* variables from awedata.c, containing the envelope data from synthgm.sf2 */
extern short int _awe_sf_defaults[];
extern int _awe_sf_num_presets;
extern short int _awe_sf_presets[];
extern short int _awe_sf_splits[];
extern short int _awe_sf_gens[];
extern int _awe_sf_sample_data[];


/* external interface to the AWE32 driver */
static int awe32_detect(int input);
static int awe32_init(int input, int voices);
static void awe32_exit(int input);
static void awe32_key_on(int inst, int note, int bend, int vol, int pan);
static void awe32_key_off(int voice);
static void _awe32_do_note(int inst, int note, int bend, int vol, int pan);
static void awe32_set_volume(int voice, int vol);
static void awe32_set_pitch(int voice, int note, int bend);
static void translate_soundfont_into_something_useful(void);
static void destroy_useful_version_of_soundfont(void);


static struct midi_preset_t {   /* struct to hold envelope data for each preset */
   int num_splits;              /* number of splits in this preset */
   struct envparms_t **split;   /* array of num_splits pointers to envelope data */
} *midi_preset;                 /* global variable to hold the data */


static struct envparms_t **voice_envelope;   /* array of pointers pointing at the envelope playing on each voice */
static int *exclusive_class_info;            /* exclusive class information */


static const unsigned char attentbl[] =      /* logarithm table */
{
   255, 255, 221, 199, 184, 172, 162, 154, 147, 141, 135, 130, 125, 121, 117, 113,
   110, 107, 104, 101, 98, 95, 93, 91, 88, 86, 84, 82, 80, 78, 76, 75,
   73, 71, 70, 68, 67, 65, 64, 62, 61, 60, 59, 57, 56, 55, 54, 53,
   51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 39, 38, 37,
   36, 35, 34, 34, 33, 32, 31, 31, 30, 29, 28, 28, 27, 26, 25, 25,
   24, 23, 23, 22, 22, 21, 20, 20, 19, 18, 18, 17, 17, 16, 16, 15,
   14, 14, 13, 13, 12, 12, 11, 11, 10, 10, 9, 9, 8, 8, 7, 7,
   6, 6, 5, 5, 4, 4, 3, 3, 3, 2, 2, 1, 1, 0, 0, 0,
};


static char awe32_desc[256] = EMPTY_STRING;



MIDI_DRIVER midi_awe32 =
{
   MIDI_AWE32,                  /* id */
   empty_string,                /* name */
   empty_string,                /* desc */
   "AWE32/EMU8000",             /* ASCII name */
   32, 0, 32, 32, -1, -1,       /* voices, basevoice, max_voices, def_voices, xmin, xmax */
   awe32_detect,                /* detect */
   awe32_init,                  /* init */
   awe32_exit,                  /* exit */
   NULL,                        /* mixer_set_volume */
   NULL,                        /* mixer_get_volume */
   NULL,                        /* raw_midi */
   _dummy_load_patches,         /* load_patches */
   _dummy_adjust_patches,       /* adjust_patches */
   awe32_key_on,                /* key_on */
   awe32_key_off,               /* key_off */
   awe32_set_volume,            /* set_volume */
   awe32_set_pitch,             /* set_pitch */
   _dummy_noop2,                /* set_pan */
   _dummy_noop2                 /* set_vibrato */
};



/* awe32_key_on:
 *  Triggers the specified voice. The instrument is specified as a GM
 *  patch number, pitch as a midi note number, and volume from 0-127.
 *  The bend parameter is _not_ expressed as a midi pitch bend value.
 *  It ranges from 0 (no pitch change) to 0xFFF (almost a semitone sharp).
 *  Drum sounds are indicated by passing an instrument number greater than
 *  128, in which case the sound is GM percussion key #(inst-128).
 */
static void awe32_key_on(int inst, int note, int bend, int vol, int pan)
{
   if (inst > 127) {            /* drum sound? */
      _awe32_do_note(128, inst - 128, bend, vol, pan);
   }
   else {                       /* regular instrument */
      _awe32_do_note(inst, note, bend, vol, pan);
   }
}

END_OF_STATIC_FUNCTION(awe32_key_on);



/* _awe32_do_note:
 *  Actually plays the note as described above; the above function just remaps
 *  the drums.
 */
static void _awe32_do_note(int inst, int note, int bend, int vol, int pan)
{
   int voice;
   int i;
   envparms_t *env;
   int key, vel;
   int atten;
   int pan_pos;

   /* EMU8000 pan is back-to-front and twice the scale */
   pan = 0x100 - 2 * pan;
   if (pan > 0xff)
      pan = 0xff;
   if (pan < 0x00)
      pan = 0x00;

   for (i=0; i<midi_preset[inst].num_splits; i++) {
      /* envelope for this split */
      env = midi_preset[inst].split[i];

      /* should we play this split? */
      if ((note >= env->minkey) && (note <= env->maxkey) && (vol >= env->minvel) && (vol <= env->maxvel)) {

	 /* get a voice (any voice) to play it on */
	 voice = _midi_allocate_voice(-1, -1);

	 /* did we get one? */
	 if (voice >= 0) {

	    /* set the current envelope for this voice */
	    voice_envelope[voice] = env;

	    /* set pitch and velocity */
	    key = note * 0x1000 + bend;
	    vel = vol;

	    /* override key and velocity if envelope says so */
	    if ((env->key >= 0) && (env->key <= 127))
	       key = env->key * 0x1000;
	    if ((env->vel >= 0) && (env->vel <= 127))
	       vel = env->vel;

	    /* check key and velocity numbers are within range */
	    if (key > 0x7ffff)
	       key = 0x7ffff;
	    if (key < 0)
	       key = 0;
	    if (vel > 127)
	       vel = 127;
	    if (vel < 0)
	       vel = 0;

	    /* add one-off information to the envelope (these have no side-effects on the other voices using this envelope) */
	    env->ip = env->ipbase + (env->ipscale * key) / 1200;

	    /* remap MIDI velocity to attenuation */
	    if (vel)
	       atten = env->atten + attentbl[vel];
	    else
	       atten = 0xff;
	    if (atten < 0x00)
	       atten = 0x00;
	    if (atten > 0xff)
	       atten = 0xff;

	    /* update it in the envelope */
	    env->ifatn = env->filter + atten;

	    /* modify pan with envelope's built-in pan */
	    if (pan < 0x80) {
	       pan_pos = (pan * env->pan) / 0x80;
	    }
	    else {
	       pan_pos = env->pan + (pan - 0x80) * (256 - env->pan) / 0x80;
	    }
	    if (pan_pos < 0x00)
	       pan_pos = 0x00;
	    if (pan_pos > 0xff)
	       pan_pos = 0xff;

	    /* update pan in the envelope */
	    env->psst = (pan_pos << 24) + env->loopst;

	    /* test exclusive class */
	    exclusive_class_info[voice] = (inst << 8) + env->exc;

	    if (env->exc) {
	       int chan;
	       for (chan=0; chan<32; chan++)
		  if ((chan != voice) && (exclusive_class_info[chan] == exclusive_class_info[voice]))
		     emu8k_terminatesound(chan);
	    }

	    /* start the note playing */
	    emu8k_startsound(voice, env);
	 }
      }
   }
}

END_OF_STATIC_FUNCTION(_awe32_do_note);



/* awe32_set_*:
 *  Modulation routines
 */
static void awe32_set_volume(int voice, int vol)
{
   int atten;
   struct envparms_t *env;

   /* get envelope in use on this voice */
   env = voice_envelope[voice];

   /* allow envelope to override new volume */
   if ((env->vel >= 0) && (env->vel <= 127))
      vol = env->vel;

   /* check velocity number is within range */
   if (vol > 127)
      vol = 127;
   if (vol < 0)
      vol = 0;

   /* remap MIDI velocity to attenuation */
   if (vol)
      atten = env->atten + attentbl[vol];
   else
      atten = 0xff;
   if (atten < 0x00)
      atten = 0x00;
   if (atten > 0xff)
      atten = 0xff;

   emu8k_modulate_atten(voice, atten);
}

END_OF_STATIC_FUNCTION(awe32_set_volume);



static void awe32_set_pitch(int voice, int note, int bend)
{
   struct envparms_t *env;
   int key, ip;

   /* get envelope in use on this voice */
   env = voice_envelope[voice];

   key = note * 0x1000 + bend;

   /* override key if envelope says so */
   if ((env->key >= 0) && (env->key <= 127))
      key = env->key * 0x1000;

   /* check key number is within range */
   if (key > 0x7ffff)
      key = 0x7ffff;
   if (key < 0)
      key = 0;

   ip = env->ipbase + (env->ipscale * key) / 1200;

   emu8k_modulate_ip(voice, ip);
}

END_OF_STATIC_FUNCTION(awe32_set_pitch);



/* awe32_key_off:
 *  Hey, guess what this does :-)
 */
static void awe32_key_off(int voice)
{
   emu8k_releasesound(voice, voice_envelope[voice]);
}

END_OF_STATIC_FUNCTION(awe32_key_off);



/* awe32_detect:
 *  AWE32/EMU8000 detection routine.
 */
static int awe32_detect(int input)
{
   if (input)
      return FALSE;

   if (emu8k_detect()) {
      uszprintf(awe32_desc, sizeof(awe32_desc), get_config_text("SB AWE32/compatible on port 0x%04x"), _emu8k_baseport);
      midi_awe32.desc = awe32_desc;
      return TRUE;
   }

   ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("AWE32 not detected"));
   return FALSE;
}



/* awe32_lockmem:
 *  Locks required memory blocks
 */
static void awe32_lockmem(void)
{
   /* functions */
   LOCK_FUNCTION(awe32_key_on);
   LOCK_FUNCTION(awe32_key_off);
   LOCK_FUNCTION(_awe32_do_note);
   LOCK_FUNCTION(awe32_set_volume);
   LOCK_FUNCTION(awe32_set_pitch);

   /* data */
   LOCK_VARIABLE(midi_preset);
   LOCK_VARIABLE(voice_envelope);
   LOCK_VARIABLE(exclusive_class_info);
   LOCK_VARIABLE(midi_awe32);
   LOCK_VARIABLE(attentbl);

   /* stuff in emu8k.c */
   emu8k_lock();
}



/* awe32_init:
 *  Setup the AWE32/EMU8000 driver.
 */
static int awe32_init(int input, int voices)
{
   int chan;

   emu8k_init();
   translate_soundfont_into_something_useful();
   voice_envelope = (struct envparms_t **)_lock_malloc(32 * sizeof(struct envparms_t *));
   exclusive_class_info = (int *)_lock_malloc(32 * sizeof(int));
   awe32_lockmem();

   for (chan=0; chan<32; chan++) {
      voice_envelope[chan] = NULL;
      exclusive_class_info[chan] = 0;
   }

   return 0;
}



/* awe32_exit:
 *  Cleanup when we are finished.
 */
static void awe32_exit(int input)
{
   int i;

   for (i=0; i<_emu8k_numchannels; i++)
      emu8k_terminatesound(i);

   destroy_useful_version_of_soundfont();
   _AL_FREE(voice_envelope);
   _AL_FREE(exclusive_class_info);
}



/* translate_soundfont_into_something_useful:
 *  Like it says, translate the soundfont data into something we can use
 *  when playing notes.
 */
static void translate_soundfont_into_something_useful(void)
{
   int p, s, gen, weirdo;
   struct midi_preset_t *thing_to_write = NULL;
   generators_t temp_gens;
   int global_split = 0, global_weirdo = 0, num_weirdos;

   midi_preset = (struct midi_preset_t *)_lock_malloc(129 * sizeof(struct midi_preset_t));

   for (p=0; p<_awe_sf_num_presets; p++) {
      if (_awe_sf_presets[p * 3 + 1] == 0) {
	 thing_to_write = &midi_preset[_awe_sf_presets[p * 3 + 0]];
      }
      else if (_awe_sf_presets[p * 3 + 1] == 128) {
	 thing_to_write = &midi_preset[128];
      }
      else {
	 thing_to_write = NULL;
      }
      if (thing_to_write) {
	 thing_to_write->num_splits = _awe_sf_presets[p * 3 + 2];
	 thing_to_write->split = (struct envparms_t **)_lock_malloc(thing_to_write->num_splits * sizeof(struct envparms_t *));
	 for (s=0; s<thing_to_write->num_splits; s++) {
	    for (gen=0; gen<SOUNDFONT_NUM_GENERATORS - 4; gen++)
	       temp_gens[gen] = _awe_sf_defaults[gen];
	    num_weirdos = _awe_sf_splits[global_split];
	    for (weirdo=global_weirdo; weirdo<global_weirdo+num_weirdos; weirdo++)
	       temp_gens[_awe_sf_gens[weirdo * 2]] = _awe_sf_gens[weirdo * 2 + 1];
	    global_weirdo += num_weirdos;
	    for (gen=0; gen<4; gen++)
	       temp_gens[gfgen_startAddrs + gen] = _awe_sf_sample_data[global_split * 4 + gen];
	    global_split++;
	    thing_to_write->split[s] = emu8k_createenvelope(temp_gens);
	 }
      }
      else {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("AWE32 driver: had trouble with the embedded data"));
      }
   }
}



/* destroy_useful_version_of_soundfont:
 *  Destroys the data created by the above function
 */
static void destroy_useful_version_of_soundfont(void)
{
   int p, s;

   for (p=0; p<129; p++) {
      if (midi_preset[p].num_splits > 0) {
	 for (s=0; s<midi_preset[p].num_splits; s++)
	    emu8k_destroyenvelope(midi_preset[p].split[s]);
	 _AL_FREE(midi_preset[p].split);
      }
   }

   _AL_FREE(midi_preset);
}



/* _lock_malloc:
 *  Allocates a locked memory block
 */
void *_lock_malloc(size_t size)
{
   void *ret = _AL_MALLOC(size);

   if (!ret)
      return NULL;

   LOCK_DATA(ret, size);

   return ret;
}

