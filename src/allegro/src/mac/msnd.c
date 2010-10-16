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
 *      Mac sound support.
 *
 *      By Ronaldo Hideki Yamada.
 *
 *      from code of /src/mixer.c by Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#include <Gestalt.h>
#include <Sound.h>

#include "allegro.h"
#include "allegro/platform/aintmac.h"

#ifndef ALLEGRO_MPW
   #error something is wrong with the makefile
#endif

static void pascal sound_mac_interrupt(SndChannelPtr chan, SndDoubleBufferPtr doubleBuffer);
static int sound_mac_detect(int input);
static int sound_mac_init(int input, int voices);
static void sound_mac_exit(int input);
static int sound_mac_set_mixer_volume(int volume);
static int sound_mac_buffer_size(void);

static char sb_desc[256] = EMPTY_STRING;

static int sound_mac_in_use = 0;
static int sound_mac_stereo = 0;
static int sound_mac_16bit = 0;
static int sound_mac_buf_size = 256;
static int sound_mac_total_buf_size = 256;
static int sound_mac_freq=44100;
static long _mac_sound=0;
static char sound_mac_desc[256] = EMPTY_STRING;
static SndDoubleBackUPP myDBUPP=NULL;
static SndChannelPtr chan=NULL;
static SndDoubleBufferHeader myDblHeader;


#define MAC_MSG_NONE  0
#define MAC_MSG_DONE  1
#define MAC_MSG_CLOSE 2


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
 *      Proper 16 bit sample support added by SET.
 *
 *      Ben Davis provided the voice_volume_scale functionality, programmed
 *      the silent mixer so that silent voices don't freeze, and fixed a
 *      few minor bugs elsewhere.
 *
 *      See readme.txt for copyright information.
 */


#include <string.h>
#include "allegro/internal/aintern.h"

struct MIXER_VOICE;

typedef struct MIXER_VOICE
{
   int playing;               /* are we active? */
   int stereo;                /* mono or stereo input data? */
   unsigned char *data8;      /* data for 8 bit samples */
   unsigned short *data16;    /* data for 16 bit samples */
   long pos;                  /* fixed point position in sample */
   long diff;                 /* fixed point speed of play */
   long len;                  /* fixed point sample length */
   long loop_start;           /* fixed point loop start position */
   long loop_end;             /* fixed point loop end position */
   int lvol;                  /* left channel volume */
   int rvol;                  /* right channel volume */
   AL_METHOD(void, mix , (struct MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len));
} MIXER_VOICE;


#define MIX_FIX_SHIFT         8
#define MIX_FIX_SCALE         (1<<MIX_FIX_SHIFT)

#define UPDATE_FREQ           32
#define UPDATE_FREQ_SHIFT     5


/* the samples currently being played */
static MIXER_VOICE sound_mac_voice[MIXER_MAX_SFX];

/* flags for the mixing code */
static int sound_mac_voices;


static void sound_mac_lock_mem();


/* updatemixer_volume:
 *  Called whenever the voice volume or pan changes, to update the mixer 
 *  amplification table indexes.
 */
static void updatemixer_volume(MIXER_VOICE *mv, PHYS_VOICE *pv)
{
   int vol, pan, lvol, rvol;
      vol = pv->vol>>13;
      pan = pv->pan>>13;

      /* no need to check for mix_stereo if we're using hq */
      lvol = vol*(127-pan);
      rvol = vol*pan;

      /* adjust for 127*127<128*128-1 */
      lvol += lvol>>6;
      rvol += rvol>>6;

      mv->lvol = MAX(0, lvol);
      mv->rvol = MAX(0, rvol);
}

END_OF_STATIC_FUNCTION(updatemixer_volume);



/* update_mixer_freq:
 *  Called whenever the voice frequency changes, to update the sample
 *  delta value.
 */
static void updatemixer_freq(MIXER_VOICE *mv, PHYS_VOICE *pv)
{
   mv->diff = (pv->freq >> (12 - MIX_FIX_SHIFT)) / sound_mac_freq;

   if (pv->playmode & PLAYMODE_BACKWARD)
      mv->diff = -mv->diff;
}

END_OF_STATIC_FUNCTION(updatemixer_freq);




/* update_silent_mixer:
 *  Another helper for updating the volume ramp and pitch/pan sweep status.
 *  This version is designed for the silent mixer, and it is called just once
 *  per buffer. The len parameter is used to work out how much the values
 *  must be adjusted.
 */
static void update_silent_mixer(MIXER_VOICE *spl, PHYS_VOICE *voice, int len)
{
   len >>= UPDATE_FREQ_SHIFT;

   if (voice->dpan) {
      /* update pan sweep */
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

      updatemixer_freq(spl, voice);
   }
}

END_OF_STATIC_FUNCTION(update_silent_mixer);

#define BEGIN_MIXER()                                                        \
   int register lvol = spl->lvol;                                            \
   int register rvol = spl->rvol;                                            \
   int register d;                                                           \
   int register left;                                                        \
   int register right;                                                       \
   int register sublen;                                                      \
   long register pos = spl->pos;                                             \
   long register diff = spl->diff;                                           \
   long register loop_end = spl->loop_end;                                   \
   long register loop_start = spl->loop_start

#define END_MIXER()                                                          \
   spl->pos = pos;                                                           \
   spl->diff = diff;                                                         \
   spl->loop_end = loop_end;                                                 \
   spl->loop_start = loop_start;                                             \
   spl->lvol = lvol;                                                         \
   spl->rvol = rvol


#define BEGIN_8x1()                                                          \
   unsigned char *data = spl->data8
   
#define BEGIN_8x2()                                                          \
   unsigned short *data = (unsigned short *)spl->data8

#define BEGIN_16x1()                                                         \
   unsigned short *data = spl->data16

#define BEGIN_16x2()                                                         \
   unsigned long *data = (unsigned long *)spl->data16

