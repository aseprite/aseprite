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
 *      Adlib/FM driver for the MIDI player.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintdos.h"

#ifndef ALLEGRO_DOS
   #error something is wrong with the makefile
#endif



/* external interface to the Adlib driver */
static int fm_detect(int input);
static int fm_init(int input, int voices);
static void fm_exit(int input);
static int fm_set_mixer_volume(int volume);
static int fm_load_patches(AL_CONST char *patches, AL_CONST char *drums);
static void fm_key_on(int inst, int note, int bend, int vol, int pan);
static void fm_key_off(int voice);
static void fm_set_volume(int voice, int vol);
static void fm_set_pitch(int voice, int note, int bend);

static char adlib_desc[256] = EMPTY_STRING;



MIDI_DRIVER midi_opl2 =
{
   MIDI_OPL2,
   empty_string, 
   empty_string,
   "Adlib (OPL2)",
   9, 0, 9, 9, -1, -1,
   fm_detect,
   fm_init,
   fm_exit,
   fm_set_mixer_volume,
   NULL,
   NULL,
   fm_load_patches,
   _dummy_adjust_patches,
   fm_key_on,
   fm_key_off,
   fm_set_volume,
   fm_set_pitch,
   _dummy_noop2,
   _dummy_noop2
};



MIDI_DRIVER midi_2xopl2 =
{
   MIDI_2XOPL2,
   empty_string, 
   empty_string,
   "Adlib (dual OPL2)",
   18, 0, 18, 18, -1, -1,
   fm_detect,
   fm_init,
   fm_exit,
   fm_set_mixer_volume,
   NULL,
   NULL,
   fm_load_patches,
   _dummy_adjust_patches,
   fm_key_on,
   fm_key_off,
   fm_set_volume,
   fm_set_pitch,
   _dummy_noop2,
   _dummy_noop2
};



MIDI_DRIVER midi_opl3 =
{
   MIDI_OPL3,
   empty_string, 
   empty_string,
   "Adlib (OPL3)",
   18, 0, 18, 18, -1, -1,
   fm_detect,
   fm_init,
   fm_exit,
   fm_set_mixer_volume,
   NULL,
   NULL,
   fm_load_patches,
   _dummy_adjust_patches,
   fm_key_on,
   fm_key_off,
   fm_set_volume,
   fm_set_pitch,
   _dummy_noop2,
   _dummy_noop2
};



#define FM_HH     1
#define FM_CY     2
#define FM_TT     4
#define FM_SD     8
#define FM_BD     16


/* include the GM patch set (static data) */
#include "../misc/fm_instr.h"
#include "../misc/fm_drum.h"


/* where to find the card */
int _fm_port = -1;


/* is the OPL in percussion mode? */
static int fm_drum_mode = FALSE;


/* delays when writing to OPL registers */
static int fm_delay_1 = 6;
static int fm_delay_2 = 35;


/* register offsets for each voice */
static int fm_offset[18] = {
   0x000, 0x001, 0x002, 0x008, 0x009, 0x00A, 0x010, 0x011, 0x012, 
   0x100, 0x101, 0x102, 0x108, 0x109, 0x10A, 0x110, 0x111, 0x112
};


/* for converting midi note numbers to FM frequencies */
static int fm_freq[13] = {
   0x157, 0x16B, 0x181, 0x198, 0x1B0, 0x1CA,
   0x1E5, 0x202, 0x220, 0x241, 0x263, 0x287, 0x2AE
};


/* logarithmic relationship between midi and FM volumes */
static int fm_vol_table[128] = {
   0,  11, 16, 19, 22, 25, 27, 29, 32, 33, 35, 37, 39, 40, 42, 43,
   45, 46, 48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
   64, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 75, 76, 77,
   78, 79, 80, 80, 81, 82, 83, 83, 84, 85, 86, 86, 87, 88, 89, 89,
   90, 91, 91, 92, 93, 93, 94, 95, 96, 96, 97, 97, 98, 99, 99, 100,
   101, 101, 102, 103, 103, 104, 104, 105, 106, 106, 107, 107, 108,
   109, 109, 110, 110, 111, 112, 112, 113, 113, 114, 114, 115, 115,
   116, 117, 117, 118, 118, 119, 119, 120, 120, 121, 121, 122, 122,
   123, 123, 124, 124, 125, 125, 126, 126, 127
};


