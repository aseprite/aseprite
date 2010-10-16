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
 *      ALSA RawMIDI Sound driver.
 *
 *      By Thomas Fjellstrom.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"

#if (defined ALLEGRO_WITH_ALSAMIDI) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#include "allegro/internal/aintern.h"

#ifdef ALLEGRO_QNX
#include "allegro/platform/aintqnx.h"
#else
#include "allegro/platform/aintunix.h"
#endif

#ifndef SCAN_DEPEND
   #include <stdlib.h>
   #include <stdio.h>
   #include <string.h>
   #include <errno.h>

   #if ALLEGRO_ALSA_VERSION == 9
      #define ALSA_PCM_NEW_HW_PARAMS_API 1
      #include <alsa/asoundlib.h>
   #else  /* ALLEGRO_ALSA_VERSION == 5 */
      #include <sys/asoundlib.h>
   #endif

#endif


#define ALSA_RAWMIDI_MAX_ERRORS  3


static int alsa_rawmidi_detect(int input);
static int alsa_rawmidi_init(int input, int voices);
static void alsa_rawmidi_exit(int input);
static void alsa_rawmidi_output(int data);

static char alsa_rawmidi_desc[256];

static snd_rawmidi_t *rawmidi_handle = NULL;
static int alsa_rawmidi_errors = 0;


MIDI_DRIVER midi_alsa =
{
   MIDI_ALSA,                /* id */
   empty_string,             /* name */
   empty_string,             /* desc */
   "ALSA RawMIDI",           /* ASCII name */
   0,                        /* voices */
   0,                        /* basevoice */
   0xFFFF,                   /* max_voices */
   0,                        /* def_voices */
   -1,                       /* xmin */
   -1,                       /* xmax */
   alsa_rawmidi_detect,      /* detect */
   alsa_rawmidi_init,        /* init */
   alsa_rawmidi_exit,        /* exit */
   NULL,                     /* set_mixer_volume */
   NULL,                     /* get_mixer_volume */
   alsa_rawmidi_output,      /* raw_midi */
   _dummy_load_patches,      /* load_patches */
   _dummy_adjust_patches,    /* adjust_patches */
   _dummy_key_on,            /* key_on */
   _dummy_noop1,             /* key_off */
   _dummy_noop2,             /* set_volume */
   _dummy_noop3,             /* set_pitch */
   _dummy_noop2,             /* set_pan */
   _dummy_noop2              /* set_vibrato */
};



/* alsa_rawmidi_detect:
 *  ALSA RawMIDI detection.
 */
static int alsa_rawmidi_detect(int input)
{
#if ALLEGRO_ALSA_VERSION == 9
   const char *device = NULL;
#else  /* ALLEGRO_ALSA_VERSION == 5 */
   int card = -1;
   int device = -1;
#endif
   int ret = FALSE, err;
   char tmp1[128], tmp2[128], temp[256];
   snd_rawmidi_t *handle = NULL;

   if (input) {
      ret = FALSE;
   }
   else {
#if ALLEGRO_ALSA_VERSION == 9
      device = get_config_string(uconvert_ascii("sound", tmp1),
				 uconvert_ascii("alsa_rawmidi_device", tmp2),
				 "default");

      err = snd_rawmidi_open(NULL, &handle, device, 0);
#else  /* ALLEGRO_ALSA_VERSION == 5 */
      card = get_config_int(uconvert_ascii("sound", tmp1),
			    uconvert_ascii("alsa_rawmidi_card", tmp2),
			    snd_defaults_rawmidi_card());

      device = get_config_int(uconvert_ascii("sound", tmp1),
			      uconvert_ascii("alsa_rawmidi_device", tmp2),
			      snd_defaults_rawmidi_device());

      err = snd_rawmidi_open(&handle, card, device, SND_RAWMIDI_OPEN_OUTPUT_APPEND);
#endif
      if (err) {
	 snprintf(temp, sizeof(temp), "Could not open card/rawmidi device: %s", snd_strerror(err));
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(temp));
	 ret = FALSE;
      }
      else {
	 snd_rawmidi_close(handle);
	 ret = TRUE;
      }
   }

   return ret;
}



