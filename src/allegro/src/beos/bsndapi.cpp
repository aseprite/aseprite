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
 *      BeOS sound driver implementation.
 *
 *      Originally by Peter Wang, rewritten to use the BSoundPlayer
 *      class by Angelo Mottola.
 *
 *      See readme.txt for copyright information.
 */

#include "bealleg.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintbeos.h"

#if !defined ALLEGRO_BEOS && !defined ALLEGRO_HAIKU
#error something is wrong with the makefile
#endif

#ifndef SCAN_DEPEND
#include <media/SoundPlayer.h>
#endif


#ifdef ALLEGRO_BIG_ENDIAN
   #define BE_SOUND_ENDIAN	 B_MEDIA_BIG_ENDIAN
#else
   #define BE_SOUND_ENDIAN	 B_MEDIA_LITTLE_ENDIAN
#endif


sem_id _be_sound_stream_lock = -1;

static bool be_sound_active = false;
static bool be_sound_stream_locked = false;

static BLocker *locker = NULL;
static BSoundPlayer *be_sound = NULL;

static int be_sound_bufsize;
static int be_sound_signed;

static char be_sound_desc[256] = EMPTY_STRING;



/* be_sound_handler:
 *  Update data.
 */
static void be_sound_handler(void *cookie, void *buffer, size_t size, const media_raw_audio_format &format)
{
   locker->Lock();
   acquire_sem(_be_sound_stream_lock);
   if (be_sound_active) {
      _mix_some_samples((unsigned long)buffer, 0, be_sound_signed);
   }
   else {
      memset(buffer, 0, size);
   }
   release_sem(_be_sound_stream_lock);
   locker->Unlock();
}
      


/* be_sound_detect.
 *  Return TRUE if sound available.
 */
extern "C" int be_sound_detect(int input)
{   
   BSoundPlayer *sound;
   media_raw_audio_format format;
   status_t status;
   
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }

   format.frame_rate = 11025;
   format.channel_count = 1;
   format.format = media_raw_audio_format::B_AUDIO_UCHAR;
   format.byte_order = BE_SOUND_ENDIAN;
   format.buffer_size = 0;
      
   sound = new BSoundPlayer(&format);
   status = sound->InitCheck();
   delete sound;
   
   return (status == B_OK) ? TRUE : FALSE;
}



/* be_sound_init:
 *  Init sound driver.
 */
extern "C" int be_sound_init(int input, int voices)
{
   media_raw_audio_format format;
   char tmp1[128], tmp2[128];

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }

   be_sound_active = false;

   /* create BPushGameSound instance */
   format.frame_rate    = (_sound_freq > 0) ? _sound_freq : 44100;
   format.channel_count = (_sound_stereo) ? 2 : 1;
   format.format	    = (_sound_bits == 8) ? (media_raw_audio_format::B_AUDIO_UCHAR) : (media_raw_audio_format::B_AUDIO_SHORT);
   format.byte_order    = BE_SOUND_ENDIAN;
   format.buffer_size   = 0;

   be_sound = new BSoundPlayer(&format, "Sound player", be_sound_handler);
   
   if (be_sound->InitCheck() != B_OK) {
      goto cleanup;
   }

   /* read the sound format back */
   format = be_sound->Format();

   switch (format.format) {
       
      case media_raw_audio_format::B_AUDIO_UCHAR:
	 _sound_bits = 8;
	 be_sound_signed = FALSE;
	 break;
	 
      case media_raw_audio_format::B_AUDIO_SHORT:
	 _sound_bits = 16;
	 be_sound_signed = TRUE;
	 break;

      default:
	 goto cleanup;
   }
      
   _sound_stereo = (format.channel_count == 2) ? 1 : 0;
   _sound_freq = (int)format.frame_rate;

   /* start internal mixer */
   be_sound_bufsize = format.buffer_size;
   digi_beos.voices = voices;
   
   if (_mixer_init(be_sound_bufsize / (_sound_bits / 8), _sound_freq,
		   _sound_stereo, ((_sound_bits == 16) ? 1 : 0),
		   &digi_beos.voices) != 0) {
      goto cleanup;
   }

   /* start audio output */
   locker = new BLocker();
   if (!locker)
      goto cleanup;
   
   be_sound->Start();
   be_sound->SetHasData(true);

   uszprintf(be_sound_desc, sizeof(be_sound_desc), get_config_text("%d bits, %s, %d bps, %s"),
	     _sound_bits, uconvert_ascii(be_sound_signed ? "signed" : "unsigned", tmp1), 
	     _sound_freq, uconvert_ascii(_sound_stereo ? "stereo" : "mono", tmp2));

   digi_driver->desc = be_sound_desc;
   be_sound_active = true;
   
   return 0;

   cleanup: {
      be_sound_exit(input);
      return -1;
   }
}



/* be_sound_exit:
 *  Shutdown sound driver.
 */
extern "C" void be_sound_exit(int input)
{
   if (input) {
      return;
   }
   be_sound_active = false;
   be_sound->Stop();
   
   acquire_sem(_be_sound_stream_lock);
   _mixer_exit();
   release_sem(_be_sound_stream_lock);
   
   delete be_sound;
   delete locker;
      
   be_sound = NULL;
   locker = NULL;
}



/* be_sound_lock_voice:
 *  Locks audio stream for exclusive access.
 */
extern "C" void *be_sound_lock_voice(int voice, int start, int end)
{
   if (!be_sound_stream_locked) {
      be_sound_stream_locked = true;
      acquire_sem(_be_sound_stream_lock);
   }
   return NULL;
}



/* be_sound_unlock_voice:
 *  Unlocks audio stream.
 */
void be_sound_unlock_voice(int voice)
{
   if (be_sound_stream_locked) {
      be_sound_stream_locked = false;
      release_sem(_be_sound_stream_lock);
   }
}



/* be_sound_buffer_size:
 *  Returns the current buffer size, for use by the audiostream code.
 */
extern "C" int be_sound_buffer_size()
{
   return be_sound_bufsize / (_sound_bits / 8) / (_sound_stereo ? 2 : 1);
}



/* be_sound_set_mixer_volume:
 *  Set mixer volume.
 */
extern "C" int be_sound_set_mixer_volume(int volume)
{
   if (!be_sound)
      return -1;
   
   be_sound->SetVolume((float)volume / 255.0);

   return 0;
}



/* be_sound_get_mixer_volume:
 *  Set mixer volume.
 */
extern "C" int be_sound_get_mixer_volume(void)
{
   if (!be_sound)
      return -1;

   return (int)(be_sound->Volume() * 255.0);
}


/* be_sound_suspend:
 *  Pauses the sound output.
 */
extern "C" void be_sound_suspend(void)
{
   if (!be_sound)
      return;
   locker->Lock();
   be_sound_active = false;
   locker->Unlock();
}



/* be_sound_resume:
 *  Resumes the sound output.
 */
extern "C" void be_sound_resume(void)
{
   if (!be_sound)
      return;
   locker->Lock();
   be_sound_active = true;
   locker->Unlock();
}