/* drum channel tables:          BD       SD       TT       CY       HH    */
static int fm_drum_channel[] = { 6,       7,       8,       8,       7     };
static int fm_drum_op1[] =     { TRUE,    FALSE,   TRUE,    FALSE,   TRUE  };
static int fm_drum_op2[] =     { TRUE,    TRUE,    FALSE,   TRUE,    FALSE };


/* cached information about the state of the drum channels */
static FM_INSTRUMENT *fm_drum_cached_inst1[5];
static FM_INSTRUMENT *fm_drum_cached_inst2[5];
static int fm_drum_cached_vol1[5];
static int fm_drum_cached_vol2[5];
static long fm_drum_cached_time[5];


/* various bits of information about the current state of the FM chip */
static unsigned char fm_drum_mask;
static unsigned char fm_key[18];
static unsigned char fm_keyscale[18];
static unsigned char fm_feedback[18];
static int fm_level[18];
static int fm_patch[18];


#define VOICE_OFFSET(x)     ((x < 9) ? x : 0x100+x-9)



/* fm_write:
 *  Writes a byte to the specified register on the FM chip.
 */
static void fm_write(int reg, unsigned char data)
{
   int i;
   int port = (reg & 0x100) ? _fm_port+2 : _fm_port;

   outportb(port, reg & 0xFF);      /* write the register */

   for (i=0; i<fm_delay_1; i++)     /* delay */
      inportb(port);

   outportb(port+1, data);          /* write the data */

   for (i=0; i<fm_delay_2; i++)     /* delay */
      inportb(port);
}

END_OF_STATIC_FUNCTION(fm_write);



/* fm_reset:
 *  Resets the FM chip. If enable is set, puts OPL3 cards into OPL3 mode,
 *  otherwise puts them into OPL2 emulation mode.
 */
static void fm_reset(int enable)
{
   int i;

   for (i=0xF5; i>0; i--)
      fm_write(i, 0);

   if (midi_card == MIDI_OPL3) {          /* if we have an OPL3... */
      fm_delay_1 = 1;
      fm_delay_2 = 2;

      fm_write(0x105, 1);                 /* enable OPL3 mode */

      for (i=0x1F5; i>0x105; i--)
	 fm_write(i, 0);

      for (i=0x104; i>0x100; i--)
	 fm_write(i, 0);

      if (!enable)
	 fm_write(0x105, 0);              /* turn OPL3 mode off again */
   }
   else {
      fm_delay_1 = 6;
      fm_delay_2 = 35;

      if (midi_card == MIDI_2XOPL2) {     /* if we have a second OPL2... */
	 for (i=0x1F5; i>0x100; i--)
	    fm_write(i, 0);

	 fm_write(0x101, 0x20); 
	 fm_write(0x1BD, 0xC0); 
      }
   }

   for (i=0; i<midi_driver->voices; i++) {
      fm_key[i] = 0;
      fm_keyscale[i] = 0;
      fm_feedback[i] = 0;
      fm_level[i] = 0;
      fm_patch[i] = -1;
      fm_write(0x40+fm_offset[i], 63);
      fm_write(0x43+fm_offset[i], 63);
   }

   for (i=0; i<5; i++) {
      fm_drum_cached_inst1[i] = NULL;
      fm_drum_cached_inst2[i] = NULL;
      fm_drum_cached_vol1[i] = -1;
      fm_drum_cached_vol2[i] = -1;
      fm_drum_cached_time[i] = 0;
   }

   fm_write(0x01, 0x20);                  /* turn on wave form control */

   fm_drum_mode = FALSE;
   fm_drum_mask = 0xC0;
   fm_write(0xBD, fm_drum_mask);          /* set AM and vibrato to high */

   midi_driver->xmin = -1;
   midi_driver->xmax = -1;
}