#define MIX_8x1()                                                           \
      d =  data[pos>>MIX_FIX_SHIFT]-0x80;                                   \
      right = *(unsigned long *)buf;                                        \
      left  = right & 0xFFFF0000;                                           \
      right += (d*rvol)>>8;                                                 \
      left += (((d*lvol)<<8) & 0xFFFF0000);                                 \
      right &= 0xFFFF;                                                      \
      right |= left;                                                        \
      *(unsigned long *)buf = right;                                        \
      buf = buf + 2

#define MIX_8x2()                                                           \
      d = data[pos>>MIX_FIX_SHIFT];                                         \
      right = *(unsigned long *)buf;                                        \
      left  = right >> 16 ;                                                 \
      right &= 0xFFFF;                                                      \
      left += (((d>>8)-0x80)*lvol)>>8;                                      \
      right += (((d&0xFF)-0x80)*rvol)>>8;                                   \
      right &= 0xFFFF;                                                      \
      right += (left << 16);                                                \
      *(unsigned long *)buf = right;                                        \
      buf = buf + 2

#define MIX_16x1()                                                          \
      d = data[pos >>MIX_FIX_SHIFT]-0x8000;                                 \
      right = *(unsigned long *)buf;                                        \
      left  = right >> 16 ;                                                 \
      right &= 0xFFFF;                                                      \
      left += (d*lvol)>>16;                                                 \
      right += (d*rvol)>>16;                                                \
      right &= 0xFFFF;                                                      \
      right += (left << 16);                                                \
      *(unsigned long *)buf = right;                                        \
      buf = buf + 2

#define MIX_16x2()                                                          \
      d = data[pos >>MIX_FIX_SHIFT];                                        \
      right = *(unsigned long *)buf;                                        \
      left  = right >> 16 ;                                                 \
      right &= 0xFFFF;                                                      \
      left += (((((unsigned)d)>>16)-0x8000)*lvol)>>16;                      \
      right += (((d&0xFFFF)-0x8000)*rvol)>>16;                              \
      right &= 0xFFFF;                                                      \
      right += (left << 16);                                                \
      *(unsigned long *)buf = right;                                        \
      buf = buf + 2

#define UPDATE()                                                            \
{                                                                           \
   if ((voice->dvol) || (voice->dpan)) {                                    \
	 /* update volume ramp */                                           \
	 if (voice->dvol) {                                                 \
	    voice->vol += voice->dvol;                                      \
	    if (((voice->dvol > 0) && (voice->vol >= voice->target_vol)) || \
		((voice->dvol < 0) && (voice->vol <= voice->target_vol))) { \
	       voice->vol = voice->target_vol;                              \
	       voice->dvol = 0;                                             \
	    }                                                               \
	 }                                                                  \
	 /* update pan sweep */                                             \
	 if (voice->dpan) {                                                 \
	    voice->pan += voice->dpan;                                      \
	    if (((voice->dpan > 0) && (voice->pan >= voice->target_pan)) || \
		((voice->dpan < 0) && (voice->pan <= voice->target_pan))) { \
	       voice->pan = voice->target_pan;                              \
	       voice->dpan = 0;                                             \
	    }                                                               \
	 }                                                                  \
	/* updatemixer_volume(spl, voice);*/                                \
      d = voice->vol>>13;                                                   \
      right = voice->pan>>13;                                               \
      lvol = d*(127-right);                                                 \
      rvol = d*right;                                                       \
      lvol += lvol>>6;                                                      \
      rvol += rvol>>6;                                                      \
      lvol = MAX(0, lvol);                                                  \
      rvol = MAX(0, rvol);                                                  \
   }                                                                        \
      /* update frequency sweep */                                          \
   if (voice->dfreq) {                                                      \
      voice->freq += voice->dfreq;                                          \
      if (((voice->dfreq > 0) && (voice->freq >= voice->target_freq)) ||    \
          ((voice->dfreq < 0) && (voice->freq <= voice->target_freq))) {    \
	  voice->freq = voice->target_freq;                                 \
	  voice->dfreq = 0;                                                 \
      }                                                                     \
      diff = (voice->freq >> (12 - MIX_FIX_SHIFT)) / sound_mac_freq;        \
      if (voice->playmode & PLAYMODE_BACKWARD)                              \
         diff = -diff;                                                      \
   }                                                                        \
}
      
#define MIXER_FORWARD() {                                                    \
   len = len >> UPDATE_FREQ_SHIFT;                                           \
	 do {                                                                \
	    sublen = UPDATE_FREQ;                                            \
	    do {                                                             \
	       MIX();                                                        \
	       pos += diff;                                                  \
	       if (pos >= loop_end) {                                        \
	          spl->playing = FALSE;                                      \
	          goto MIXER_END;                                            \
	       }                                                             \
	    } while (--sublen) ;                                             \
	    UPDATE();                                                        \
	} while (--len);                                                     \
MIXER_END:{};                                                                \
}

#define MIXER_BACKWARD() {                                                   \
   len = len >> UPDATE_FREQ_SHIFT;                                           \
	 do {                                                                \
	    sublen = UPDATE_FREQ;                                            \
	    do {                                                             \
	       MIX();                                                        \
	       pos += diff;                                                  \
	       if (pos < loop_start) {                                       \
	          spl->playing = FALSE;                                      \
	          goto MIXER_END;                                            \
	       }                                                             \
	    } while (--sublen) ;                                             \
	    UPDATE();                                                        \
	} while (--len);                                                     \
MIXER_END:{};                                                                \
}

#define MIXER_LOOP_FORWARD() {                                               \
   len = len >> UPDATE_FREQ_SHIFT;                                           \
	 do {                                                                \
	    sublen = UPDATE_FREQ;                                            \
	    do {                                                             \
	       MIX();                                                        \
	       pos += diff;                                                  \
	       if (pos >= loop_end) {                                        \
		  pos -= (loop_end - loop_start);                            \
	       }                                                             \
	    } while (--sublen) ;                                             \
	    UPDATE();                                                        \
	} while (--len);                                                     \
}

