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
 *      Open Sound System sequencer support.
 *
 *      By Peter Wang.
 *
 *      See readme.txt for copyright information.
 */

#include "allegro.h"

#ifdef ALLEGRO_WITH_OSSMIDI

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if defined(ALLEGRO_HAVE_SOUNDCARD_H)
   #include <soundcard.h>
#elif defined(ALLEGRO_HAVE_SYS_SOUNDCARD_H)
   #include <sys/soundcard.h>
#elif defined(ALLEGRO_HAVE_MACHINE_SOUNDCARD_H)
   #include <machine/soundcard.h>
#elif defined(ALLEGRO_HAVE_LINUX_SOUNDCARD_H)
   #include <linux/soundcard.h>
#endif
#include <sys/ioctl.h>

#if defined(ALLEGRO_HAVE_LINUX_AWE_VOICE_H)
#include <linux/awe_voice.h>
#define UOSSMIDI_HAVE_AWE32
#endif


/* our patch data */
#include "../misc/fm_instr.h"
#include "../misc/fm_emu.h"



static int oss_midi_detect(int input);
static int oss_midi_init(int input, int voices);
static void oss_midi_exit(int input);
static int oss_midi_set_mixer_volume(int volume);
static int oss_midi_get_mixer_volume(void);
static void oss_midi_key_on(int inst, int note, int bend, int vol, int pan);
static void oss_midi_key_off(int voice);
static void oss_midi_set_volume(int voice, int vol);
static void oss_midi_set_pitch(int voice, int note, int bend);



#define MAX_VOICES 256

static int seq_fd = -1;
static int seq_device;
static int seq_synth_type, seq_synth_subtype; 
static int seq_patch[MAX_VOICES];
static int seq_note[MAX_VOICES];
static int seq_drum_start;
static char seq_desc[256] = EMPTY_STRING;

static char seq_driver[256] = EMPTY_STRING;
static char mixer_driver[256] = EMPTY_STRING;

SEQ_DEFINEBUF(2048);



MIDI_DRIVER midi_oss =
{
   MIDI_OSS,
   empty_string,
   empty_string,
   "Open Sound System", 
   0, 0, 0xFFFF, 0, -1, -1,
   oss_midi_detect,
   oss_midi_init,
   oss_midi_exit,
   oss_midi_set_mixer_volume,
   oss_midi_get_mixer_volume,
   NULL,
   _dummy_load_patches, 
   _dummy_adjust_patches, 
   oss_midi_key_on,
   oss_midi_key_off,
   oss_midi_set_volume,
   oss_midi_set_pitch,
   _dummy_noop2,                /* TODO */
   _dummy_noop2                 /* TODO */
};



/* as required by the OSS API */
void seqbuf_dump(void)
{
   if (_seqbufptr) {
      write(seq_fd, _seqbuf, _seqbufptr);
      _seqbufptr = 0;
   }
}



/* attempt to open sequencer device */
static int seq_attempt_open(void)
{
   char tmp1[128], tmp2[128], tmp3[128];
   int fd;

   ustrzcpy(seq_driver, sizeof(seq_driver), get_config_string(uconvert_ascii("sound", tmp1),
					                      uconvert_ascii("oss_midi_driver", tmp2),
					                      uconvert_ascii("/dev/sequencer", tmp3)));

   fd = open(uconvert_toascii(seq_driver, tmp1), O_WRONLY);
   if (fd < 0) 
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s: %s"), seq_driver, ustrerror(errno));

   return fd;
}



