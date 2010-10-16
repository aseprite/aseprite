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
 *      Audio stream functions.
 *
 *      By Shawn Hargreaves (original version by Andrew Ellem).
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"



/* play_audio_stream:
 *  Creates a new audio stream and starts it playing. The length is the
 *  size of each transfer buffer.
 */
AUDIOSTREAM *play_audio_stream(int len, int bits, int stereo, int freq, int vol, int pan)
{
   AUDIOSTREAM *stream;
   int i, bufcount;
   ASSERT(len > 0);
   ASSERT(bits > 0);
   ASSERT(freq > 0);

   /* decide how many buffer fragments we will need */
   if ((digi_driver) && (digi_driver->buffer_size))
      i = digi_driver->buffer_size();
   else
      i = 2048;

   if (len >= i)
      bufcount = 1;
   else
      bufcount = (i + len-1) / len;

   /* create the stream structure */
   stream = _AL_MALLOC(sizeof(AUDIOSTREAM));
   if (!stream)
      return NULL;

   stream->len = len;
   stream->bufcount = bufcount;
   stream->bufnum = 0;
   stream->active = 1;
   stream->locked = NULL;

   /* create the underlying sample */
   stream->samp = create_sample(bits, stereo, freq, len*bufcount*2);
   if (!stream->samp) {
      _AL_FREE(stream);
      return NULL;
   }

   /* fill with silence */
   if (bits == 16) {
      unsigned short *p = stream->samp->data;
      for (i=0; i < len*bufcount*2 * ((stereo) ? 2 : 1); i++)
	 p[i] = 0x8000;
   }
   else {
      unsigned char *p = stream->samp->data;
      for (i=0; i < len*bufcount*2 * ((stereo) ? 2 : 1); i++)
	 p[i] = 0x80;
   }

   LOCK_DATA(stream, sizeof(AUDIOSTREAM));

   /* play the sample in looped mode */
   stream->voice = allocate_voice(stream->samp);
   if (stream->voice < 0) {
      destroy_sample(stream->samp);
      UNLOCK_DATA(stream, sizeof(AUDIOSTREAM));
      _AL_FREE(stream);
      return NULL;
   }

   voice_set_playmode(stream->voice, PLAYMODE_LOOP);
   voice_set_volume(stream->voice, vol);
   voice_set_pan(stream->voice, pan);

   return stream;
}



/* stop_audio_stream:
 *  Destroys an audio stream when it is no longer required.
 */
void stop_audio_stream(AUDIOSTREAM *stream)
{
   ASSERT(stream);
   
   if ((stream->locked) && (digi_driver->unlock_voice))
      digi_driver->unlock_voice(stream->voice);

   voice_stop(stream->voice);
   deallocate_voice(stream->voice);

   destroy_sample(stream->samp);

   UNLOCK_DATA(stream, sizeof(AUDIOSTREAM));
   _AL_FREE(stream); 
}



/* get_audio_stream_buffer:
 *  Returns a pointer to the next audio buffer, or NULL if the previous 
 *  data is still playing. This must be called at regular intervals while
 *  the stream is playing, and you must fill the return address with the
 *  appropriate number (the same length that you specified when you create
 *  the stream) of samples. Call free_audio_stream_buffer() after loading
 *  the new samples, to indicate that the data is now valid.
 */
void *get_audio_stream_buffer(AUDIOSTREAM *stream)
{
   int pos;
   char *data = NULL;
   ASSERT(stream);

   if (stream->bufnum == stream->active * stream->bufcount) {
      /* waiting for the sample to switch halves */
      pos = voice_get_position(stream->voice);

      if (stream->active == 0) {
	 if (pos < stream->len*stream->bufcount)
	    return NULL;
      }
      else {
	 if (pos >= stream->len*stream->bufcount)
	    return NULL;
      }

      stream->active = 1-stream->active;
   }

   /* make sure we got access to the right bit of sample data */
   if (!stream->locked) {
      pos = (1-stream->active) * stream->bufcount * stream->len;

      if (digi_driver->lock_voice)
	 data = digi_driver->lock_voice(stream->voice, pos, pos+stream->bufcount*stream->len);

      if (data) 
	 stream->locked = data;
      else
	 stream->locked = (char *)stream->samp->data + (pos * ((stream->samp->bits==8) ? 1 : sizeof(short)) * ((stream->samp->stereo) ? 2 : 1));
   }

   return (char *)stream->locked + ((stream->bufnum % stream->bufcount) *
				    stream->len *
				    ((stream->samp->bits==8) ? 1 : sizeof(short)) *
				    ((stream->samp->stereo) ? 2 : 1));
}



/* free_audio_stream_buffer:
 *  Indicates that a sample buffer previously returned by a call to
 *  get_audio_stream_buffer() has now been filled with valid data.
 */
void free_audio_stream_buffer(AUDIOSTREAM *stream)
{
   ASSERT(stream);
   
   /* flip buffers */
   stream->bufnum++;

   if (stream->bufnum >= stream->bufcount*2)
      stream->bufnum = 0;

   /* unlock old waveform region */
   if ((stream->locked) && 
       ((stream->bufnum == 0) || (stream->bufnum == stream->bufcount))) {

      if (digi_driver->unlock_voice)
	 digi_driver->unlock_voice(stream->voice);

      stream->locked = NULL;
   }

   /* start playing if it wasn't already */
   if (voice_get_position(stream->voice) == -1)
      voice_start(stream->voice);
}
