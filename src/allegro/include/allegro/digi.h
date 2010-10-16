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
 *      Digital sound routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_DIGI_H
#define ALLEGRO_DIGI_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct PACKFILE;       


#define DIGI_VOICES           64       /* Theoretical maximums: */
                                       /* actual drivers may not be */
                                       /* able to handle this many */

typedef struct SAMPLE                  /* a sample */
{
   int bits;                           /* 8 or 16 */
   int stereo;                         /* sample type flag */
   int freq;                           /* sample frequency */
   int priority;                       /* 0-255 */
   unsigned long len;                  /* length (in samples) */
   unsigned long loop_start;           /* loop start position */
   unsigned long loop_end;             /* loop finish position */
   unsigned long param;                /* for internal use by the driver */
   void *data;                         /* sample data */
} SAMPLE;


#define DIGI_AUTODETECT       -1       /* for passing to install_sound() */
#define DIGI_NONE             0

typedef struct DIGI_DRIVER             /* driver for playing digital sfx */
{
   int  id;                            /* driver ID code */
   AL_CONST char *name;                /* driver name */
   AL_CONST char *desc;                /* description string */
   AL_CONST char *ascii_name;          /* ASCII format name string */
   int  voices;                        /* available voices */
   int  basevoice;                     /* voice number offset */
   int  max_voices;                    /* maximum voices we can support */
   int  def_voices;                    /* default number of voices to use */

   /* setup routines */
   AL_METHOD(int,  detect, (int input));
   AL_METHOD(int,  init, (int input, int voices));
   AL_METHOD(void, exit, (int input));
   AL_METHOD(int,  set_mixer_volume, (int volume));
   AL_METHOD(int,  get_mixer_volume, (void));

   /* for use by the audiostream functions */
   AL_METHOD(void *, lock_voice, (int voice, int start, int end));
   AL_METHOD(void, unlock_voice, (int voice));
   AL_METHOD(int,  buffer_size, (void));

   /* voice control functions */
   AL_METHOD(void, init_voice, (int voice, AL_CONST SAMPLE *sample));
   AL_METHOD(void, release_voice, (int voice));
   AL_METHOD(void, start_voice, (int voice));
   AL_METHOD(void, stop_voice, (int voice));
   AL_METHOD(void, loop_voice, (int voice, int playmode));

   /* position control functions */
   AL_METHOD(int,  get_position, (int voice));
   AL_METHOD(void, set_position, (int voice, int position));

   /* volume control functions */
   AL_METHOD(int,  get_volume, (int voice));
   AL_METHOD(void, set_volume, (int voice, int volume));
   AL_METHOD(void, ramp_volume, (int voice, int tyme, int endvol));
   AL_METHOD(void, stop_volume_ramp, (int voice));

   /* pitch control functions */
   AL_METHOD(int,  get_frequency, (int voice));
   AL_METHOD(void, set_frequency, (int voice, int frequency));
   AL_METHOD(void, sweep_frequency, (int voice, int tyme, int endfreq));
   AL_METHOD(void, stop_frequency_sweep, (int voice));

   /* pan control functions */
   AL_METHOD(int,  get_pan, (int voice));
   AL_METHOD(void, set_pan, (int voice, int pan));
   AL_METHOD(void, sweep_pan, (int voice, int tyme, int endpan));
   AL_METHOD(void, stop_pan_sweep, (int voice));

   /* effect control functions */
   AL_METHOD(void, set_echo, (int voice, int strength, int delay));
   AL_METHOD(void, set_tremolo, (int voice, int rate, int depth));
   AL_METHOD(void, set_vibrato, (int voice, int rate, int depth));

   /* input functions */
   int rec_cap_bits;
   int rec_cap_stereo;
   AL_METHOD(int,  rec_cap_rate, (int bits, int stereo));
   AL_METHOD(int,  rec_cap_parm, (int rate, int bits, int stereo));
   AL_METHOD(int,  rec_source, (int source));
   AL_METHOD(int,  rec_start, (int rate, int bits, int stereo));
   AL_METHOD(void, rec_stop, (void));
   AL_METHOD(int,  rec_read, (void *buf));
} DIGI_DRIVER;

AL_ARRAY(_DRIVER_INFO, _digi_driver_list);

/* macros for constructing the driver lists */
#define BEGIN_DIGI_DRIVER_LIST                                 \
   _DRIVER_INFO _digi_driver_list[] =                          \
   {

#define END_DIGI_DRIVER_LIST                                   \
      {  0,                NULL,                0     }        \
   };