/* find the best (supported) synth type for device */
static int seq_find_synth(int fd)
{
   struct synth_info info;
   int num_synths, i;
   char *s;
   char tmp1[64], tmp2[256];
   int score = 0, best_score, best_device;

   if (ioctl(fd, SNDCTL_SEQ_NRSYNTHS, &num_synths) == -1)
      return 0;

   best_device = -1;
   best_score = 0;
   
   /* Detect the best device */
   for (i = 0; i < num_synths; i++) {
      info.device = i;
      if (ioctl(fd, SNDCTL_SYNTH_INFO, &info) == -1)
	 return 0;

      switch (info.synth_type) {

	 case SYNTH_TYPE_FM:
	    /* FM synthesis is kind of ok */
	    score = 2;
	    break;

	 case SYNTH_TYPE_SAMPLE:
	    /* Wavetable MIDI is cool! */
	    score = 3;
	    break;

	 case SYNTH_TYPE_MIDI:
	    /* Only weird people want to use the MIDI out port... */
	    /* ... so we don't accept it yet, this can be fixed when
	     * we've got a config file option to select the MIDI device
	     * so then we'll only select this if people specifically
	     * ask for it */
	    score = 0;
	    break;
      }

      if (score > best_score) {
	 best_score = score;
	 best_device = i;
      }
   }

   if (best_score == 0) {
      return 0;
   }

   /* Cool, we got a decent synth type */
   seq_device = best_device;
   
   /* Now get more information */
   info.device = seq_device;
   if (ioctl(fd, SNDCTL_SYNTH_INFO, &info) == -1)
      return 0;

   seq_synth_type = info.synth_type;
   seq_synth_subtype = info.synth_subtype;

   midi_oss.voices = info.nr_voices;
   if (midi_oss.voices > MAX_VOICES) midi_oss.voices = MAX_VOICES;

   
   switch (seq_synth_type) {
   
      case SYNTH_TYPE_FM:
	 switch (seq_synth_subtype) {
	    case FM_TYPE_ADLIB:
	       s = uconvert_ascii("Adlib", tmp1);
	       break;
	    case FM_TYPE_OPL3:
	       s = uconvert_ascii("OPL3", tmp1);
	       break;
	    default:
	       s = uconvert_ascii("FM (unknown)", tmp1);
	       break;
	 }
	 break;

      case SYNTH_TYPE_SAMPLE:
	 switch (seq_synth_subtype) {
#ifdef UOSSMIDI_HAVE_AWE32
	    case SAMPLE_TYPE_AWE32:
	       s = uconvert_ascii("AWE32", tmp1);
	       break;
#endif
	    default:
	       s = uconvert_ascii("sample (unknown)", tmp1);
	       break;
	 }
	 break;

      case SYNTH_TYPE_MIDI:
	 s = uconvert_ascii("MIDI out", tmp1);
	 break;

      default:
	 s = uconvert_ascii("Unknown synth", tmp1);
	 break;
   }

   uszprintf(seq_desc, sizeof(seq_desc), uconvert_ascii("Open Sound System (%s)", tmp2), s);
   midi_driver->desc = seq_desc;

   return 1;
}



/* FM synth: load our instrument patches */
static void seq_set_fm_patches(int fd)
{
   struct sbi_instrument ins;
   int i;

   ins.device = seq_device;
   ins.key = FM_PATCH;
   memset(ins.operators, 0, sizeof(ins.operators));

   /* instruments */
   for (i=0; i<128; i++) {
      ins.channel = i;
      memcpy(&ins.operators, &fm_instrument[i], sizeof(FM_INSTRUMENT));
      write(fd, &ins, sizeof(ins));
   }

   /* (emulated) drums */
   for (i=0; i<47; i++) {
      ins.channel = 128+i;
      memset(ins.operators, 0, sizeof(ins.operators));
      memcpy(&ins.operators, &fm_emulated_drum[i], sizeof(FM_INSTRUMENT));
      write(fd, &ins, sizeof(ins));
   }
}



/* FM synth setup */
static void seq_setup_fm (void)
{
   seq_set_fm_patches(seq_fd);
   seq_drum_start = midi_oss.voices - 5;
}



