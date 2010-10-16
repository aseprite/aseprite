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
 *      Sample mixing code.
 *
 *      By Shawn Hargreaves.
 *
 *      Proper 16 bit sample support added by Salvador Eduardo Tropea.
 *
 *      Ben Davis provided the set_volume_per_voice() functionality,
 *      programmed the silent mixer so that silent voices don't freeze,
 *      and fixed a few minor bugs elsewhere.
 *
 *      Synchronization added by Sam Hocevar.
 *
 *      Chris Robinson included functions to report the mixer's settings,
 *      switched to signed 24-bit mixing, and cleaned up some of the mess the
 *      code had gathered.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"



typedef struct MIXER_VOICE
{
   int playing;               /* are we active? */
   int channels;              /* # of chaanels for input data? */
   int bits;                  /* sample bit-depth */
   union {
      unsigned char *u8;      /* data for 8 bit samples */
      unsigned short *u16;    /* data for 16 bit samples */
      void *buffer;           /* generic data pointer */
   } data;
   long pos;                  /* fixed point position in sample */
   long diff;                 /* fixed point speed of play */
   long len;                  /* fixed point sample length */
   long loop_start;           /* fixed point loop start position */
   long loop_end;             /* fixed point loop end position */
   int lvol;                  /* left channel volume */
   int rvol;                  /* right channel volume */
} MIXER_VOICE;


/* MIX_FIX_SHIFT must be <= (sizeof(int)*8)-24 */
#define MIX_FIX_SHIFT         8
#define MIX_FIX_SCALE         (1<<MIX_FIX_SHIFT)

#define UPDATE_FREQ           16
#define UPDATE_FREQ_SHIFT     4


/* the samples currently being played */
static MIXER_VOICE mixer_voice[MIXER_MAX_SFX];

/* temporary sample mixing buffer */
static signed int *mix_buffer = NULL;

/* lookup table for converting sample volumes */
#define MIX_VOLUME_LEVELS     32
typedef signed int MIXER_VOL_TABLE[256];
static MIXER_VOL_TABLE mix_vol_table[MIX_VOLUME_LEVELS];

/* stats for the mixing code */
static int mix_voices;
static int mix_size;
static int mix_freq;
static int mix_channels;
static int mix_bits;

/* shift factor for volume per voice */
static int voice_volume_scale = 1;

static void mixer_lock_mem(void);

#ifdef ALLEGRO_MULTITHREADED
/* global mixer mutex */
static void *mixer_mutex = NULL;
#endif



/* set_mixer_quality:
 *  Sets the resampling quality of the mixer. Valid values are the same as
 *  the 'quality' config variable.
 */
void set_mixer_quality(int quality)
{
   if((quality < 0) || (quality > 2))
      quality = 2;
   if(mix_channels == 1)
      quality = 0;

   _sound_hq = quality;
}

END_OF_FUNCTION(set_mixer_quality);



/* get_mixer_quality:
 *  Returns the current mixing quality, as loaded by the 'quality' config
 *  variable, or a previous call to set_mixer_quality.
 */
int get_mixer_quality(void)
{
   return _sound_hq;
}

END_OF_FUNCTION(get_mixer_quality);



/* get_mixer_frequency:
 *  Returns the mixer frequency, in Hz.
 */
int get_mixer_frequency(void)
{
   return mix_freq;
}

END_OF_FUNCTION(get_mixer_frequency);



/* get_mixer_bits:
 *  Returns the mixer bitdepth.
 */
int get_mixer_bits(void)
{
   return mix_bits;
}

END_OF_FUNCTION(get_mixer_bits);



/* get_mixer_channels:
 *  Returns the number of output channels.
 */
int get_mixer_channels(void)
{
   return mix_channels;
}

END_OF_FUNCTION(get_mixer_channels);



/* get_mixer_voices:
 *  Returns the number of voices allocated to the mixer.
 */
int get_mixer_voices(void)
{
   return mix_voices;
}

END_OF_FUNCTION(get_mixer_voices);



/* get_mixer_buffer_length:
 *  Returns the number of samples per channel in the mixer buffer.
 */
int get_mixer_buffer_length(void)
{
   return mix_size;
}

END_OF_FUNCTION(get_mixer_buffer_length);



/* clamp_volume:
 *  Clamps an integer between 0 and the specified (positive!) value.
 */
static INLINE int clamp_val(int i, int max)
{
   /* Clamp to 0 */
   i &= (~i) >> 31;

   /* Clamp to max */
   i -= max;
   i &= i >> 31;
   i += max;

   return i;
}


/* set_volume_per_voice:
 *  Enables the programmer (not the end-user) to alter the maximum volume of
 *  each voice:
 *  - pass -1 for Allegro to work as it did before this option was provided
 *    (volume dependent on number of voices),
 *  - pass 0 if you want a single centred sample to be as loud as possible
 *    without distorting,
 *  - pass 1 if you want to pan a full-volume sample to one side without
 *    distortion,
 *  - each time the scale parameter increases by 1, the volume halves.
 */
static void update_mixer_volume(MIXER_VOICE *mv, PHYS_VOICE *pv);
void set_volume_per_voice(int scale)
{
   int i;

   if(scale < 0) {
      /* Work out the # of voices and the needed scale */
      scale = 1;
      for(i = 1;i < mix_voices;i <<= 1)
         scale++;

      /* Backwards compatiblity with 3.12 */
      if(scale < 2)
         scale = 2;
   }

   /* Update the mixer voices' volumes */
#ifdef ALLEGRO_MULTITHREADED
   if(mixer_mutex)
      system_driver->lock_mutex(mixer_mutex);
#endif
   voice_volume_scale = scale;

   for(i = 0;i < mix_voices;++i)
      update_mixer_volume(mixer_voice+i, _phys_voice+i);
#ifdef ALLEGRO_MULTITHREADED
   if(mixer_mutex)
      system_driver->unlock_mutex(mixer_mutex);
#endif
}

END_OF_FUNCTION(set_volume_per_voice);