#define MIXER_LOOP_BACKWARD() {                                              \
   len = len >> UPDATE_FREQ_SHIFT;                                           \
	 do {                                                                \
	    sublen = UPDATE_FREQ;                                            \
	    do {                                                             \
	       MIX();                                                        \
	       pos += diff;                                                  \
	       if (pos < loop_start) {                                       \
		  pos += (loop_end - loop_start);                            \
	       }                                                             \
	    } while (--sublen) ;                                             \
	    UPDATE();                                                        \
	} while (--len);                                                     \
}


#define MIXER_BIDIR() {                                                      \
   len = len >> UPDATE_FREQ_SHIFT;                                           \
      if (voice->playmode & PLAYMODE_BACKWARD){                              \
	 do {                                                                \
	    sublen = UPDATE_FREQ;                                            \
	    do {                                                             \
	       MIX();                                                        \
	       pos += diff;                                                  \
	       if (pos < loop_start) {                                       \
		  diff = -diff;                                              \
                  /* however far the sample has overshot, move it the same */\
                  /* distance from the loop point, within the loop section */\
                 /* if sample is more short than buffer then this is a bug */\
                  pos = (loop_start << 1) - pos;                             \
		  voice->playmode ^= PLAYMODE_BACKWARD;                      \
	       }                                                             \
	    } while (--sublen) ;                                             \
	    UPDATE();                                                        \
	} while (--len);                                                     \
      }                                                                      \
      else{                                                                  \
	 do {                                                                \
	    sublen = UPDATE_FREQ;                                            \
	    do {                                                             \
	       MIX();                                                        \
	       pos += diff;                                                  \
	       if (pos >= loop_end) {                                        \
		  diff = -diff;                                              \
                  /* however far the sample has overshot, move it the same */\
                  /* distance from the loop point, within the loop section */\
                 /* if sample is more short than buffer then this is a bug */\
                  pos = ((loop_end - 1) << 1) - pos;                         \
		  voice->playmode ^= PLAYMODE_BACKWARD;                      \
	       }                                                             \
	    } while (--sublen) ;                                             \
	    UPDATE();                                                        \
	} while (--len);                                                     \
      }                                                                      \
}


static void mac_mixer_8x1_forward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x1();
   #define MIX() MIX_8x1()
   MIXER_FORWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x1_forward);


static void mac_mixer_8x1_backward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x1();
   #define MIX() MIX_8x1()
   MIXER_BACKWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x1_backward);


static void mac_mixer_8x1_loop_forward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x1();
   #define MIX() MIX_8x1()
   MIXER_LOOP_FORWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x1_loop_forward);

static void mac_mixer_8x1_loop_backward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x1();
   #define MIX() MIX_8x1()
   MIXER_LOOP_BACKWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x1_loop_backward);

static void mac_mixer_8x1_bidir(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x1();
   #define MIX() MIX_8x1()
   MIXER_BIDIR();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x1_bidir);

static void mac_mixer_8x2_forward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x2();
   #define MIX() MIX_8x2()
   MIXER_FORWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x2_forward);

static void mac_mixer_8x2_backward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x2();
   #define MIX() MIX_8x2()
   MIXER_BACKWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x2_backward);

static void mac_mixer_8x2_loop_forward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x2();
   #define MIX() MIX_8x2()
   MIXER_LOOP_FORWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x2_loop_forward);


static void mac_mixer_8x2_loop_backward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x2();
   #define MIX() MIX_8x2()
   MIXER_LOOP_BACKWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x2_loop_backward);


static void mac_mixer_8x2_bidir(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_8x2();
   #define MIX() MIX_8x2()
   MIXER_BIDIR();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_8x2_bidir);


static void mac_mixer_16x1_forward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x1();
   #define MIX() MIX_16x1()
   MIXER_FORWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x1_forward);

static void mac_mixer_16x1_backward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x1();
   #define MIX() MIX_16x1()
   MIXER_BACKWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x1_backward);

static void mac_mixer_16x1_loop_forward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x1();
   #define MIX() MIX_16x1()
   MIXER_LOOP_FORWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x1_loop_forward);


static void mac_mixer_16x1_loop_backward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x1();
   #define MIX() MIX_16x1()
   MIXER_LOOP_BACKWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x1_loop_backward);


static void mac_mixer_16x1_bidir(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x1();
   #define MIX() MIX_16x1()
   MIXER_BIDIR();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x1_bidir);


static void mac_mixer_16x2_forward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x2();
   #define MIX() MIX_16x2()
   MIXER_FORWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x2_forward);

static void mac_mixer_16x2_backward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x2();
   #define MIX() MIX_16x2()
   MIXER_BACKWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x2_backward);

static void mac_mixer_16x2_loop_forward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x2();
   #define MIX() MIX_16x2()
   MIXER_LOOP_FORWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x2_loop_forward);


static void mac_mixer_16x2_loop_backward(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x2();
   #define MIX() MIX_16x2()
   MIXER_LOOP_BACKWARD();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x2_loop_backward);


static void mac_mixer_16x2_bidir(MIXER_VOICE *spl, PHYS_VOICE *voice, unsigned short *buf, int len)
{
   BEGIN_MIXER();
   BEGIN_16x2();
   #define MIX() MIX_16x2()
   MIXER_BIDIR();
   #undef MIX
   END_MIXER();
}
END_OF_STATIC_FUNCTION(mac_mixer_16x2_bidir);