/* alsa_rawmidi_init:
 *  Inits the ALSA RawMIDI interface.
 */
static int alsa_rawmidi_init(int input, int voices)
{
   int ret = -1, err;
   char tmp1[128], tmp2[128], temp[256];
#if ALLEGRO_ALSA_VERSION == 9
   snd_rawmidi_info_t *info;
   const char *device = NULL;
#else  /* ALLEGRO_ALSA_VERSION == 5 */
   snd_rawmidi_info_t info;
   int card = -1;
   int device = -1;
#endif

   if (input) {
      ret = -1;
   }
   else {
#if ALLEGRO_ALSA_VERSION == 9
      device = get_config_string(uconvert_ascii("sound", tmp1),
				 uconvert_ascii("alsa_rawmidi_device", tmp2),
				 "default");

      err = snd_rawmidi_open(NULL, &rawmidi_handle, device, 0);
#else  /* ALLEGRO_ALSA_VERSION == 5 */
      card = get_config_int(uconvert_ascii("sound", tmp1),
			    uconvert_ascii("alsa_rawmidi_card", tmp2),
			    snd_defaults_rawmidi_card());

      device = get_config_int(uconvert_ascii("sound", tmp1),
			     uconvert_ascii("alsa_rawmidi_device", tmp2),
			     snd_defaults_rawmidi_device());

      err = snd_rawmidi_open(&rawmidi_handle, card, device, SND_RAWMIDI_OPEN_OUTPUT_APPEND);
#endif
      if (err) {
	 snprintf(temp, sizeof(temp), "Could not open card/rawmidi device: %s", snd_strerror(err));
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text(temp));
	 ret = -1;
      }

      ret = 0;
   }

   if (rawmidi_handle) {
#if ALLEGRO_ALSA_VERSION == 9
      snd_rawmidi_nonblock(rawmidi_handle, 0);
      snd_rawmidi_info_malloc(&info);
      snd_rawmidi_info(rawmidi_handle, info);
      _al_sane_strncpy(alsa_rawmidi_desc, snd_rawmidi_info_get_name(info), sizeof(alsa_rawmidi_desc));
#else  /* ALLEGRO_ALSA_VERSION == 5 */
      snd_rawmidi_block_mode(rawmidi_handle, 1);
      snd_rawmidi_info(rawmidi_handle, &info);
      _al_sane_strncpy(alsa_rawmidi_desc, info.name, sizeof(alsa_rawmidi_desc));
#endif
      midi_alsa.desc = alsa_rawmidi_desc;
      alsa_rawmidi_errors = 0;
   }

   return ret;
}



/* alsa_rawmidi_exit:
 *   Cleans up.
 */
static void alsa_rawmidi_exit(int input)
{
   if (rawmidi_handle) {
#if ALLEGRO_ALSA_VERSION == 9
      snd_rawmidi_drain(rawmidi_handle);
#else  /* ALLEGRO_ALSA_VERSION == 5 */
      snd_rawmidi_output_drain(rawmidi_handle);
#endif
      snd_rawmidi_close(rawmidi_handle);
   }

   rawmidi_handle = NULL;
}



/* alsa_rawmidi_output:
 *   Outputs MIDI data.
 */
static void alsa_rawmidi_output(int data)
{
   int err;

   /* If there are too many errors, just give up.  Otherwise the calling thread
    * can end up consuming CPU time for no reason.  It probably means the user
    * hasn't configured ALSA properly.
    */
   if (alsa_rawmidi_errors > ALSA_RAWMIDI_MAX_ERRORS) {
      return;
   }

   err = snd_rawmidi_write(rawmidi_handle, &data, sizeof(char));
   if (err) {
      alsa_rawmidi_errors++;
      if (alsa_rawmidi_errors == ALSA_RAWMIDI_MAX_ERRORS) {
	  TRACE("al-alsamidi: too many errors, giving up\n");
      }
   }
}



#ifdef ALLEGRO_MODULE

/* _module_init:
 *   Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   _unix_register_midi_driver(MIDI_ALSA, &midi_alsa, TRUE, TRUE);
}

#endif /* ALLEGRO_MODULE */

#endif /* MIDI_ALSA */