/* _mixer_init:
 *  Initialises the sample mixing code, returning 0 on success. You should
 *  pass it the number of samples you want it to mix each time the refill
 *  buffer routine is called, the sample rate to mix at, and two flags
 *  indicating whether the mixing should be done in stereo or mono and with
 *  eight or sixteen bits. The bufsize parameter is the number of samples,
 *  not bytes. It should take into account whether you are working in stereo
 *  or not (eg. double it if in stereo), but it should not be affected by
 *  whether each sample is 8 or 16 bits.
 */
int _mixer_init(int bufsize, int freq, int stereo, int is16bit, int *voices)
{
   int i, j;

   if((_sound_hq < 0) || (_sound_hq > 2))
      _sound_hq = 2;

   mix_voices = *voices;
   if(mix_voices > MIXER_MAX_SFX)
      *voices = mix_voices = MIXER_MAX_SFX;

   mix_freq = freq;
   mix_channels = (stereo ? 2 : 1);
   mix_bits = (is16bit ? 16 : 8);
   mix_size = bufsize / mix_channels;

   for (i=0; i<MIXER_MAX_SFX; i++) {
      mixer_voice[i].playing = FALSE;
      mixer_voice[i].data.buffer = NULL;
   }

   /* temporary buffer for sample mixing */
   mix_buffer = _AL_MALLOC_ATOMIC(mix_size*mix_channels * sizeof(*mix_buffer));
   if (!mix_buffer) {
      mix_size = 0;
      mix_freq = 0;
      mix_channels = 0;
      mix_bits = 0;
      return -1;
   }

   LOCK_DATA(mix_buffer, mix_size*mix_channels * sizeof(*mix_buffer));

   for (j=0; j<MIX_VOLUME_LEVELS; j++)
      for (i=0; i<256; i++)
	 mix_vol_table[j][i] = ((i-128) * 256 * j / MIX_VOLUME_LEVELS) << 8;

   mixer_lock_mem();

#ifdef ALLEGRO_MULTITHREADED
   /* Woops. Forgot to clean up incase this fails. :) */
   mixer_mutex = system_driver->create_mutex();
   if (!mixer_mutex) {
      _AL_FREE(mix_buffer);
      mix_buffer = NULL;
      mix_size = 0;
      mix_freq = 0;
      mix_channels = 0;
      mix_bits = 0;
      return -1;
   }
#endif

   return 0;
}



/* _mixer_exit:
 *  Cleans up the sample mixer code when you are done with it.
 */
void _mixer_exit(void)
{
#ifdef ALLEGRO_MULTITHREADED
   system_driver->destroy_mutex(mixer_mutex);
   mixer_mutex = NULL;
#endif

   if (mix_buffer)
      _AL_FREE(mix_buffer);
   mix_buffer = NULL;

   mix_size = 0;
   mix_freq = 0;
   mix_channels = 0;
   mix_bits = 0;
   mix_voices = 0;
}


/* update_mixer_volume:
 *  Called whenever the voice volume or pan changes, to update the mixer
 *  amplification table indexes.
 */
static void update_mixer_volume(MIXER_VOICE *mv, PHYS_VOICE *pv)
{
   int vol, pan, lvol, rvol;

   /* now use full 16 bit volume ranges */
   vol = pv->vol>>12;
   pan = pv->pan>>12;

   lvol = vol * (255-pan);
   rvol = vol * pan;

   /* Adjust for 255*255 < 256*256-1 */
   lvol += lvol >> 7;
   rvol += rvol >> 7;

   /* Apply voice volume scale and clamp */
   mv->lvol = clamp_val((lvol<<1) >> voice_volume_scale, 65535);
   mv->rvol = clamp_val((rvol<<1) >> voice_volume_scale, 65535);

   if (!_sound_hq) {
      /* Scale 16-bit -> table size */
      mv->lvol = mv->lvol * MIX_VOLUME_LEVELS / 65536;
      mv->rvol = mv->rvol * MIX_VOLUME_LEVELS / 65536;
   }
}

END_OF_STATIC_FUNCTION(update_mixer_volume);



/* update_mixer_freq:
 *  Called whenever the voice frequency changes, to update the sample
 *  delta value.
 */
static INLINE void update_mixer_freq(MIXER_VOICE *mv, PHYS_VOICE *pv)
{
   mv->diff = (pv->freq >> (12 - MIX_FIX_SHIFT)) / mix_freq;

   if (pv->playmode & PLAYMODE_BACKWARD)
      mv->diff = -mv->diff;
}



/* update_mixer:
 *  Helper for updating the volume ramp and pitch/pan sweep status.
 */
static void update_mixer(MIXER_VOICE *spl, PHYS_VOICE *voice, int len)
{
   if ((voice->dvol) || (voice->dpan)) {
      /* update volume ramp */
      if (voice->dvol) {
         voice->vol += voice->dvol;
         if (((voice->dvol > 0) && (voice->vol >= voice->target_vol)) ||
             ((voice->dvol < 0) && (voice->vol <= voice->target_vol))) {
            voice->vol = voice->target_vol;
            voice->dvol = 0;
         }
      }

      /* update pan sweep */
      if (voice->dpan) {
         voice->pan += voice->dpan;
         if (((voice->dpan > 0) && (voice->pan >= voice->target_pan)) ||
             ((voice->dpan < 0) && (voice->pan <= voice->target_pan))) {
            voice->pan = voice->target_pan;
            voice->dpan = 0;
         }
      }

      update_mixer_volume(spl, voice);
   }

   /* update frequency sweep */
   if (voice->dfreq) {
      voice->freq += voice->dfreq;
      if (((voice->dfreq > 0) && (voice->freq >= voice->target_freq)) ||
          ((voice->dfreq < 0) && (voice->freq <= voice->target_freq))) {
         voice->freq = voice->target_freq;
         voice->dfreq = 0;
      }

      update_mixer_freq(spl, voice);
   }
}

END_OF_STATIC_FUNCTION(update_mixer);

/* update_silent_mixer:
 *  Another helper for updating the volume ramp and pitch/pan sweep status.
 *  This version is designed for the silent mixer, and it is called just once
 *  per buffer. The len parameter is used to work out how much the values
 *  must be adjusted.
 */