#ifdef UOSSMIDI_HAVE_AWE32
/* AWE32 synth setup */
static void seq_setup_awe32 (void)
{
   int bits = 0, drums;

   seq_drum_start = midi_oss.voices;
   if (seq_drum_start > 32) seq_drum_start = 32;
   
   /* These non-32 cases probably never happen, since the AWE32 has 
    * 32 voices and anything higher needs a new interface... */
   if (midi_oss.voices <= 1) {
      drums = 0;
   } else if (midi_oss.voices <= 4) {
      drums = 1;
   } else if (midi_oss.voices <= 32) {
      drums = midi_oss.voices / 8;
   } else {
      drums = 4;
   }

#if 0 // I think I got this wrong
   /* Set the top 'drums' bits of bitfield and decrement seq_drum_start */
   while (drums--) bits |= (1 << --seq_drum_start);
#else
   /* The AWE driver seems to like receiving all percussion on MIDI channel
    * 10 (OSS channel 9) */
   bits = (1<<9);
   seq_drum_start -= drums;
#endif
   
   /* Tell the AWE which channels are drum channels.  No, I don't know 
    * what 'multi' mode is or how to play drums in the other modes.  
    * This is just what playmidi does (except I'm using AWE_PLAY_MULTI 
    * instead of its value, 1). */
   AWE_SET_CHANNEL_MODE(seq_device, AWE_PLAY_MULTI);
   AWE_DRUM_CHANNELS(seq_device, bits);
}
#endif



/* oss_midi_detect:
 *  Sequencer detection routine.
 */
static int oss_midi_detect(int input)
{
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }

   seq_fd = seq_attempt_open();
   if (seq_fd < 0)
      return FALSE;

   close(seq_fd);
   return TRUE;
}



/* oss_midi_init:
 *  Init the driver.
 */
static int oss_midi_init(int input, int voices)
{
   char tmp1[128], tmp2[128], tmp3[128];
   unsigned int i;

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }

   seq_fd = seq_attempt_open();
   if (seq_fd < 0) 
      return -1;

   if (!seq_find_synth(seq_fd)) {
      close(seq_fd);
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("No supported synth type found"));
      return -1;
   }

   ioctl(seq_fd, SNDCTL_SEQ_RESET);

   /* Driver-specific setup */
   if (seq_synth_type == SYNTH_TYPE_FM) {
   
      seq_setup_fm();
      
   } else if (seq_synth_type == SYNTH_TYPE_SAMPLE) {

#ifdef UOSSMIDI_HAVE_AWE32
      if (seq_synth_subtype == SAMPLE_TYPE_AWE32) {

	 seq_setup_awe32();
	 
      }
#endif
      
   }

	       
   for (i = 0; i < (sizeof(seq_patch) / sizeof(int)); i++) {
      seq_patch[i] = -1;
      seq_note[i] = -1;
   }

   /* for the mixer routine */
   ustrzcpy(mixer_driver, sizeof(mixer_driver), get_config_string(uconvert_ascii("sound", tmp1),
					                          uconvert_ascii("oss_mixer_driver", tmp2),
					                          uconvert_ascii("/dev/mixer", tmp3)));

   return 0;
}



/* oss_midi_set_mixer_volume:
 *  Sets the mixer volume for output.
 */
static int oss_midi_set_mixer_volume(int volume)
{
   int fd, vol, ret;
   char tmp[128];

   fd = open(uconvert_toascii(mixer_driver, tmp), O_WRONLY);
   if (fd < 0)
      return -1;

   vol = (volume * 100) / 255;
   vol = (vol << 8) | (vol);
   ret = ioctl(fd, MIXER_WRITE(SOUND_MIXER_SYNTH), &vol);
   close(fd);

   return ((ret == -1) ? -1 : 0);
}



/* oss_midi_get_mixer_volume:
 *  Returns mixer volume.
 */
static int oss_midi_get_mixer_volume(void)
{
   int fd, vol;
   char tmp[128];

   fd = open(uconvert_toascii(mixer_driver, tmp), O_RDONLY);
   if (fd < 0)
      return -1;

   if (ioctl(fd, MIXER_READ(SOUND_MIXER_SYNTH), &vol) != 0)
      return -1;
   close(fd);

   vol = ((vol & 0xff) + (vol >> 8)) / 2;
   vol = vol * 255 / 100;
   return vol;
}