/* helper for constructing the body of a sample mixing routine */
#define MIXER()                                                              \
{                                                                            \
   if ((voice->playmode & PLAYMODE_LOOP) &&                                  \
       (spl->loop_start < spl->loop_end)) {                                  \
									     \
      if (voice->playmode & PLAYMODE_BACKWARD) {                             \
	 /* mix a backward looping sample */                                 \
	 while (len-- > 0) {                                                 \
               MIX();                                                        \
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
	    update_mixer(spl, voice, len);                                   \
	 }                                                                   \
      }                                                                      \
      else {                                                                 \
	 /* mix a forward looping sample */                                  \
	 while (len-- > 0) {                                                 \
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
	    update_mixer(spl, voice, len);                                   \
	 }                                                                   \
      }                                                                      \
   }                                                                         \
   else {                                                                    \
      /* mix a non-looping sample */                                         \
      while (len-- > 0) {                                                    \
	 MIX();                                                              \
	 spl->pos += spl->diff;                                              \
	 if ((unsigned long)spl->pos >= (unsigned long)spl->len) {           \
	    /* note: we don't need a different version for reverse play, */  \
	    /* as this will wrap and automatically do the Right Thing */     \
	    spl->playing = FALSE;                                            \
	    return;                                                          \
	 }                                                                   \
	 update_mixer(spl, voice, len);                                      \
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
 *  the len parameter by 2. This is done in mix_some_samples().
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


static void sound_mac_update_mix(int voice){
   int playmode=_phys_voice[voice].playmode;
   if (sound_mac_voice[voice].loop_start > sound_mac_voice[voice].loop_end)
   	playmode &= ~( PLAYMODE_LOOP | PLAYMODE_BIDIR );
   switch (playmode){
      case PLAYMODE_PLAY:
         if (sound_mac_voice[voice].data8) {
            if(sound_mac_voice[voice].stereo)
                sound_mac_voice[voice].mix = mac_mixer_8x2_forward;      
            else
                sound_mac_voice[voice].mix = mac_mixer_8x1_forward;
            }
         else if(sound_mac_voice[voice].data16) {
            if(sound_mac_voice[voice].stereo)
               sound_mac_voice[voice].mix = mac_mixer_16x2_forward;      
            else
               sound_mac_voice[voice].mix = mac_mixer_16x1_forward;
         }
	 else
	    sound_mac_voice[voice].mix = 0 ;
         break;
      case PLAYMODE_LOOP:
         if (sound_mac_voice[voice].data8) {
            if(sound_mac_voice[voice].stereo)
                sound_mac_voice[voice].mix = mac_mixer_8x2_loop_forward;      
            else
                sound_mac_voice[voice].mix = mac_mixer_8x1_loop_forward;
            }
         else if(sound_mac_voice[voice].data16) {
            if(sound_mac_voice[voice].stereo)
               sound_mac_voice[voice].mix = mac_mixer_16x2_loop_forward;      
            else
               sound_mac_voice[voice].mix = mac_mixer_16x1_loop_forward;
         }
	 else
	    sound_mac_voice[voice].mix = 0 ;
         break;
      case PLAYMODE_PLAY+PLAYMODE_BACKWARD:
         if (sound_mac_voice[voice].data8) {
            if(sound_mac_voice[voice].stereo)
                sound_mac_voice[voice].mix = mac_mixer_8x2_backward;      
            else
                sound_mac_voice[voice].mix = mac_mixer_8x1_backward;
            }
         else if(sound_mac_voice[voice].data16) {
            if(sound_mac_voice[voice].stereo)
               sound_mac_voice[voice].mix = mac_mixer_16x2_backward;      
            else
               sound_mac_voice[voice].mix = mac_mixer_16x1_backward;
         }
	 else
	    sound_mac_voice[voice].mix = 0 ;
         break;
      case PLAYMODE_LOOP+PLAYMODE_BACKWARD:
         if (sound_mac_voice[voice].data8) {
            if(sound_mac_voice[voice].stereo)
                sound_mac_voice[voice].mix = mac_mixer_8x2_loop_backward;      
            else
                sound_mac_voice[voice].mix = mac_mixer_8x1_loop_backward;
            }
         else if(sound_mac_voice[voice].data16) {
            if(sound_mac_voice[voice].stereo)
               sound_mac_voice[voice].mix = mac_mixer_16x2_loop_backward;      
            else
               sound_mac_voice[voice].mix = mac_mixer_16x1_loop_backward;
         }
	 else
	    sound_mac_voice[voice].mix = 0 ;
         break;
      default:
         if (_phys_voice[voice].playmode & PLAYMODE_BIDIR){
            if (sound_mac_voice[voice].data8) {
               if(sound_mac_voice[voice].stereo)
                  sound_mac_voice[voice].mix = mac_mixer_8x2_bidir;      
               else
                  sound_mac_voice[voice].mix = mac_mixer_8x1_bidir;
               }
            else if(sound_mac_voice[voice].data16) {
               if(sound_mac_voice[voice].stereo)
                  sound_mac_voice[voice].mix = mac_mixer_16x2_bidir;      
               else
                  sound_mac_voice[voice].mix = mac_mixer_16x1_bidir;
	       }
            }
	 else
	    sound_mac_voice[voice].mix = 0 ;
	 break;
   }      
}

/* sound_mac_init_voice:
 *  Initialises the specificed voice ready for playing a sample.
 */
static void sound_mac_init_voice(int voice, AL_CONST SAMPLE *sample)
{
   sound_mac_voice[voice].playing = FALSE;
   sound_mac_voice[voice].stereo = sample->stereo;
   sound_mac_voice[voice].pos = 0;
   sound_mac_voice[voice].len = sample->len << MIX_FIX_SHIFT;
   sound_mac_voice[voice].loop_start = sample->loop_start << MIX_FIX_SHIFT;
   sound_mac_voice[voice].loop_end = sample->loop_end << MIX_FIX_SHIFT;

   if (sample->bits == 8) {
      sound_mac_voice[voice].data8 = sample->data;
      sound_mac_voice[voice].data16 = NULL;
   }
   else {
      sound_mac_voice[voice].data8 = NULL;
      sound_mac_voice[voice].data16 = sample->data;
   }

   updatemixer_volume(sound_mac_voice+voice, _phys_voice+voice);
   updatemixer_freq(sound_mac_voice+voice, _phys_voice+voice);
   sound_mac_update_mix(voice);
}

END_OF_STATIC_FUNCTION(sound_mac_init_voice);



/* sound_mac_release_voice:
 *  Releases a voice when it is no longer required.
 */
static void sound_mac_release_voice(int voice)
{
   sound_mac_voice[voice].playing = FALSE;
   sound_mac_voice[voice].data8 = NULL;
   sound_mac_voice[voice].data16 = NULL;
}

END_OF_STATIC_FUNCTION(sound_mac_release_voice);



/* sound_mac_start_voice:
 *  Activates a voice, with the currently selected parameters.
 */
static void sound_mac_start_voice(int voice)
{
   if (sound_mac_voice[voice].pos >= sound_mac_voice[voice].len)
      sound_mac_voice[voice].pos = 0;

   sound_mac_voice[voice].playing = TRUE;
}

END_OF_STATIC_FUNCTION(sound_mac_start_voice);



/* sound_mac_stop_voice:
 *  Stops a voice from playing.
 */
static void sound_mac_stop_voice(int voice)
{
   sound_mac_voice[voice].playing = FALSE;
}

END_OF_STATIC_FUNCTION(sound_mac_stop_voice);



/* sound_mac_loop_voice:
 *  Sets the loopmode for a voice.
 */
static void sound_mac_loop_voice(int voice, int loopmode)
{
   sound_mac_update_mix(voice);
   updatemixer_freq(sound_mac_voice+voice, _phys_voice+voice);
}

END_OF_STATIC_FUNCTION(sound_mac_loop_voice);



/* sound_mac_get_position:
 *  Returns the current play position of a voice, or -1 if it has finished.
 */
static int sound_mac_get_position(int voice)
{
   if ((!sound_mac_voice[voice].playing) ||
       (sound_mac_voice[voice].pos >= sound_mac_voice[voice].len))
      return -1;

   return (sound_mac_voice[voice].pos >> MIX_FIX_SHIFT);
}

END_OF_STATIC_FUNCTION(sound_mac_get_position);



/* sound_mac_set_position:
 *  Sets the current play position of a voice.
 */
static void sound_mac_set_position(int voice, int position)
{
   sound_mac_voice[voice].pos = (position << MIX_FIX_SHIFT);

   if (sound_mac_voice[voice].pos >= sound_mac_voice[voice].len)
      sound_mac_voice[voice].playing = FALSE;
}

END_OF_STATIC_FUNCTION(sound_mac_set_position);



/* sound_mac_get_volume:
 *  Returns the current volume of a voice.
 */
static int sound_mac_get_volume(int voice)
{
   return (_phys_voice[voice].vol >> 12);
}

END_OF_STATIC_FUNCTION(sound_mac_get_volume);



/* sound_mac_set_volume:
 *  Sets the volume of a voice.
 */
static void sound_mac_set_volume(int voice, int volume)
{
   updatemixer_volume(sound_mac_voice+voice, _phys_voice+voice);
}

END_OF_STATIC_FUNCTION(sound_mac_set_volume);



/* sound_mac_ramp_volume:
 *  Starts a volume ramping operation.
 */
static void sound_mac_ramp_volume(int voice, int time, int endvol)
{
   int d = (endvol << 12) - _phys_voice[voice].vol;
   time = MAX(time * (sound_mac_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_vol = endvol << 12;
   _phys_voice[voice].dvol = d / time;
}

END_OF_STATIC_FUNCTION(sound_mac_ramp_volume);



/* sound_mac_stop_volume_ramp:
 *  Ends a volume ramp operation.
 */
static void sound_mac_stop_volume_ramp(int voice)
{
   _phys_voice[voice].dvol = 0;
}

END_OF_STATIC_FUNCTION(sound_mac_stop_volume_ramp);



/* sound_mac_get_frequency:
 *  Returns the current frequency of a voice.
 */
static int sound_mac_get_frequency(int voice)
{
   return (_phys_voice[voice].freq >> 12);
}

END_OF_STATIC_FUNCTION(sound_mac_get_frequency);



/* sound_mac_set_frequency:
 *  Sets the frequency of a voice.
 */
static void sound_mac_set_frequency(int voice, int frequency)
{
   updatemixer_freq(sound_mac_voice+voice, _phys_voice+voice);
}

END_OF_STATIC_FUNCTION(sound_mac_set_frequency);



/* sound_mac_sweep_frequency:
 *  Starts a frequency sweep.
 */
static void sound_mac_sweep_frequency(int voice, int time, int endfreq)
{
   int d = (endfreq << 12) - _phys_voice[voice].freq;
   time = MAX(time * (sound_mac_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_freq = endfreq << 12;
   _phys_voice[voice].dfreq = d / time;
}

END_OF_STATIC_FUNCTION(sound_mac_sweep_frequency);



/* sound_mac_stop_frequency_sweep:
 *  Ends a frequency sweep.
 */
static void sound_mac_stop_frequency_sweep(int voice)
{
   _phys_voice[voice].dfreq = 0;
}

END_OF_STATIC_FUNCTION(sound_mac_stop_frequency_sweep);



/* sound_mac_get_pan:
 *  Returns the current pan position of a voice.
 */
static int sound_mac_get_pan(int voice)
{
   return (_phys_voice[voice].pan >> 12);
}

END_OF_STATIC_FUNCTION(sound_mac_get_pan);



/* sound_mac_set_pan:
 *  Sets the pan position of a voice.
 */
static void sound_mac_set_pan(int voice, int pan)
{
   updatemixer_volume(sound_mac_voice+voice, _phys_voice+voice);
}

END_OF_STATIC_FUNCTION(sound_mac_set_pan);



/* sound_mac_sweep_pan:
 *  Starts a pan sweep.
 */
static void sound_mac_sweep_pan(int voice, int time, int endpan)
{
   int d = (endpan << 12) - _phys_voice[voice].pan;
   time = MAX(time * (sound_mac_freq / UPDATE_FREQ) / 1000, 1);

   _phys_voice[voice].target_pan = endpan << 12;
   _phys_voice[voice].dpan = d / time;
}

END_OF_STATIC_FUNCTION(sound_mac_sweep_pan);



/* sound_mac_stop_pan_sweep:
 *  Ends a pan sweep.
 */
static void sound_mac_stop_pan_sweep(int voice)
{
   _phys_voice[voice].dpan = 0;
}

END_OF_STATIC_FUNCTION(sound_mac_stop_pan_sweep);



/* sound_mac_set_echo:
 *  Sets the echo parameters for a voice.
 */
static void sound_mac_set_echo(int voice, int strength, int delay)
{
   /* not implemented */
}

END_OF_STATIC_FUNCTION(sound_mac_set_echo);



/* sound_mac_set_tremolo:
 *  Sets the tremolo parameters for a voice.
 */
static void sound_mac_set_tremolo(int voice, int rate, int depth)
{
   /* not implemented */
}

END_OF_STATIC_FUNCTION(sound_mac_set_tremolo);



/* sound_mac_set_vibrato:
 *  Sets the amount of vibrato for a voice.
 */
static void sound_mac_set_vibrato(int voice, int rate, int depth)
{
   /* not implemented */
}

END_OF_STATIC_FUNCTION(sound_mac_set_vibrato);





DIGI_DRIVER digi_macos={
   DIGI_MACOS,
   empty_string,
   empty_string,
   "Apple SoundManager",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   /* setup routines */
   sound_mac_detect,
   sound_mac_init,
   sound_mac_exit,
   sound_mac_set_mixer_volume,
   NULL,

   /* for use by the audiostream functions */
   NULL,
   NULL,
   sound_mac_buffer_size,

   /* voice control functions */
   sound_mac_init_voice,
   sound_mac_release_voice,
   sound_mac_start_voice,
   sound_mac_stop_voice,
   sound_mac_loop_voice,

   /* position control functions */
   sound_mac_get_position,
   sound_mac_set_position,

   /* volume control functions */
   sound_mac_get_volume,
   sound_mac_set_volume,
   sound_mac_ramp_volume,
   sound_mac_stop_volume_ramp,

   /* pitch control functions */
   sound_mac_get_frequency,
   sound_mac_set_frequency,
   sound_mac_sweep_frequency,
   sound_mac_stop_frequency_sweep,

   /* pan control functions */
   sound_mac_get_pan,
   sound_mac_set_pan,
   sound_mac_sweep_pan,
   sound_mac_stop_pan_sweep,

   /* effect control functions */
   sound_mac_set_echo,
   sound_mac_set_tremolo,
   sound_mac_set_vibrato,

   /* input functions */
   0,/*int rec_cap_bits;*/
   0,/*int rec_cap_stereo;*/
   NULL,/*AL_METHOD(int,rec_cap_rate, (int bits, int stereo));*/
   NULL,/*AL_METHOD(int,rec_cap_parm, (int rate, int bits, int stereo));*/
   NULL,/*AL_METHOD(int,rec_source, (int source));*/
   NULL,/*AL_METHOD(int,rec_start, (int rate, int bits, int stereo));*/
   NULL,/*AL_METHOD(void, rec_stop, (void));*/
   NULL,/*AL_METHOD(int,rec_read, (void *buf));*/
};


/* sound_mac_interrupt
 *
 */
static void pascal sound_mac_interrupt(SndChannelPtr chan, SndDoubleBufferPtr doubleBuffer)
{
   if(doubleBuffer->dbUserInfo[0] == MAC_MSG_CLOSE){
      doubleBuffer->dbNumFrames = 1;
      doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
      doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbLastBuffer;
      doubleBuffer->dbUserInfo[0] = MAC_MSG_DONE;
   }
   
   else{
      int i;
      unsigned short *p = (unsigned short *)(&(doubleBuffer->dbSoundData[0]));
      BlockZero(p,sound_mac_total_buf_size);
      for (i=0; i<sound_mac_voices; i++) {
	 if (sound_mac_voice[i].playing) {
	    if ((_phys_voice[i].vol > 0) || (_phys_voice[i].dvol > 0)) {
	       if (sound_mac_voice[i].mix)
	          sound_mac_voice[i].mix(sound_mac_voice+i, _phys_voice+i, p, sound_mac_buf_size);
            }
            else
               mix_silent_samples(sound_mac_voice+i, _phys_voice+i, sound_mac_buf_size);
         }
      }
      doubleBuffer->dbNumFrames = sound_mac_buf_size;
      doubleBuffer->dbFlags = doubleBuffer->dbFlags | dbBufferReady;
   }
}
END_OF_STATIC_FUNCTION(sound_mac_interrupt);



/*
 * sound_mac_detect:
 */
static int sound_mac_detect(int input){
#define _sound_has_stereo()	 (_mac_sound & (1<<gestaltStereoCapability))
#define _sound_has_stereo_mix()     (_mac_sound & (1<<gestaltStereoMixing))
#define _sound_has_input()	  (_mac_sound & (1<<gestaltHasSoundInputDevice))
#define _sound_has_play_record()    (_mac_sound & (1<<gestaltPlayAndRecord))
#define _sound_has_16bit()	  (_mac_sound & (1<<gestalt16BitSoundIO))
#define _sound_has_stereo_input()   (_mac_sound & (1<<gestaltStereoInput))
#define _sound_has_double_buffer()  (_mac_sound & (1<<gestaltSndPlayDoubleBuffer))
#define _sound_has_multichannels()  (_mac_sound & (1<<gestaltMultiChannels))
#define _sound_has_16BitAudioSupport() (_mac_sound & (1<<gestalt16BitAudioSupport))

   SndCommand	theCommand;
   NumVersion myVersion;
   OSErr my_err;
   char tmp1[512];
   char *sound = uconvert_ascii("sound", tmp1);

   if(!input){
      if(RTrapAvailable(_SoundDispatch,ToolTrap)){
	 myVersion = SndSoundManagerVersion();
	 if(myVersion.majorRev >= 3){
	    my_err = Gestalt(gestaltSoundAttr, &_mac_sound);
	    if(my_err == noErr){
	       if(_sound_has_double_buffer()){
		  sound_mac_stereo = 1/*_sound_has_stereo()?_sound_stereo:0*/;/*_sound_stereo*/
		  sound_mac_16bit = 1/*(_sound_bits == 16 && _sound_has_16bit())?1:0*/;/*_sound_bits*/
		  sound_mac_freq = (_sound_freq > 0)?_sound_freq:44100;
		  my_err=SndNewChannel(&chan, sampledSynth, sound_mac_stereo?initStereo:initMono, NULL);
		  if(chan && my_err==noErr){
/*		     theCommand.cmd = volumeCmd;*/
/*		     theCommand.param1 = 0;*/
/*		     theCommand.param2 = (long)(&sound_mac_freq);*/
/*		     my_err = SndDoImmediate(chan, &theCommand);*/
/*		     printf("Sound freq %d",sound_mac_freq);*/
/*		     sound_mac_freq = Fix2Long((Fixed)sound_mac_freq);*/
/*                   SndDisposeChannel(chan , true);*/
		     _sound_freq = sound_mac_freq;
		     _sound_bits = sound_mac_16bit?16:8;
		     _sound_stereo = sound_mac_stereo;
		     sound_mac_buf_size = get_config_int(sound, uconvert_ascii("oss_fragsize",  tmp1), 256);
		     uszprintf(sound_mac_desc, sizeof(sound_mac_desc),
		        get_config_text("Apple SoundManager %d.x %d hz, %d-bit, %s , %d "),
		        myVersion.majorRev,
	                sound_mac_freq,
		        sound_mac_16bit ? 16 : 8,
	                uconvert_ascii(sound_mac_stereo ? "stereo" : "mono", tmp1),
	                sound_mac_buf_size);
		     digi_driver->desc=sound_mac_desc;
                     return 1;
		  }
	       }
	       else{
		  ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not do double buffered sound"));		  
	         }
	       }
	    else{
	       ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not get information about sound"));
	    }
	 }
	 else{
	    ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("SoundManager 3.0 or later required"));
	 }
      }
      else{
	 ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("SoundManager not found"));
      }
   }
   else{
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
   }
   return 0;
}



/* sound_mac_init:
 *  returns zero on success, -1 on failure.
 */
static int sound_mac_init(int input, int voices){
   int i;
   OSErr my_err;
   SndDoubleBufferPtr myDblBuffer;
   chan = NULL;
   digi_driver->voices = voices;
   if(input)
      return -1;
   
   my_err=SndNewChannel(&chan, sampledSynth, sound_mac_stereo?initStereo:initMono, NULL);
   if(my_err == noErr){
      sound_mac_voices = 1;

      while ((sound_mac_voices < MIXER_MAX_SFX) && (sound_mac_voices < voices)) {
         sound_mac_voices <<= 1;
      }

      voices = sound_mac_voices;

      for (i=0; i<MIXER_MAX_SFX; i++) {
         sound_mac_voice[i].playing = FALSE;
         sound_mac_voice[i].data8 = NULL;
         sound_mac_voice[i].data16 = NULL;
         sound_mac_voice[i].mix = NULL;
      }
      sound_mac_total_buf_size = sound_mac_buf_size*(sound_mac_16bit?2:1)*(sound_mac_stereo?2:1);
      
      myDblHeader.dbhNumChannels = sound_mac_stereo?2:1;
      myDblHeader.dbhSampleSize = sound_mac_16bit?16:8;
      myDblHeader.dbhCompressionID = 0;
      myDblHeader.dbhPacketSize = 0;
      myDblHeader.dbhSampleRate = Long2Fix(sound_mac_freq);
      myDBUPP = NewSndDoubleBackUPP (sound_mac_interrupt);
      myDblHeader.dbhDoubleBack = myDBUPP;

      for (i = 0; i < 2; i++){
	 myDblBuffer = (SndDoubleBufferPtr)NewPtr(sizeof(SndDoubleBuffer) + sound_mac_total_buf_size);
	 if( myDblBuffer == NULL){
	    my_err=MemError();
	 }
	 else{
	    LOCK_DATA(myDblBuffer,sizeof(SndDoubleBuffer) + sound_mac_total_buf_size);
	    myDblBuffer->dbNumFrames = 0;
	    myDblBuffer->dbFlags = 0;
	    myDblBuffer->dbUserInfo[0] =MAC_MSG_NONE;
	    InvokeSndDoubleBackUPP (chan, myDblBuffer , myDBUPP);
	    myDblHeader.dbhBufferPtr[i] = myDblBuffer;
	 }
      }

      if(my_err==noErr)
	 my_err = SndPlayDoubleBuffer(chan, &myDblHeader);
      if (my_err != noErr){
	 for( i = 0 ; i < 2 ; i++ )
	    if( myDblHeader.dbhBufferPtr[i] != NULL){
	       DisposePtr((Ptr) myDblHeader.dbhBufferPtr[i]);
	       myDblHeader.dbhBufferPtr[i]=NULL;
	    }
      
	 if( myDBUPP != NULL ){
	    DisposeSndDoubleBackUPP (myDBUPP);
	    myDBUPP=NULL;
	 }
	 if( chan!=NULL ){
	    SndDisposeChannel(chan , true);
	    chan=NULL;
	 }
	 return -1;
      }
   }
   else
      return -1;
   sound_mac_set_mixer_volume(512);
   sound_mac_in_use = TRUE;
   sound_mac_lock_mem();
   return 0;
}



/* sound_mac_exit:
 * 
 */
static void sound_mac_exit(int input)
{
   if(input)return;

   if( chan != NULL ){
      SCStatus myStatus;
      OSErr my_err;
      int i;
      
      myDblHeader.dbhBufferPtr[0]->dbUserInfo[0] =MAC_MSG_CLOSE;
      myDblHeader.dbhBufferPtr[1]->dbUserInfo[0] =MAC_MSG_CLOSE;

      do
	 my_err = SndChannelStatus(chan, sizeof(myStatus), &myStatus);
      while(myStatus.scChannelBusy);
      
      for( i = 0 ; i < 2 ; i++ )
	 if( myDblHeader.dbhBufferPtr[i] != NULL){
	    DisposePtr( (Ptr) myDblHeader.dbhBufferPtr[i]);
	    myDblHeader.dbhBufferPtr[i]=NULL;
	 }
	 
      if( myDBUPP != NULL ){
	 DisposeSndDoubleBackUPP ( myDBUPP );   
	 myDBUPP=NULL;
      }

      SndDisposeChannel(chan , true);
      chan=NULL;
   }
   sound_mac_in_use = FALSE;
}



/*
 *
 */
static int sound_mac_buffer_size(void)
{
   return sound_mac_buf_size+(sound_mac_stereo?sound_mac_buf_size:0);
}



/*
 *
 */
static int sound_mac_set_mixer_volume(int volume)
{
   SndCommand	theCommand;
   OSErr	e;
   if(chan){
      theCommand.cmd = volumeCmd;
      theCommand.param1 = 0;
      theCommand.param2 = (long)(volume+(volume<<16));
      e = SndDoImmediate(chan, &theCommand);
   }
   printf("smvol = %d",volume);
   return 0;
}



/* sound_mac_lock_mem:
 *  Locks memory used by the functions in this file.
 */
static void sound_mac_lock_mem()
{
   LOCK_VARIABLE(myDblHeader);
   LOCK_VARIABLE(sound_mac_voice);
   LOCK_VARIABLE(sound_mac_voices);
   LOCK_VARIABLE(sound_mac_freq);
   LOCK_VARIABLE(sound_mac_buf_size);
   LOCK_VARIABLE(sound_mac_total_buf_size);
   LOCK_FUNCTION(mix_silent_samples);
   LOCK_FUNCTION(updatemixer_volume);
   LOCK_FUNCTION(updatemixer_freq);
   LOCK_FUNCTION(update_silent_mixer);
   LOCK_FUNCTION(sound_mac_init_voice);
   LOCK_FUNCTION(sound_mac_release_voice);
   LOCK_FUNCTION(sound_mac_start_voice);
   LOCK_FUNCTION(sound_mac_stop_voice);
   LOCK_FUNCTION(sound_mac_loop_voice);
   LOCK_FUNCTION(sound_mac_get_position);
   LOCK_FUNCTION(sound_mac_set_position);
   LOCK_FUNCTION(sound_mac_get_volume);
   LOCK_FUNCTION(sound_mac_set_volume);
   LOCK_FUNCTION(sound_mac_ramp_volume);
   LOCK_FUNCTION(sound_mac_stop_volume_ramp);
   LOCK_FUNCTION(sound_mac_get_frequency);
   LOCK_FUNCTION(sound_mac_set_frequency);
   LOCK_FUNCTION(sound_mac_sweep_frequency);
   LOCK_FUNCTION(sound_mac_stop_frequency_sweep);
   LOCK_FUNCTION(sound_mac_get_pan);
   LOCK_FUNCTION(sound_mac_set_pan);
   LOCK_FUNCTION(sound_mac_sweep_pan);
   LOCK_FUNCTION(sound_mac_stop_pan_sweep);
   LOCK_FUNCTION(sound_mac_set_echo);
   LOCK_FUNCTION(sound_mac_set_tremolo);
   LOCK_FUNCTION(sound_mac_set_vibrato);
   LOCK_FUNCTION(mac_mixer_8x1_forward);
   LOCK_FUNCTION(mac_mixer_8x1_backward);
   LOCK_FUNCTION(mac_mixer_8x1_loop_forward);
   LOCK_FUNCTION(mac_mixer_8x1_loop_backward);
   LOCK_FUNCTION(mac_mixer_8x1_bidir);
   LOCK_FUNCTION(mac_mixer_8x2_forward);
   LOCK_FUNCTION(mac_mixer_8x2_backward);
   LOCK_FUNCTION(mac_mixer_8x2_loop_forward);
   LOCK_FUNCTION(mac_mixer_8x2_loop_backward);
   LOCK_FUNCTION(mac_mixer_8x2_bidir);
   LOCK_FUNCTION(mac_mixer_16x1_forward);
   LOCK_FUNCTION(mac_mixer_16x1_backward);
   LOCK_FUNCTION(mac_mixer_16x1_loop_forward);
   LOCK_FUNCTION(mac_mixer_16x1_loop_backward);
   LOCK_FUNCTION(mac_mixer_16x1_bidir);
   LOCK_FUNCTION(mac_mixer_16x2_forward);
   LOCK_FUNCTION(mac_mixer_16x2_backward);
   LOCK_FUNCTION(mac_mixer_16x2_loop_forward);
   LOCK_FUNCTION(mac_mixer_16x2_loop_backward);
   LOCK_FUNCTION(mac_mixer_16x2_bidir);
   LOCK_FUNCTION(sound_mac_interrupt);
}