END_OF_STATIC_FUNCTION(fm_reset);



/* fm_set_drum_mode:
 *  Switches the OPL synth between normal and percussion modes.
 */
static void fm_set_drum_mode(int usedrums)
{
   int i;

   fm_drum_mode = usedrums;
   fm_drum_mask = usedrums ? 0xE0 : 0xC0;

   midi_driver->xmin = usedrums ? 6 : -1;
   midi_driver->xmax = usedrums ? 8 : -1;

   for (i=6; i<9; i++)
      if (midi_card == MIDI_OPL3)
	 fm_write(0xC0+VOICE_OFFSET(i), 0x30);
      else
	 fm_write(0xC0+VOICE_OFFSET(i), 0);

   fm_write(0xBD, fm_drum_mask);
}

END_OF_STATIC_FUNCTION(fm_set_drum_mode);



/* fm_set_voice:
 *  Sets the sound to be used for the specified voice, from a structure
 *  containing eleven bytes of FM operator data. Note that it doesn't
 *  actually set the volume: it just stores volume data in the fm_level
 *  arrays for fm_set_volume() to use.
 */
static INLINE void fm_set_voice(int voice, FM_INSTRUMENT *inst)
{
   /* store some info */
   fm_keyscale[voice] = inst->level2 & 0xC0;
   fm_level[voice] = 63 - (inst->level2 & 63);
   fm_feedback[voice] = inst->feedback;

   /* write the new data */
   fm_write(0x20+fm_offset[voice], inst->characteristic1);
   fm_write(0x23+fm_offset[voice], inst->characteristic2);
   fm_write(0x60+fm_offset[voice], inst->attackdecay1);
   fm_write(0x63+fm_offset[voice], inst->attackdecay2);
   fm_write(0x80+fm_offset[voice], inst->sustainrelease1);
   fm_write(0x83+fm_offset[voice], inst->sustainrelease2);
   fm_write(0xE0+fm_offset[voice], inst->wave1);
   fm_write(0xE3+fm_offset[voice], inst->wave2);

   /* don't set operator1 level for additive synthesis sounds */
   if (!(inst->feedback & 1))
      fm_write(0x40+fm_offset[voice], inst->level1);

   /* on OPL3, 0xC0 contains pan info, so don't set it until fm_key_on() */
   if (midi_card != MIDI_OPL3)
      fm_write(0xC0+VOICE_OFFSET(voice), inst->feedback);
}



/* fm_set_drum_op1:
 *  Sets the sound for operator #1 of a drum channel.
 */
static INLINE void fm_set_drum_op1(int voice, FM_INSTRUMENT *inst)
{
   fm_write(0x20+fm_offset[voice], inst->characteristic1);
   fm_write(0x60+fm_offset[voice], inst->attackdecay1);
   fm_write(0x80+fm_offset[voice], inst->sustainrelease1);
   fm_write(0xE0+fm_offset[voice], inst->wave1);
}



/* fm_set_drum_op2:
 *  Sets the sound for operator #2 of a drum channel.
 */
static INLINE void fm_set_drum_op2(int voice, FM_INSTRUMENT *inst)
{
   fm_write(0x23+fm_offset[voice], inst->characteristic2);
   fm_write(0x63+fm_offset[voice], inst->attackdecay2);
   fm_write(0x83+fm_offset[voice], inst->sustainrelease2);
   fm_write(0xE3+fm_offset[voice], inst->wave2);
}



/* fm_set_drum_vol_op1:
 *  Sets the volume for operator #1 of a drum channel.
 */
static INLINE void fm_set_drum_vol_op1(int voice, int vol)
{
   vol = 63 * fm_vol_table[vol] / 128;
   fm_write(0x40+fm_offset[voice], (63-vol));
}



