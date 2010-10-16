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
 *      Streaming sound routines.
 *
 *      By Shawn Hargreaves.
 *
 *      See readme.txt for copyright information.
 */


#ifndef ALLEGRO_STREAM_H
#define ALLEGRO_STREAM_H

#include "base.h"

#ifdef __cplusplus
   extern "C" {
#endif

struct SAMPLE;

typedef struct AUDIOSTREAM
{
   int voice;                          /* the voice we are playing on */
   struct SAMPLE *samp;                /* the sample we are using */
   int len;                            /* buffer length */
   int bufcount;                       /* number of buffers per sample half */
   int bufnum;                         /* current refill buffer */
   int active;                         /* which half is currently playing */
   void *locked;                       /* the locked buffer */
} AUDIOSTREAM;

AL_FUNC(AUDIOSTREAM *, play_audio_stream, (int len, int bits, int stereo, int freq, int vol, int pan));
AL_FUNC(void, stop_audio_stream, (AUDIOSTREAM *stream));
AL_FUNC(void *, get_audio_stream_buffer, (AUDIOSTREAM *stream));
AL_FUNC(void, free_audio_stream_buffer, (AUDIOSTREAM *stream));

#ifdef __cplusplus
   }
#endif

#endif          /* ifndef ALLEGRO_STREAM_H */