AL_VAR(DIGI_DRIVER *, digi_driver);

AL_VAR(DIGI_DRIVER *, digi_input_driver);

AL_VAR(int, digi_card);

AL_VAR(int, digi_input_card);

AL_FUNC(int, detect_digi_driver, (int driver_id));


AL_FUNC(SAMPLE *, load_sample, (AL_CONST char *filename));
AL_FUNC(SAMPLE *, load_wav, (AL_CONST char *filename));
AL_FUNC(SAMPLE *, load_wav_pf, (struct PACKFILE *f));
AL_FUNC(SAMPLE *, load_voc, (AL_CONST char *filename));
AL_FUNC(SAMPLE *, load_voc_pf, (struct PACKFILE *f));
AL_FUNC(int, save_sample, (AL_CONST char *filename, SAMPLE *spl));
AL_FUNC(SAMPLE *, create_sample, (int bits, int stereo, int freq, int len));
AL_FUNC(void, destroy_sample, (SAMPLE *spl));

AL_FUNC(int, play_sample, (AL_CONST SAMPLE *spl, int vol, int pan, int freq, int loop));
AL_FUNC(void, stop_sample, (AL_CONST SAMPLE *spl));
AL_FUNC(void, adjust_sample, (AL_CONST SAMPLE *spl, int vol, int pan, int freq, int loop));

AL_FUNC(int, allocate_voice, (AL_CONST SAMPLE *spl));
AL_FUNC(void, deallocate_voice, (int voice));
AL_FUNC(void, reallocate_voice, (int voice, AL_CONST SAMPLE *spl));
AL_FUNC(void, release_voice, (int voice));
AL_FUNC(void, voice_start, (int voice));
AL_FUNC(void, voice_stop, (int voice));
AL_FUNC(void, voice_set_priority, (int voice, int priority));
AL_FUNC(SAMPLE *, voice_check, (int voice));

#define PLAYMODE_PLAY           0
#define PLAYMODE_LOOP           1
#define PLAYMODE_FORWARD        0
#define PLAYMODE_BACKWARD       2
#define PLAYMODE_BIDIR          4

AL_FUNC(void, voice_set_playmode, (int voice, int playmode));

AL_FUNC(int, voice_get_position, (int voice));
AL_FUNC(void, voice_set_position, (int voice, int position));

AL_FUNC(int, voice_get_volume, (int voice));
AL_FUNC(void, voice_set_volume, (int voice, int volume));
AL_FUNC(void, voice_ramp_volume, (int voice, int tyme, int endvol));
AL_FUNC(void, voice_stop_volumeramp, (int voice));

AL_FUNC(int, voice_get_frequency, (int voice));
AL_FUNC(void, voice_set_frequency, (int voice, int frequency));
AL_FUNC(void, voice_sweep_frequency, (int voice, int tyme, int endfreq));
AL_FUNC(void, voice_stop_frequency_sweep, (int voice));

AL_FUNC(int, voice_get_pan, (int voice));
AL_FUNC(void, voice_set_pan, (int voice, int pan));
AL_FUNC(void, voice_sweep_pan, (int voice, int tyme, int endpan));
AL_FUNC(void, voice_stop_pan_sweep, (int voice));

AL_FUNC(void, voice_set_echo, (int voice, int strength, int delay));
AL_FUNC(void, voice_set_tremolo, (int voice, int rate, int depth));
AL_FUNC(void, voice_set_vibrato, (int voice, int rate, int depth));

#define SOUND_INPUT_MIC    1
#define SOUND_INPUT_LINE   2
#define SOUND_INPUT_CD     3

AL_FUNC(int, get_sound_input_cap_bits, (void));
AL_FUNC(int, get_sound_input_cap_stereo, (void));
AL_FUNC(int, get_sound_input_cap_rate, (int bits, int stereo));
AL_FUNC(int, get_sound_input_cap_parm, (int rate, int bits, int stereo));
AL_FUNC(int, set_sound_input_source, (int source));
AL_FUNC(int, start_sound_input, (int rate, int bits, int stereo));
AL_FUNC(void, stop_sound_input, (void));
AL_FUNC(int, read_sound_input, (void *buffer));

AL_FUNCPTR(void, digi_recorder, (void));

AL_FUNC(void, lock_sample, (struct SAMPLE *spl));

AL_FUNC(void, register_sample_file_type, (AL_CONST char *ext, AL_METHOD(struct SAMPLE *, load, (AL_CONST char *filename)), AL_METHOD(int, save, (AL_CONST char *filename, struct SAMPLE *spl))));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_DIGI_H */