/* fm_set_drum_vol_op2:
 *  Sets the volume for operator #2 of a drum channel.
 */
static INLINE void fm_set_drum_vol_op2(int voice, int vol)
{
   vol = 63 * fm_vol_table[vol] / 128;
   fm_write(0x43+fm_offset[voice], (63-vol));
}



/* fm_set_drum_pitch:
 *  Sets the pitch of a drum channel.
 */
static INLINE void fm_set_drum_pitch(int voice, FM_INSTRUMENT *drum)
{
   fm_write(0xA0+VOICE_OFFSET(voice), drum->freq);
   fm_write(0xB0+VOICE_OFFSET(voice), drum->key & 0x1F);
}



/* fm_trigger_drum:
 *  Triggers a note on a drum channel.
 */
static INLINE void fm_trigger_drum(int inst, int vol)
{
   FM_INSTRUMENT *drum = fm_drum+inst;
   int d;

   if (!fm_drum_mode)
      fm_set_drum_mode(TRUE);

   if (drum->type == FM_BD)
      d = 0;
   else if (drum->type == FM_SD)
      d = 1;
   else if (drum->type == FM_TT)
      d = 2;
   else if (drum->type == FM_CY)
      d = 3;
   else
      d = 4;

   /* don't let drum sounds come too close together */
   if (fm_drum_cached_time[d] == _midi_tick)
      return;

   fm_drum_cached_time[d] = _midi_tick;

   fm_drum_mask &= (~drum->type);
   fm_write(0xBD, fm_drum_mask);

   vol = vol*3/4;

   if (fm_drum_op1[d]) {
      if (fm_drum_cached_inst1[d] != drum) {
	 fm_drum_cached_inst1[d] = drum;
	 fm_set_drum_op1(fm_drum_channel[d], drum);
      }

      if (fm_drum_cached_vol1[d] != vol) {
	 fm_drum_cached_vol1[d] = vol;
	 fm_set_drum_vol_op1(fm_drum_channel[d], vol);
      }
   }

   if (fm_drum_op2[d]) {
      if (fm_drum_cached_inst2[d] != drum) {
	 fm_drum_cached_inst2[d] = drum;
	 fm_set_drum_op2(fm_drum_channel[d], drum);
      }

      if (fm_drum_cached_vol2[d] != vol) {
	 fm_drum_cached_vol2[d] = vol;
	 fm_set_drum_vol_op2(fm_drum_channel[d], vol);
      }
   }

   fm_set_drum_pitch(fm_drum_channel[d], drum);

   fm_drum_mask |= drum->type;
   fm_write(0xBD, fm_drum_mask);
}



/* fm_key_on:
 *  Triggers the specified voice. The instrument is specified as a GM
 *  patch number, pitch as a midi note number, and volume from 0-127.
 *  The bend parameter is _not_ expressed as a midi pitch bend value.
 *  It ranges from 0 (no pitch change) to 0xFFF (almost a semitone sharp).
 *  Drum sounds are indicated by passing an instrument number greater than
 *  128, in which case the sound is GM percussion key #(inst-128).
 */
static void fm_key_on(int inst, int note, int bend, int vol, int pan)
{
   int voice;

   if (inst > 127) {                               /* drum sound? */
      inst -= 163;
      if (inst < 0)
	 inst = 0;
      else if (inst > 46)
	 inst = 46;

      fm_trigger_drum(inst, vol);
   }
   else {                                          /* regular instrument */
      if (midi_card == MIDI_2XOPL2) {
	 /* the SB Pro-1 has fixed pan positions per voice... */
	 if (pan < 64)
	    voice = _midi_allocate_voice(0, 5);
	 else
	    voice = _midi_allocate_voice(9, midi_driver->voices-1);
      }
      else
	 /* on other cards we can use any voices */
	 voice = _midi_allocate_voice(-1, -1);

      if (voice < 0)
	 return;

      /* make sure the voice isn't sounding */
      fm_write(0x43+fm_offset[voice], 63);
      if (fm_feedback[voice] & 1)
	 fm_write(0x40+fm_offset[voice], 63);

      /* make sure the voice is set up with the right sound */
      if (inst != fm_patch[voice]) {
	 fm_set_voice(voice, fm_instrument+inst);
	 fm_patch[voice] = inst;
      }

      /* set pan position */
      if (midi_card == MIDI_OPL3) {
	 if (pan < 48)
	    pan = 0x10;
	 else if (pan >= 80)
	    pan = 0x20;
	 else
	    pan = 0x30;

	 fm_write(0xC0+VOICE_OFFSET(voice), pan | fm_feedback[voice]);
      }

      /* and play the note */
      fm_set_pitch(voice, note, bend);
      fm_set_volume(voice, vol);
   }
}