static void update_silent_mixer(MIXER_VOICE *spl, PHYS_VOICE *voice, int len)
{
   len >>= UPDATE_FREQ_SHIFT;

   /* update pan sweep */
   if (voice->dpan) {
      voice->pan += voice->dpan * len;
      if (((voice->dpan > 0) && (voice->pan >= voice->target_pan)) ||
          ((voice->dpan < 0) && (voice->pan <= voice->target_pan))) {
         voice->pan = voice->target_pan;
         voice->dpan = 0;
      }
   }

   /* update frequency sweep */
   if (voice->dfreq) {
      voice->freq += voice->dfreq * len;
      if (((voice->dfreq > 0) && (voice->freq >= voice->target_freq)) ||
          ((voice->dfreq < 0) && (voice->freq <= voice->target_freq))) {
         voice->freq = voice->target_freq;
         voice->dfreq = 0;
      }

      update_mixer_freq(spl, voice);
   }
}

END_OF_STATIC_FUNCTION(update_silent_mixer);



/* helper for constructing the body of a sample mixing routine */
#define MIXER()                                                              \
{                                                                            \
   if ((voice->playmode & PLAYMODE_LOOP) &&                                  \
       (spl->loop_start < spl->loop_end)) {                                  \
                                                                             \
      if (voice->playmode & PLAYMODE_BACKWARD) {                             \
         /* mix a backward looping sample */                                 \
         while (len--) {                                                     \
            MIX();                                                           \
            spl->pos += spl->diff;                                           \
            if (spl->pos < spl->loop_start) {                                \
               if (voice->playmode & PLAYMODE_BIDIR) {                       \
                  spl->diff = -spl->diff;                                    \
                  /* however far the sample has overshot, move it the same */\
                  /* distance from the loop point, within the loop section */\
                  spl->pos = (spl->loop_start << 1) - spl->pos;              \
                  voice->playmode ^= PLAYMODE_BACKWARD;                      \
               }                                                             \
               else                                                          \
                  spl->pos += (spl->loop_end - spl->loop_start);             \
            }                                                                \
            if ((len & (UPDATE_FREQ-1)) == 0)                                \
               update_mixer(spl, voice, len);                                \
         }                                                                   \
      }                                                                      \
      else {                                                                 \
         /* mix a forward looping sample */                                  \
         while (len--) {                                                     \
            MIX();                                                           \
            spl->pos += spl->diff;                                           \
            if (spl->pos >= spl->loop_end) {                                 \
               if (voice->playmode & PLAYMODE_BIDIR) {                       \
                  spl->diff = -spl->diff;                                    \
                  /* however far the sample has overshot, move it the same */\
                  /* distance from the loop point, within the loop section */\
                  spl->pos = ((spl->loop_end - 1) << 1) - spl->pos;          \
                  voice->playmode ^= PLAYMODE_BACKWARD;                      \
               }                                                             \
               else                                                          \
                  spl->pos -= (spl->loop_end - spl->loop_start);             \
            }                                                                \
            if ((len & (UPDATE_FREQ-1)) == 0)                                \
               update_mixer(spl, voice, len);                                \
         }                                                                   \
      }                                                                      \
   }                                                                         \
   else {                                                                    \
      /* mix a non-looping sample */                                         \
      while (len--) {                                                        \
         MIX();                                                              \
         spl->pos += spl->diff;                                              \
         if ((unsigned long)spl->pos >= (unsigned long)spl->len) {           \
            /* note: we don't need a different version for reverse play, */  \
            /* as this will wrap and automatically do the Right Thing */     \
            spl->playing = FALSE;                                            \
            return;                                                          \
         }                                                                   \
         if ((len & (UPDATE_FREQ-1)) == 0)                                   \
            update_mixer(spl, voice, len);                                   \
      }                                                                      \
   }                                                                         \
}


/* mix_silent_samples:
 *  This is used when the voice is silent, instead of the other
 *  mix_*_samples() functions. It just extrapolates the sample position,
 *  and stops the sample if it reaches the end and isn't set to loop.
 *  Since no mixing is necessary, this function is much faster than
 *  its friends. In addition, no buffer parameter is required,
 *  and the same function can be used for all sample types.
 *
 *  There is a catch. All the mix_stereo_*_samples() and
 *  mix_hq?_*_samples() functions (those which write to a stereo mixing
 *  buffer) divide len by 2 before using it in the MIXER() macro.
 *  Therefore, all the mix_silent_samples() for stereo buffers must divide
 *  the len parameter by 2.
 */
static void mix_silent_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, int len)
{
   if ((voice->playmode & PLAYMODE_LOOP) &&
       (spl->loop_start < spl->loop_end)) {

      if (voice->playmode & PLAYMODE_BACKWARD) {
         /* mix a backward looping sample */
         spl->pos += spl->diff * len;
         if (spl->pos < spl->loop_start) {
            if (voice->playmode & PLAYMODE_BIDIR) {
               do {
                  spl->diff = -spl->diff;
                  spl->pos = (spl->loop_start << 1) - spl->pos;
                  voice->playmode ^= PLAYMODE_BACKWARD;
                  if (spl->pos < spl->loop_end) break;
                  spl->diff = -spl->diff;
                  spl->pos = ((spl->loop_end - 1) << 1) - spl->pos;
                  voice->playmode ^= PLAYMODE_BACKWARD;
               } while (spl->pos < spl->loop_start);
            }
            else {
               do {
                  spl->pos += (spl->loop_end - spl->loop_start);
               } while (spl->pos < spl->loop_start);
            }
         }
         update_silent_mixer(spl, voice, len);
      }
      else {
         /* mix a forward looping sample */
         spl->pos += spl->diff * len;
         if (spl->pos >= spl->loop_end) {
            if (voice->playmode & PLAYMODE_BIDIR) {
               do {
                  spl->diff = -spl->diff;
                  spl->pos = ((spl->loop_end - 1) << 1) - spl->pos;
                  voice->playmode ^= PLAYMODE_BACKWARD;
                  if (spl->pos >= spl->loop_start) break;
                  spl->diff = -spl->diff;
                  spl->pos = (spl->loop_start << 1) - spl->pos;
                  voice->playmode ^= PLAYMODE_BACKWARD;
               } while (spl->pos >= spl->loop_end);
            }
            else {
               do {
                  spl->pos -= (spl->loop_end - spl->loop_start);
               } while (spl->pos >= spl->loop_end);
            }
         }
         update_silent_mixer(spl, voice, len);
      }
   }
   else {
      /* mix a non-looping sample */
      spl->pos += spl->diff * len;
      if ((unsigned long)spl->pos >= (unsigned long)spl->len) {
         /* note: we don't need a different version for reverse play, */
         /* as this will wrap and automatically do the Right Thing */
         spl->playing = FALSE;
         return;
      }
      update_silent_mixer(spl, voice, len);
   }
}