/* get_hardware_voice:
 *  Get the hardware voice corresponding to this virtual voice.  The 
 *  hardware may support more than one note per voice, in which case
 *  you don't want to terminate old notes on reusing a voice, but when
 *  Allegro reuses a voice it forgets the old note and won't ever send
 *  a keyoff.  So we should use more virtual (Allegro) voices than the
 *  channels the hardware provides, and map them down to actual 
 *  hardware channels here.
 *
 *  We also swap voices 9 and 15 (MIDI 10 and 16).  This is necessary 
 *  because _midi_allocate_voice can only allocate in continuous ranges 
 *  -- so we use voices [0,seq_drum_start) for melody and for the
 *  percussion [seq_drum_start,max_voices), as far as Allegro is 
 *  concerned.  But hardware likes percussion on MIDI 10, so we need to
 *  remap it.
 */
static int get_hardware_voice (int voice)
{
   int hwvoice = voice;
   
   /* FIXME: is this OK/useful for other things than AWE32? */
   if (seq_synth_type != SYNTH_TYPE_FM && seq_drum_start > 0) {

      /* map drums >= 15, everything else < 15 */
      hwvoice = hwvoice * 15 / seq_drum_start;

      /* fix up so drums are on MIDI channel 10 */
      if (hwvoice >= 15)
         hwvoice = 9;
      else if (hwvoice == 9)
         hwvoice = 15;

   }

   return hwvoice;
}



/* oss_midi_key_on:
 *  Triggers the specified voice. 
 */
static void oss_midi_key_on(int inst, int note, int bend, int vol, int pan)
{
   int voice, hwvoice;
   int is_percussion = 0;

   /* percussion? */
   if (inst > 127) {
      voice = _midi_allocate_voice(seq_drum_start, midi_driver->voices-1);
      /* TODO: Peter's code decrements inst by 35; but the AWE driver 
       * ignores inst completely, using note instead, as God (well, GM) 
       * intended.  Does the FM driver ignore note?  If so then we're OK. */
      note = inst-128;
      inst -= 35;
      is_percussion = 1;
   }
   else
      voice = _midi_allocate_voice(0, seq_drum_start-1);

   if (voice < 0)
      return;

   /* Get the hardware voice corresponding to this virtual voice. */
   hwvoice = get_hardware_voice (voice);
      
   /* FIXME: should we do this or not for FM? */
   if (seq_synth_type != SYNTH_TYPE_FM) {
      /* Stop any previous note on this voice -- but not if it's percussion */
      if (!is_percussion && seq_note[voice] != -1) {
         SEQ_STOP_NOTE(seq_device, hwvoice, seq_note[voice], 64);
      }
   }
   seq_note[voice] = note;

   /* make sure the (hardware) voice is set up with the right sound */
   if (inst != seq_patch[hwvoice]) {
      SEQ_SET_PATCH(seq_device, hwvoice, inst);
      seq_patch[hwvoice] = inst;
   }

   SEQ_CONTROL(seq_device, hwvoice, CTL_PAN, pan);
   SEQ_BENDER(seq_device, hwvoice, 8192 + bend);
   SEQ_START_NOTE(seq_device, hwvoice, note, vol);
   SEQ_DUMPBUF();
}



/* oss_midi_key_off:
 *  Insert witty remark about this line being unhelpful.
 */
static void oss_midi_key_off(int voice)
{
   /* Get the hardware voice corresponding to this virtual voice. */
   int hwvoice = get_hardware_voice (voice);
      
   SEQ_STOP_NOTE(seq_device, hwvoice, seq_note[voice], 64);
   SEQ_DUMPBUF();

   seq_note[voice] = -1;
}



/* oss_midi_set_volume:
 *  Sets the volume of the specified voice.
 */
static void oss_midi_set_volume(int voice, int vol)
{
   SEQ_CONTROL(seq_device, voice, CTL_MAIN_VOLUME, vol);
}



/* oss_midi_set_pitch:
 *  Sets the pitch of the specified voice.
 */
static void oss_midi_set_pitch(int voice, int note, int bend)
{
   SEQ_CONTROL(seq_device, voice, CTRL_PITCH_BENDER, 8192 + bend);
}



/* oss_midi_exit:
 *  Cleanup when we are finished.
 */
static void oss_midi_exit(int input)
{
   if (input)
      return;

   if (seq_fd > 0) {
      close(seq_fd);
      seq_fd = -1;
   }
}

#endif