END_OF_STATIC_FUNCTION(fm_key_on);



/* fm_key_off:
 *  Hey, guess what this does :-)
 */
static void fm_key_off(int voice)
{
   fm_write(0xB0+VOICE_OFFSET(voice), fm_key[voice] & 0xDF);
}

END_OF_STATIC_FUNCTION(fm_key_off);



/* fm_set_volume:
 *  Sets the volume of the specified voice (vol range 0-127).
 */
static void fm_set_volume(int voice, int vol)
{
   vol = fm_level[voice] * fm_vol_table[vol] / 128;
   fm_write(0x43+fm_offset[voice], (63-vol) | fm_keyscale[voice]);
   if (fm_feedback[voice] & 1)
      fm_write(0x40+fm_offset[voice], (63-vol) | fm_keyscale[voice]);
}

END_OF_STATIC_FUNCTION(fm_set_volume);



/* fm_set_pitch:
 *  Sets the pitch of the specified voice.
 */
static void fm_set_pitch(int voice, int note, int bend)
{
   int oct = 1;
   int freq;

   note -= 24;
   while (note >= 12) {
      note -= 12;
      oct++;
   }

   freq = fm_freq[note];
   if (bend)
      freq += (fm_freq[note+1] - fm_freq[note]) * bend / 0x1000;

   fm_key[voice] = (oct<<2) | (freq >> 8);

   fm_write(0xA0+VOICE_OFFSET(voice), freq & 0xFF); 
   fm_write(0xB0+VOICE_OFFSET(voice), fm_key[voice] | 0x20);
}

END_OF_STATIC_FUNCTION(fm_set_pitch);



/* fm_load_patches:
 *  Called before starting to play a MIDI file, to check if we need to be
 *  in rhythm mode or not.
 */
static int fm_load_patches(AL_CONST char *patches, AL_CONST char *drums)
{
   int i;
   int usedrums = FALSE;

   for (i=6; i<9; i++) {
      fm_key[i] = 0;
      fm_keyscale[i] = 0;
      fm_feedback[i] = 0;
      fm_level[i] = 0;
      fm_patch[i] = -1;
      fm_write(0x40+fm_offset[i], 63);
      fm_write(0x43+fm_offset[i], 63);
   }

   for (i=0; i<5; i++) {
      fm_drum_cached_inst1[i] = NULL;
      fm_drum_cached_inst2[i] = NULL;
      fm_drum_cached_vol1[i] = -1;
      fm_drum_cached_vol2[i] = -1;
      fm_drum_cached_time[i] = 0;
   }

   for (i=0; i<128; i++) {
      if (drums[i]) {
	 usedrums = TRUE;
	 break;
      }
   }

   fm_set_drum_mode(usedrums);

   return 0;
}

END_OF_STATIC_FUNCTION(fm_load_patches);



/* fm_set_mixer_volume:
 *  For SB-Pro cards, sets the mixer volume for FM output.
 */
static int fm_set_mixer_volume(int volume)
{
   return _sb_set_mixer(-1, volume);
}



/* fm_is_there:
 *  Checks for the presence of an OPL synth at the current port.
 */