END_OF_STATIC_FUNCTION(mix_silent_samples);



/* mix_mono_8x1_samples:
 *  Mixes from an eight bit sample into a mono buffer, until either len
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_mono_8x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   signed int *lvol = (int *)(mix_vol_table + (spl->lvol>>1));
   signed int *rvol = (int *)(mix_vol_table + (spl->rvol>>1));

   #define MIX()                                                             \
      *(buf)   += lvol[spl->data.u8[spl->pos>>MIX_FIX_SHIFT]];               \
      *(buf++) += rvol[spl->data.u8[spl->pos>>MIX_FIX_SHIFT]];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_mono_8x1_samples);



/* mix_mono_8x2_samples:
 *  Mixes from an eight bit stereo sample into a mono buffer, until either
 *  len samples have been mixed or until the end of the sample is reached.
 */
static void mix_mono_8x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   signed int *lvol = (int *)(mix_vol_table + (spl->lvol>>1));
   signed int *rvol = (int *)(mix_vol_table + (spl->rvol>>1));

   #define MIX()                                                             \
      *(buf)   += lvol[spl->data.u8[(spl->pos>>MIX_FIX_SHIFT)*2  ]];         \
      *(buf++) += rvol[spl->data.u8[(spl->pos>>MIX_FIX_SHIFT)*2+1]];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_mono_8x2_samples);



/* mix_mono_16x1_samples:
 *  Mixes from a 16 bit sample into a mono buffer, until either len samples
 *  have been mixed or until the end of the sample is reached.
 */
static void mix_mono_16x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   signed int *lvol = (int *)(mix_vol_table + (spl->lvol>>1));
   signed int *rvol = (int *)(mix_vol_table + (spl->rvol>>1));

   #define MIX()                                                             \
      *(buf)   += lvol[(spl->data.u16[spl->pos>>MIX_FIX_SHIFT])>>8];         \
      *(buf++) += rvol[(spl->data.u16[spl->pos>>MIX_FIX_SHIFT])>>8];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_mono_16x1_samples);



/* mix_mono_16x2_samples:
 *  Mixes from a 16 bit stereo sample into a mono buffer, until either len
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_mono_16x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   signed int *lvol = (int *)(mix_vol_table + (spl->lvol>>1));
   signed int *rvol = (int *)(mix_vol_table + (spl->rvol>>1));

   #define MIX()                                                             \
      *(buf)   += lvol[(spl->data.u16[(spl->pos>>MIX_FIX_SHIFT)*2  ])>>8];   \
      *(buf++) += rvol[(spl->data.u16[(spl->pos>>MIX_FIX_SHIFT)*2+1])>>8];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_mono_16x2_samples);



/* mix_stereo_8x1_samples:
 *  Mixes from an eight bit sample into a stereo buffer, until either len
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_8x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   signed int *lvol = (int *)(mix_vol_table + spl->lvol);
   signed int *rvol = (int *)(mix_vol_table + spl->rvol);

   #define MIX()                                                             \
      *(buf++) += lvol[spl->data.u8[spl->pos>>MIX_FIX_SHIFT]];               \
      *(buf++) += rvol[spl->data.u8[spl->pos>>MIX_FIX_SHIFT]];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_stereo_8x1_samples);



/* mix_stereo_8x2_samples:
 *  Mixes from an eight bit stereo sample into a stereo buffer, until either
 *  len samples have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_8x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   signed int *lvol = (int *)(mix_vol_table + spl->lvol);
   signed int *rvol = (int *)(mix_vol_table + spl->rvol);

   #define MIX()                                                             \
      *(buf++) += lvol[spl->data.u8[(spl->pos>>MIX_FIX_SHIFT)*2  ]];         \
      *(buf++) += rvol[spl->data.u8[(spl->pos>>MIX_FIX_SHIFT)*2+1]];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_stereo_8x2_samples);



/* mix_stereo_16x1_samples:
 *  Mixes from a 16 bit sample into a stereo buffer, until either len samples
 *  have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_16x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   signed int *lvol = (int *)(mix_vol_table + spl->lvol);
   signed int *rvol = (int *)(mix_vol_table + spl->rvol);

   #define MIX()                                                             \
      *(buf++) += lvol[(spl->data.u16[spl->pos>>MIX_FIX_SHIFT])>>8];         \
      *(buf++) += rvol[(spl->data.u16[spl->pos>>MIX_FIX_SHIFT])>>8];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_stereo_16x1_samples);



/* mix_stereo_16x2_samples:
 *  Mixes from a 16 bit stereo sample into a stereo buffer, until either len
 *  samples have been mixed or until the end of the sample is reached.
 */
static void mix_stereo_16x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   signed int *lvol = (int *)(mix_vol_table + spl->lvol);
   signed int *rvol = (int *)(mix_vol_table + spl->rvol);

   #define MIX()                                                             \
      *(buf++) += lvol[(spl->data.u16[(spl->pos>>MIX_FIX_SHIFT)*2  ])>>8];   \
      *(buf++) += rvol[(spl->data.u16[(spl->pos>>MIX_FIX_SHIFT)*2+1])>>8];

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_stereo_16x2_samples);

/* mix_hq1_8x1_samples:
 *  Mixes from a mono 8 bit sample into a high quality stereo buffer,
 *  until either len samples have been mixed or until the end of the
 *  sample is reached.
 */
static void mix_hq1_8x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   int lvol = spl->lvol;
   int rvol = spl->rvol;

   #define MIX()                                                             \
      *(buf++) += (spl->data.u8[spl->pos>>MIX_FIX_SHIFT]-0x80) * lvol;       \
      *(buf++) += (spl->data.u8[spl->pos>>MIX_FIX_SHIFT]-0x80) * rvol;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq1_8x1_samples);



/* mix_hq1_8x2_samples:
 *  Mixes from a stereo 8 bit sample into a high quality stereo buffer,
 *  until either len samples have been mixed or until the end of the
 *  sample is reached.
 */
