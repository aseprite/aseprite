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
 *      MIDI driver for BeOS.
 *
 *      By Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif                

MIDI_DRIVER midi_beos =
{
   MIDI_BEOS,               /* driver ID code */
   empty_string,            /* driver name */
   empty_string,            /* description string */
   "BeOS MIDI",             /* ASCII format name string */
   16,                      /* available voices */
   0,                       /* voice number offset */
   28,                      /* maximum voices we can support */
   0,                       /* default number of voices to use */
   10, 10,                  /* reserved voice range */
   be_midi_detect,          /* AL_METHOD(int,  detect, (int input)); */
   be_midi_init,            /* AL_METHOD(int,  init, (int input, int voices)); */
   be_midi_exit,            /* AL_METHOD(void, exit, (int input)); */
   be_midi_set_mixer_volume,/* AL_METHOD(int,  set_mixer_volume, (int volume)); */
   be_midi_get_mixer_volume,/* AL_METHOD(int,  get_mixer_volume, (void)); */
   NULL,                    /* AL_METHOD(void, raw_midi, (int data)); */
   _dummy_load_patches,     /* AL_METHOD(int,  load_patches, (AL_CONST char *patches, AL_CONST char *drums)); */
   _dummy_adjust_patches,   /* AL_METHOD(void, adjust_patches, (AL_CONST char *patches, AL_CONST char *drums)); */
   be_midi_key_on,          /* AL_METHOD(void, key_on, (int inst, int note, int bend, int vol, int pan)); */
   be_midi_key_off,         /* AL_METHOD(void, key_off, (int voice)); */
   be_midi_set_volume,      /* AL_METHOD(void, set_volume, (int voice, int vol)); */
   be_midi_set_pitch,       /* AL_METHOD(void, set_pitch, (int voice, int note, int bend)); */
   be_midi_set_pan,         /* AL_METHOD(void, set_pan, (int voice, int pan)); */
   _dummy_noop2,            /* AL_METHOD(void, set_vibrato, (int voice, int amount)); */
};