static int fm_is_there(void)
{
   fm_write(1, 0);                        /* init test register */

   fm_write(4, 0x60);                     /* reset both timers */
   fm_write(4, 0x80);                     /* enable interrupts */

   if (inportb(_fm_port) & 0xE0)
      return FALSE;

   fm_write(2, 0xFF);                     /* write 0xFF to timer 1 */
   fm_write(4, 0x21);                     /* start timer 1 */

   rest(100);

   if ((inportb(_fm_port) & 0xE0) != 0xC0)
      return FALSE;

   fm_write(4, 0x60);                     /* reset both timers */
   fm_write(4, 0x80);                     /* enable interrupts */

   return TRUE;
}



/* fm_detect:
 *  Adlib detection routine.
 */
static int fm_detect(int input)
{
   static int ports[] = { 0x210, 0x220, 0x230, 0x240, 0x250, 0x260, 0x388, 0 };
   char tmp1[64], tmp2[64];
   int opl_type;
   AL_CONST char *s;
   int i;

   if (input)
      return FALSE;

   _fm_port = get_config_hex(uconvert_ascii("sound", tmp1), uconvert_ascii("fm_port", tmp2), -1);

   if (_fm_port < 0) {
      if (midi_card == MIDI_OPL2) {
	 _fm_port = 0x388;
	 if (fm_is_there())
	    goto found_it;
      }

      for (i=0; ports[i]; i++) {          /* find the card */
	 _fm_port = ports[i];
	 if (fm_is_there())
	    goto found_it;
      }
   }

   if (!fm_is_there()) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("OPL synth not found"));
      return FALSE;
   }

   found_it:

   if ((inportb(_fm_port) & 6) == 0) {    /* check for OPL3 */
      opl_type = MIDI_OPL3;
      _sb_read_dsp_version();
   }
   else {                                 /* check for second OPL2 */
      if (_sb_read_dsp_version() >= 0x300)
	 opl_type = MIDI_2XOPL2;
      else
	 opl_type = MIDI_OPL2;
   }

   if (midi_card == MIDI_OPL3) {
      if (opl_type != MIDI_OPL3) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("OPL3 synth not found"));
	 return FALSE;
      }
   }
   else if (midi_card == MIDI_2XOPL2) {
      if (opl_type != MIDI_2XOPL2) {
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Second OPL2 synth not found"));
	 return FALSE;
      }
   }
   else if (midi_card != MIDI_OPL2)
      midi_card = opl_type;

   if (midi_card == MIDI_OPL2)
      s = get_config_text("OPL2 synth");
   else if (midi_card == MIDI_2XOPL2)
      s = get_config_text("Dual OPL2 synths");
   else
      s = get_config_text("OPL3 synth");

   uszprintf(adlib_desc, sizeof(adlib_desc), get_config_text("%s on port %X"), s, _fm_port);
   midi_driver->desc = adlib_desc;

   return TRUE;
}



/* load_ibk:
 *  Reads in a .IBK patch set file, for use by the Adlib driver.
 */