static void mix_hq1_8x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   int lvol = spl->lvol;
   int rvol = spl->rvol;

   #define MIX()                                                             \
      *(buf++) += (spl->data.u8[(spl->pos>>MIX_FIX_SHIFT)*2  ]-0x80) * lvol; \
      *(buf++) += (spl->data.u8[(spl->pos>>MIX_FIX_SHIFT)*2+1]-0x80) * rvol;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq1_8x2_samples);



/* mix_hq1_16x1_samples:
 *  Mixes from a mono 16 bit sample into a high-quality stereo buffer,
 *  until either len samples have been mixed or until the end of the sample
 *  is reached.
 */
static void mix_hq1_16x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   int lvol = spl->lvol;
   int rvol = spl->rvol;

   #define MIX()                                                             \
      *(buf++) += ((spl->data.u16[spl->pos>>MIX_FIX_SHIFT]-0x8000)*lvol)>>8; \
      *(buf++) += ((spl->data.u16[spl->pos>>MIX_FIX_SHIFT]-0x8000)*rvol)>>8;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq1_16x1_samples);



/* mix_hq1_16x2_samples:
 *  Mixes from a stereo 16 bit sample into a high-quality stereo buffer,
 *  until either len samples have been mixed or until the end of the sample
 *  is reached.
 */
static void mix_hq1_16x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   int lvol = spl->lvol;
   int rvol = spl->rvol;

   #define MIX()                                                                 \
      *(buf++) += ((spl->data.u16[(spl->pos>>MIX_FIX_SHIFT)*2  ]-0x8000)*lvol)>>8;\
      *(buf++) += ((spl->data.u16[(spl->pos>>MIX_FIX_SHIFT)*2+1]-0x8000)*rvol)>>8;

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq1_16x2_samples);


/* Helper to apply a 16-bit volume to a 24-bit sample */
#define MULSC(a, b) ((int)((LONG_LONG)((a) << 4) * ((b) << 12) >> 32))

/* mix_hq2_8x1_samples:
 *  Mixes from a mono 8 bit sample into an interpolated stereo buffer,
 *  until either len samples have been mixed or until the end of the
 *  sample is reached.
 */
static void mix_hq2_8x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   int lvol = spl->lvol;
   int rvol = spl->rvol;
   int v, v1, v2;

   #define MIX()                                                             \
      v = spl->pos>>MIX_FIX_SHIFT;                                           \
                                                                             \
      v1 = (spl->data.u8[v]<<16) - 0x800000;                                 \
                                                                             \
      if (spl->pos >= spl->len-MIX_FIX_SCALE) {                              \
         if ((voice->playmode & (PLAYMODE_LOOP |                             \
                                 PLAYMODE_BIDIR)) == PLAYMODE_LOOP &&        \
             spl->loop_start < spl->loop_end && spl->loop_end == spl->len)   \
            v2 = (spl->data.u8[spl->loop_start>>MIX_FIX_SHIFT]<<16)-0x800000;\
         else                                                                \
            v2 = 0;                                                          \
      }                                                                      \
      else                                                                   \
         v2 = (spl->data.u8[v+1]<<16) - 0x800000;                            \
                                                                             \
      v = spl->pos & (MIX_FIX_SCALE-1);                                      \
      v = ((v2*v) + (v1*(MIX_FIX_SCALE-v))) >> MIX_FIX_SHIFT;                \
                                                                             \
      *(buf++) += MULSC(v, lvol);                                            \
      *(buf++) += MULSC(v, rvol);

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq2_8x1_samples);



/* mix_hq2_8x2_samples:
 *  Mixes from a stereo 8 bit sample into an interpolated stereo buffer,
 *  until either len samples have been mixed or until the end of the
 *  sample is reached.
 */
static void mix_hq2_8x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   int lvol = spl->lvol;
   int rvol = spl->rvol;
   int v, va, v1a, v2a, vb, v1b, v2b;

   #define MIX()                                                             \
      v = (spl->pos>>MIX_FIX_SHIFT) << 1; /* x2 for stereo */                \
                                                                             \
      v1a = (spl->data.u8[v]<<16) - 0x800000;                                \
      v1b = (spl->data.u8[v+1]<<16) - 0x800000;                              \
                                                                             \
      if (spl->pos >= spl->len-MIX_FIX_SCALE) {                              \
         if ((voice->playmode & (PLAYMODE_LOOP |                             \
                                 PLAYMODE_BIDIR)) == PLAYMODE_LOOP &&        \
             spl->loop_start < spl->loop_end && spl->loop_end == spl->len) { \
            v2a = (spl->data.u8[((spl->loop_start>>MIX_FIX_SHIFT)<<1)]<<16) - 0x800000;\
            v2b = (spl->data.u8[((spl->loop_start>>MIX_FIX_SHIFT)<<1)+1]<<16) - 0x800000;\
         }                                                                   \
         else                                                                \
            v2a = v2b = 0;                                                   \
      }                                                                      \
      else {                                                                 \
         v2a = (spl->data.u8[v+2]<<16) - 0x800000;                           \
         v2b = (spl->data.u8[v+3]<<16) - 0x800000;                           \
      }                                                                      \
                                                                             \
      v = spl->pos & (MIX_FIX_SCALE-1);                                      \
      va = ((v2a*v) + (v1a*(MIX_FIX_SCALE-v))) >> MIX_FIX_SHIFT;             \
      vb = ((v2b*v) + (v1b*(MIX_FIX_SCALE-v))) >> MIX_FIX_SHIFT;             \
                                                                             \
      *(buf++) += MULSC(va, lvol);                                           \
      *(buf++) += MULSC(vb, rvol);

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq2_8x2_samples);



/* mix_hq2_16x1_samples:
 *  Mixes from a mono 16 bit sample into an interpolated stereo buffer,
 *  until either len samples have been mixed or until the end of the sample
 *  is reached.
 */