int load_ibk(AL_CONST char *filename, int drums)
{
   char sig[4];
   FM_INSTRUMENT *inst;
   int c, note, oct, skip, count;

   PACKFILE *f = pack_fopen(filename, F_READ);
   if (!f)
      return -1;

   pack_fread(sig, 4, f);
   if (memcmp(sig, "IBK\x1A", 4) != 0) {
      pack_fclose(f);
      return -1;
   }

   if (drums) {
      inst = fm_drum;
      skip = 35;
      count = 47;
   }
   else {
      inst = fm_instrument;
      skip = 0;
      count = 128;
   }

   for (c=0; c<skip*16; c++)
      pack_getc(f);

   for (c=0; c<count; c++) {
      inst->characteristic1 = pack_getc(f);
      inst->characteristic2 = pack_getc(f);
      inst->level1 = pack_getc(f);
      inst->level2 = pack_getc(f);
      inst->attackdecay1 = pack_getc(f);
      inst->attackdecay2 = pack_getc(f);
      inst->sustainrelease1 = pack_getc(f);
      inst->sustainrelease2 = pack_getc(f);
      inst->wave1 = pack_getc(f);
      inst->wave2 = pack_getc(f);
      inst->feedback = pack_getc(f);

      if (drums) {
	 switch (pack_getc(f)) {
	    case 6:  inst->type = FM_BD;  break;
	    case 7:  inst->type = FM_HH;  break;
	    case 8:  inst->type = FM_TT;  break;
	    case 9:  inst->type = FM_SD;  break;
	    case 10: inst->type = FM_CY;  break;
	    default: inst->type = 0;      break;
	 }

	 pack_getc(f);

	 note = pack_getc(f) - 24;
	 oct = 1;

	 while (note >= 12) {
	    note -= 12;
	    oct++;
	 }

	 inst->freq = fm_freq[note];
	 inst->key = (oct<<2) | (fm_freq[note] >> 8);
      }
      else {
	 inst->type = 0;
	 inst->freq = 0;
	 inst->key = 0;

	 pack_getc(f);
	 pack_getc(f);
	 pack_getc(f);
      }

      pack_getc(f);
      pack_getc(f);

      inst++;
   }

   pack_fclose(f);
   return 0;
}



/* fm_init:
 *  Setup the adlib driver.
 */
static int fm_init(int input, int voices)
{
   char tmp1[128], tmp2[128];
   AL_CONST char *s;
   int i;

   fm_reset(1);

   for (i=0; i<2; i++) {
      s = get_config_string(uconvert_ascii("sound", tmp1), uconvert_ascii(((i == 0) ? "ibk_file" : "ibk_drum_file"), tmp2), NULL);
      if ((s) && (ugetc(s))) {
	 if (load_ibk(s, (i > 0)) != 0) {
	    uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Error reading .IBK file '%s'"), s);
	    return -1;
	 }
      }
   }

   LOCK_VARIABLE(midi_opl2);
   LOCK_VARIABLE(midi_2xopl2);
   LOCK_VARIABLE(midi_opl3);
   LOCK_VARIABLE(fm_instrument);
   LOCK_VARIABLE(fm_drum);
   LOCK_VARIABLE(_fm_port);
   LOCK_VARIABLE(fm_offset);
   LOCK_VARIABLE(fm_freq);
   LOCK_VARIABLE(fm_vol_table);
   LOCK_VARIABLE(fm_drum_channel);
   LOCK_VARIABLE(fm_drum_op1);
   LOCK_VARIABLE(fm_drum_op2);
   LOCK_VARIABLE(fm_drum_cached_inst1);
   LOCK_VARIABLE(fm_drum_cached_inst2);
   LOCK_VARIABLE(fm_drum_cached_vol1);
   LOCK_VARIABLE(fm_drum_cached_vol2);
   LOCK_VARIABLE(fm_drum_cached_time);
   LOCK_VARIABLE(fm_drum_mask);
   LOCK_VARIABLE(fm_drum_mode);
   LOCK_VARIABLE(fm_key);
   LOCK_VARIABLE(fm_keyscale);
   LOCK_VARIABLE(fm_feedback);
   LOCK_VARIABLE(fm_level);
   LOCK_VARIABLE(fm_patch);
   LOCK_VARIABLE(fm_delay_1);
   LOCK_VARIABLE(fm_delay_2);
   LOCK_FUNCTION(fm_write);
   LOCK_FUNCTION(fm_reset);
   LOCK_FUNCTION(fm_set_drum_mode);
   LOCK_FUNCTION(fm_key_on);
   LOCK_FUNCTION(fm_key_off);
   LOCK_FUNCTION(fm_set_volume);
   LOCK_FUNCTION(fm_set_pitch);
   LOCK_FUNCTION(fm_load_patches);

   return 0;
}



/* fm_exit:
 *  Cleanup when we are finished.
 */
static void fm_exit(int input)
{
   fm_reset(0);
}