static void mix_hq2_16x1_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   int lvol = spl->lvol;
   int rvol = spl->rvol;
   int v, v1, v2;

   #define MIX()                                                             \
      v = spl->pos>>MIX_FIX_SHIFT;                                           \
                                                                             \
      v1 = (spl->data.u16[v]<<8) - 0x800000;                                 \
                                                                             \
      if (spl->pos >= spl->len-MIX_FIX_SCALE) {                              \
         if ((voice->playmode & (PLAYMODE_LOOP |                             \
                                 PLAYMODE_BIDIR)) == PLAYMODE_LOOP &&        \
             spl->loop_start < spl->loop_end && spl->loop_end == spl->len)   \
            v2 = (spl->data.u16[spl->loop_start>>MIX_FIX_SHIFT]<<8)-0x800000;\
         else                                                                \
            v2 = 0;                                                          \
      }                                                                      \
      else                                                                   \
         v2 = (spl->data.u16[v+1]<<8) - 0x800000;                            \
                                                                             \
      v = spl->pos & (MIX_FIX_SCALE-1);                                      \
      v = ((v2*v) + (v1*(MIX_FIX_SCALE-v))) >> MIX_FIX_SHIFT;                \
                                                                             \
      *(buf++) += MULSC(v, lvol);                                            \
      *(buf++) += MULSC(v, rvol);

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq2_16x1_samples);



/* mix_hq2_16x2_samples:
 *  Mixes from a stereo 16 bit sample into an interpolated stereo buffer,
 *  until either len samples have been mixed or until the end of the sample
 *  is reached.
 */
static void mix_hq2_16x2_samples(MIXER_VOICE *spl, PHYS_VOICE *voice, signed int *buf, int len)
{
   int lvol = spl->lvol;
   int rvol = spl->rvol;
   int v, va, v1a, v2a, vb, v1b, v2b;

   #define MIX()                                                             \
      v = (spl->pos>>MIX_FIX_SHIFT) << 1; /* x2 for stereo */                \
                                                                             \
      v1a = (spl->data.u16[v]<<8) - 0x800000;                                \
      v1b = (spl->data.u16[v+1]<<8) - 0x800000;                              \
                                                                             \
      if (spl->pos >= spl->len-MIX_FIX_SCALE) {                              \
         if ((voice->playmode & (PLAYMODE_LOOP |                             \
                                 PLAYMODE_BIDIR)) == PLAYMODE_LOOP &&        \
             spl->loop_start < spl->loop_end && spl->loop_end == spl->len) { \
            v2a = (spl->data.u16[((spl->loop_start>>MIX_FIX_SHIFT)<<1)]<<8) - 0x800000;\
            v2b = (spl->data.u16[((spl->loop_start>>MIX_FIX_SHIFT)<<1)+1]<<8) - 0x800000;\
         }                                                                   \
         else                                                                \
            v2a = v2b = 0;                                                   \
      }                                                                      \
      else {                                                                 \
         v2a = (spl->data.u16[v+2]<<8) - 0x800000;                           \
         v2b = (spl->data.u16[v+3]<<8) - 0x800000;                           \
      }                                                                      \
                                                                             \
      v = spl->pos & (MIX_FIX_SCALE-1);                                      \
      va = ((v2a*v) + (v1a*(MIX_FIX_SCALE-v))) >> MIX_FIX_SHIFT;             \
      vb = ((v2b*v) + (v1b*(MIX_FIX_SCALE-v))) >> MIX_FIX_SHIFT;             \
                                                                             \
      *(buf++) += MULSC(va, lvol);                                           \
      *(buf++) += MULSC(vb, rvol);

   MIXER();

   #undef MIX
}

END_OF_STATIC_FUNCTION(mix_hq2_16x2_samples);



#define MAX_24 (0x00FFFFFF)

/* _mix_some_samples:
 *  Mixes samples into a buffer in memory (the buf parameter should be a
 *  linear offset into the specified segment), using the buffer size, sample
 *  frequency, etc, set when you called _mixer_init(). This should be called
 *  by the audio driver to get the next buffer full of samples.
 */
void _mix_some_samples(uintptr_t buf, unsigned short seg, int issigned)
{
   signed int *p = mix_buffer;
   int i;

   /* clear mixing buffer */
   memset(p, 0, mix_size*mix_channels * sizeof(*p));

#ifdef ALLEGRO_MULTITHREADED
   system_driver->lock_mutex(mixer_mutex);
#endif

   for (i=0; i<mix_voices; i++) {
      if (mixer_voice[i].playing) {
         if ((_phys_voice[i].vol > 0) || (_phys_voice[i].dvol > 0)) {
            /* Interpolated mixing */
            if (_sound_hq >= 2) {
               /* stereo input -> interpolated output */
               if (mixer_voice[i].channels != 1) {
                  if (mixer_voice[i].bits == 8)
                     mix_hq2_8x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
                  else
                     mix_hq2_16x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
               }
               /* mono input -> interpolated output */
               else {
                  if (mixer_voice[i].bits == 8)
                     mix_hq2_8x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
                  else
                     mix_hq2_16x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
               }
            }
            /* high quality mixing */
            else if (_sound_hq) {
               /* stereo input -> high quality output */
               if (mixer_voice[i].channels != 1) {
                  if (mixer_voice[i].bits == 8)
                     mix_hq1_8x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
                  else
                     mix_hq1_16x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
               }
               /* mono input -> high quality output */
               else {
                  if (mixer_voice[i].bits == 8)
                     mix_hq1_8x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
                  else
                     mix_hq1_16x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
               }
            }
            /* low quality (fast?) stereo mixing */
            else if (mix_channels != 1) {
               /* stereo input -> stereo output */
               if (mixer_voice[i].channels != 1) {
                  if (mixer_voice[i].bits == 8)
                     mix_stereo_8x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
                  else
                     mix_stereo_16x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
               }
               /* mono input -> stereo output */
               else {
                  if (mixer_voice[i].bits == 8)
                     mix_stereo_8x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
                  else
                     mix_stereo_16x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
               }
            }
            /* low quality (fast?) mono mixing */
            else {
               /* stereo input -> mono output */
               if (mixer_voice[i].channels != 1) {
                  if (mixer_voice[i].bits == 8)
                     mix_mono_8x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
                  else
                     mix_mono_16x2_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
               }
               /* mono input -> mono output */
               else {
                  if (mixer_voice[i].bits == 8)
                     mix_mono_8x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
                  else
                     mix_mono_16x1_samples(mixer_voice+i, _phys_voice+i, p, mix_size);
               }
            }
         }
         else
            mix_silent_samples(mixer_voice+i, _phys_voice+i, mix_size);
      }
   }

#ifdef ALLEGRO_MULTITHREADED
   system_driver->unlock_mutex(mixer_mutex);
#endif

   _farsetsel(seg);

   /* transfer to the audio driver's buffer */
   if (mix_bits == 16) {
      if (issigned) {
         for (i=mix_size*mix_channels; i>0; i--) {
            _farnspokew(buf, (clamp_val((*p)+0x800000, MAX_24) >> 8) ^ 0x8000);
            buf += 2;
            p++;
         }
      }
      else {
         for (i=mix_size*mix_channels; i>0; i--) {
            _farnspokew(buf, clamp_val((*p)+0x800000, MAX_24) >> 8);
            buf += 2;
            p++;
         }
      }
   }
   else {
      if(issigned) {
         for (i=mix_size*mix_channels; i>0; i--) {
            _farnspokeb(buf, (clamp_val((*p)+0x800000, MAX_24) >> 16) ^ 0x80);
            buf++;
            p++;
         }
      }
      else {
         for (i=mix_size*mix_channels; i>0; i--) {
            _farnspokeb(buf, clamp_val((*p)+0x800000, MAX_24) >> 16);
            buf++;
            p++;
         }
      }
   }
}

END_OF_FUNCTION(_mix_some_samples);



/* _mixer_init_voice:
 *  Initialises the specificed voice ready for playing a sample.
 */
void _mixer_init_voice(int voice, AL_CONST SAMPLE *sample)
{
   mixer_voice[voice].playing = FALSE;
   mixer_voice[voice].channels = (sample->stereo ? 2 : 1);
   mixer_voice[voice].bits = sample->bits;
   mixer_voice[voice].pos = 0;
   mixer_voice[voice].len = sample->len << MIX_FIX_SHIFT;
   mixer_voice[voice].loop_start = sample->loop_start << MIX_FIX_SHIFT;
   mixer_voice[voice].loop_end = sample->loop_end << MIX_FIX_SHIFT;

   mixer_voice[voice].data.buffer = sample->data;

   update_mixer_volume(mixer_voice+voice, _phys_voice+voice);
   update_mixer_freq(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_init_voice);



/* _mixer_release_voice:
 *  Releases a voice when it is no longer required.
 */
void _mixer_release_voice(int voice)
{
#ifdef ALLEGRO_MULTITHREADED
   system_driver->lock_mutex(mixer_mutex);
#endif

   mixer_voice[voice].playing = FALSE;
   mixer_voice[voice].data.buffer = NULL;

#ifdef ALLEGRO_MULTITHREADED
   system_driver->unlock_mutex(mixer_mutex);
#endif
}

END_OF_FUNCTION(_mixer_release_voice);



/* _mixer_start_voice:
 *  Activates a voice, with the currently selected parameters.
 */
void _mixer_start_voice(int voice)
{
   if (mixer_voice[voice].pos >= mixer_voice[voice].len)
      mixer_voice[voice].pos = 0;

   mixer_voice[voice].playing = TRUE;
}

END_OF_FUNCTION(_mixer_start_voice);



/* _mixer_stop_voice:
 *  Stops a voice from playing.
 */
void _mixer_stop_voice(int voice)
{
   mixer_voice[voice].playing = FALSE;
}

END_OF_FUNCTION(_mixer_stop_voice);



/* _mixer_loop_voice:
 *  Sets the loopmode for a voice.
 */
void _mixer_loop_voice(int voice, int loopmode)
{
   update_mixer_freq(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_loop_voice);



/* _mixer_get_position:
 *  Returns the current play position of a voice, or -1 if it has finished.
 */
int _mixer_get_position(int voice)
{
   if ((!mixer_voice[voice].playing) ||
       (mixer_voice[voice].pos >= mixer_voice[voice].len))
      return -1;

   return (mixer_voice[voice].pos >> MIX_FIX_SHIFT);
}

END_OF_FUNCTION(_mixer_get_position);



/* _mixer_set_position:
 *  Sets the current play position of a voice.
 */
void _mixer_set_position(int voice, int position)
{
   if (position < 0)
      position = 0;

   mixer_voice[voice].pos = (position << MIX_FIX_SHIFT);
   if (mixer_voice[voice].pos >= mixer_voice[voice].len)
      mixer_voice[voice].playing = FALSE;
}

END_OF_FUNCTION(_mixer_set_position);



/* _mixer_get_volume:
 *  Returns the current volume of a voice.
 */
int _mixer_get_volume(int voice)
{
   return (_phys_voice[voice].vol >> 12);
}

END_OF_FUNCTION(_mixer_get_volume);



/* _mixer_set_volume:
 *  Sets the volume of a voice.
 */
void _mixer_set_volume(int voice, int volume)
{
   update_mixer_volume(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_set_volume);



/* _mixer_ramp_volume:
 *  Starts a volume ramping operation.
 */
void _mixer_ramp_volume(int voice, int time, int endvol)
{
   int d = (endvol << 12) - _phys_voice[voice].vol;
   time = MAX(time * (mix_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_vol = endvol << 12;
   _phys_voice[voice].dvol = d / time;
}

END_OF_FUNCTION(_mixer_ramp_volume);



/* _mixer_stop_volume_ramp:
 *  Ends a volume ramp operation.
 */
void _mixer_stop_volume_ramp(int voice)
{
   _phys_voice[voice].dvol = 0;
}

END_OF_FUNCTION(_mixer_stop_volume_ramp);



/* _mixer_get_frequency:
 *  Returns the current frequency of a voice.
 */
int _mixer_get_frequency(int voice)
{
   return (_phys_voice[voice].freq >> 12);
}

END_OF_FUNCTION(_mixer_get_frequency);



/* _mixer_set_frequency:
 *  Sets the frequency of a voice.
 */
void _mixer_set_frequency(int voice, int frequency)
{
   update_mixer_freq(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_set_frequency);



/* _mixer_sweep_frequency:
 *  Starts a frequency sweep.
 */
void _mixer_sweep_frequency(int voice, int time, int endfreq)
{
   int d = (endfreq << 12) - _phys_voice[voice].freq;
   time = MAX(time * (mix_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_freq = endfreq << 12;
   _phys_voice[voice].dfreq = d / time;
}

END_OF_FUNCTION(_mixer_sweep_frequency);



/* _mixer_stop_frequency_sweep:
 *  Ends a frequency sweep.
 */
void _mixer_stop_frequency_sweep(int voice)
{
   _phys_voice[voice].dfreq = 0;
}

END_OF_FUNCTION(_mixer_stop_frequency_sweep);



/* _mixer_get_pan:
 *  Returns the current pan position of a voice.
 */
int _mixer_get_pan(int voice)
{
   return (_phys_voice[voice].pan >> 12);
}

END_OF_FUNCTION(_mixer_get_pan);



/* _mixer_set_pan:
 *  Sets the pan position of a voice.
 */
void _mixer_set_pan(int voice, int pan)
{
   update_mixer_volume(mixer_voice+voice, _phys_voice+voice);
}

END_OF_FUNCTION(_mixer_set_pan);



/* _mixer_sweep_pan:
 *  Starts a pan sweep.
 */
void _mixer_sweep_pan(int voice, int time, int endpan)
{
   int d = (endpan << 12) - _phys_voice[voice].pan;
   time = MAX(time * (mix_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_pan = endpan << 12;
   _phys_voice[voice].dpan = d / time;
}

END_OF_FUNCTION(_mixer_sweep_pan);



/* _mixer_stop_pan_sweep:
 *  Ends a pan sweep.
 */
void _mixer_stop_pan_sweep(int voice)
{
   _phys_voice[voice].dpan = 0;
}

END_OF_FUNCTION(_mixer_stop_pan_sweep);



/* _mixer_set_echo:
 *  Sets the echo parameters for a voice.
 */
void _mixer_set_echo(int voice, int strength, int delay)
{
   /* not implemented */
}

END_OF_FUNCTION(_mixer_set_echo);



/* _mixer_set_tremolo:
 *  Sets the tremolo parameters for a voice.
 */
void _mixer_set_tremolo(int voice, int rate, int depth)
{
   /* not implemented */
}

END_OF_FUNCTION(_mixer_set_tremolo);



/* _mixer_set_vibrato:
 *  Sets the amount of vibrato for a voice.
 */
void _mixer_set_vibrato(int voice, int rate, int depth)
{
   /* not implemented */
}

END_OF_FUNCTION(_mixer_set_vibrato);



/* mixer_lock_mem:
 *  Locks memory used by the functions in this file.
 */
static void mixer_lock_mem(void)
{
   LOCK_VARIABLE(mixer_voice);
   LOCK_VARIABLE(mix_buffer);
   LOCK_VARIABLE(mix_vol_table);
   LOCK_VARIABLE(mix_voices);
   LOCK_VARIABLE(mix_size);
   LOCK_VARIABLE(mix_freq);
   LOCK_VARIABLE(mix_channels);
   LOCK_VARIABLE(mix_bits);
   LOCK_FUNCTION(set_mixer_quality);
   LOCK_FUNCTION(get_mixer_quality);
   LOCK_FUNCTION(get_mixer_buffer_length);
   LOCK_FUNCTION(get_mixer_frequency);
   LOCK_FUNCTION(get_mixer_bits);
   LOCK_FUNCTION(get_mixer_channels);
   LOCK_FUNCTION(get_mixer_voices);
   LOCK_FUNCTION(set_volume_per_voice);
   LOCK_FUNCTION(mix_silent_samples);
   LOCK_FUNCTION(mix_mono_8x1_samples);
   LOCK_FUNCTION(mix_mono_8x2_samples);
   LOCK_FUNCTION(mix_mono_16x1_samples);
   LOCK_FUNCTION(mix_mono_16x2_samples);
   LOCK_FUNCTION(mix_stereo_8x1_samples);
   LOCK_FUNCTION(mix_stereo_8x2_samples);
   LOCK_FUNCTION(mix_stereo_16x1_samples);
   LOCK_FUNCTION(mix_stereo_16x2_samples);
   LOCK_FUNCTION(mix_hq1_8x1_samples);
   LOCK_FUNCTION(mix_hq1_8x2_samples);
   LOCK_FUNCTION(mix_hq1_16x1_samples);
   LOCK_FUNCTION(mix_hq1_16x2_samples);
   LOCK_FUNCTION(mix_hq2_8x1_samples);
   LOCK_FUNCTION(mix_hq2_8x2_samples);
   LOCK_FUNCTION(mix_hq2_16x1_samples);
   LOCK_FUNCTION(mix_hq2_16x2_samples);
   LOCK_FUNCTION(update_mixer_volume);
   LOCK_FUNCTION(update_mixer);
   LOCK_FUNCTION(update_silent_mixer);
   LOCK_FUNCTION(_mix_some_samples);
   LOCK_FUNCTION(_mixer_init_voice);
   LOCK_FUNCTION(_mixer_release_voice);
   LOCK_FUNCTION(_mixer_start_voice);
   LOCK_FUNCTION(_mixer_stop_voice);
   LOCK_FUNCTION(_mixer_loop_voice);
   LOCK_FUNCTION(_mixer_get_position);
   LOCK_FUNCTION(_mixer_set_position);
   LOCK_FUNCTION(_mixer_get_volume);
   LOCK_FUNCTION(_mixer_set_volume);
   LOCK_FUNCTION(_mixer_ramp_volume);
   LOCK_FUNCTION(_mixer_stop_volume_ramp);
   LOCK_FUNCTION(_mixer_get_frequency);
   LOCK_FUNCTION(_mixer_set_frequency);
   LOCK_FUNCTION(_mixer_sweep_frequency);
   LOCK_FUNCTION(_mixer_stop_frequency_sweep);
   LOCK_FUNCTION(_mixer_get_pan);
   LOCK_FUNCTION(_mixer_set_pan);
   LOCK_FUNCTION(_mixer_sweep_pan);
   LOCK_FUNCTION(_mixer_stop_pan_sweep);
   LOCK_FUNCTION(_mixer_set_echo);
   LOCK_FUNCTION(_mixer_set_tremolo);
   LOCK_FUNCTION(_mixer_set_vibrato);
}
