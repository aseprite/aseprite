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
 *      Allegro mixer to DirectSound driver.
 *
 *      By Robin Burrows.
 *
 *      Based on original src/win/wdsound.c by Stefan Schimanski
 *      and src/unix/oss.c by Joshua Heyer.
 *
 *      Bugfixes by Javier Gonzalez.
 *
 *      See readme.txt for copyright information.
 */


#define DIRECTSOUND_VERSION 0x0300

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
   #include <dsound.h>
   #include <math.h>

   #ifdef ALLEGRO_MSVC
      #include <mmreg.h>
   #endif
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#define PREFIX_I                "al-dsndmix INFO: "
#define PREFIX_W                "al-dsndmix WARNING: "
#define PREFIX_E                "al-dsndmix ERROR: "


static int digi_dsoundmix_detect(int input);
static int digi_dsoundmix_init(int input, int voices);
static void digi_dsoundmix_exit(int input);
static int digi_dsoundmix_set_mixer_volume(int volume);
static int digi_dsoundmix_get_mixer_volume(void);
static int digi_dsoundmix_buffer_size(void);


/* template driver: will be cloned for each device */
static DIGI_DRIVER digi_dsoundmix =
{
   0,
   empty_string,
   empty_string,
   empty_string,
   0,              // available voices
   0,              // voice number offset
   MIXER_MAX_SFX,  // maximum voices we can support
   MIXER_DEF_SFX,  // default number of voices to use

   /* setup routines */
   digi_dsoundmix_detect,
   digi_dsoundmix_init,
   digi_dsoundmix_exit,
   digi_dsoundmix_set_mixer_volume,
   digi_dsoundmix_get_mixer_volume,

   /* audiostream locking functions */
   NULL,  // AL_METHOD(void *, lock_voice, (int voice, int start, int end));
   NULL,  // AL_METHOD(void, unlock_voice, (int voice));
   digi_dsoundmix_buffer_size,

   /* voice control functions */
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,

   /* position control functions */
   _mixer_get_position,
   _mixer_set_position,

   /* volume control functions */
   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,

   /* pitch control functions */
   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,

   /* pan control functions */
   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,

   /* effect control functions */
   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato,

   /* input functions */
   0,  // int rec_cap_bits;
   0,  // int rec_cap_stereo;
   digi_directsound_rec_cap_rate,
   digi_directsound_rec_cap_param,
   digi_directsound_rec_source,
   digi_directsound_rec_start,
   digi_directsound_rec_stop,
   digi_directsound_rec_read
};


#define MAX_DRIVERS  16
static char *driver_names[MAX_DRIVERS];
static LPGUID driver_guids[MAX_DRIVERS];


#define USE_NEW_CODE 0

/* sound driver globals */
static LPDIRECTSOUND directsound = NULL;
static LPDIRECTSOUNDBUFFER prim_buf = NULL;
static LPDIRECTSOUNDBUFFER alleg_buf = NULL;
static long int initial_volume;
static int _freq, _bits, _stereo;
static int alleg_to_dsound_volume[256];
static unsigned int digidsbufsize;
static unsigned char *digidsbufdata;
#if !USE_NEW_CODE
static unsigned int bufdivs = 16, digidsbufpos;
#else
static int digidsbufdirty;
#endif
static int alleg_buf_paused = FALSE;
static int alleg_buf_vol;



/* ds_err:
 *  Returns a DirectSound error string.
 */
#ifdef DEBUGMODE
static char *ds_err(long err)
{
   static char err_str[64];

   switch (err) {

      case DS_OK:
         _al_sane_strncpy(err_str, "DS_OK", sizeof(err_str));
         break;

      case DSERR_ALLOCATED:
         _al_sane_strncpy(err_str, "DSERR_ALLOCATED", sizeof(err_str));
         break;

      case DSERR_BADFORMAT:
         _al_sane_strncpy(err_str, "DSERR_BADFORMAT", sizeof(err_str));
         break;

      case DSERR_INVALIDPARAM:
         _al_sane_strncpy(err_str, "DSERR_INVALIDPARAM", sizeof(err_str));
         break;

      case DSERR_NOAGGREGATION:
         _al_sane_strncpy(err_str, "DSERR_NOAGGREGATION", sizeof(err_str));
         break;

      case DSERR_OUTOFMEMORY:
         _al_sane_strncpy(err_str, "DSERR_OUTOFMEMORY", sizeof(err_str));
         break;

      case DSERR_UNINITIALIZED:
         _al_sane_strncpy(err_str, "DSERR_UNINITIALIZED", sizeof(err_str));
         break;

      case DSERR_UNSUPPORTED:
         _al_sane_strncpy(err_str, "DSERR_UNSUPPORTED", sizeof(err_str));
         break;

      default:
         _al_sane_strncpy(err_str, "DSERR_UNKNOWN", sizeof(err_str));
         break;
   }

   return err_str;
}
#else
#define ds_err(hr) "\0"
#endif



/* _get_dsalmix_driver:
 *  System driver hook for listing the available sound drivers. This 
 *  generates the device list at runtime, to match whatever DirectSound
 *  devices are available.
 */
DIGI_DRIVER *_get_dsalmix_driver(char *name, LPGUID guid, int num)
{
   DIGI_DRIVER *driver;

   driver = _AL_MALLOC(sizeof(DIGI_DRIVER));
   if (!driver)
      return NULL;

   memcpy(driver, &digi_dsoundmix, sizeof(DIGI_DRIVER));

   driver->id = DIGI_DIRECTAMX(num);

   driver_names[num] = _AL_MALLOC_ATOMIC(strlen(name)+10);
   if (driver_names[num]) {
      _al_sane_strncpy(driver_names[num], "Allegmix ", strlen(name)+10);
      _al_sane_strncpy(driver_names[num]+9, name, strlen(name)+1);
      driver->ascii_name = driver_names[num];
   }

   driver_guids[num] = guid;

   return driver;
}



/* _free_win_dsalmix_name_list
 * Helper function for freeing dynamically generated driver names.
 */
void _free_win_dsalmix_name_list(void)
{
   int i = 0;
   for (i = 0; i < MAX_DRIVERS; i++) {
      if (driver_names[i]) _AL_FREE(driver_names[i]);
   }
}



/* create_dsound_buffer:
 *  Worker function for creating a DirectSound buffer.
 */
static LPDIRECTSOUNDBUFFER create_dsound_buffer(int len, int freq, int bits, int stereo, int vol)
{
   LPDIRECTSOUNDBUFFER snd_buf;
   WAVEFORMATEX wf;
   DSBUFFERDESC dsbdesc;
   HRESULT hr;

   /* setup wave format structure */
   memset(&wf, 0, sizeof(WAVEFORMATEX));
   wf.wFormatTag = WAVE_FORMAT_PCM;
   wf.nChannels = stereo ? 2 : 1;
   wf.nSamplesPerSec = freq;
   wf.wBitsPerSample = bits;
   wf.nBlockAlign = bits * (stereo ? 2 : 1) / 8;
   wf.nAvgBytesPerSec = wf.nSamplesPerSec * wf.nBlockAlign;

   /* setup DSBUFFERDESC structure */
   memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
   dsbdesc.dwSize = sizeof(DSBUFFERDESC);

   /* need volume control and global focus */
   dsbdesc.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS |
                     DSBCAPS_GETCURRENTPOSITION2;

   dsbdesc.dwBufferBytes = len * (bits / 8) * (stereo ? 2 : 1);
   dsbdesc.lpwfxFormat = &wf;

   /* create buffer */
   hr = IDirectSound_CreateSoundBuffer(directsound, &dsbdesc, &snd_buf, NULL);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "create_directsound_buffer() failed (%s).\n", ds_err(hr));
      _TRACE(PREFIX_E " - %d Hz, %s, %d bits\n", freq, stereo ? "stereo" : "mono", bits);
      return NULL;
   }

   /* set volume */
   IDirectSoundBuffer_SetVolume(snd_buf, alleg_to_dsound_volume[CLAMP(0, vol, 255)]);

   return snd_buf;
}



/* digi_dsoundmix_mixer_callback:
 *  Callback function to update sound in the DS buffer.
 */
static void digi_dsoundmix_mixer_callback(void)
{ 
   LPVOID lpvPtr1, lpvPtr2;
   DWORD dwBytes1, dwBytes2;
   DWORD playcurs, writecurs;
   HRESULT hr;
   int switch_mode;

   /* handle display switchs */
   switch_mode = get_display_switch_mode();

   if (alleg_buf_paused) {
      if (_win_app_foreground ||
          (switch_mode == SWITCH_BACKGROUND) || (switch_mode == SWITCH_BACKAMNESIA)) {
         /* get current state of the sound buffer */
         hr = IDirectSoundBuffer_GetStatus(alleg_buf, &dwBytes1);
         if ((hr == DS_OK) && (dwBytes1 & DSBSTATUS_BUFFERLOST)) {
            if(IDirectSoundBuffer_Restore(alleg_buf) != DS_OK)
               return;
            IDirectSoundBuffer_SetVolume(alleg_buf, alleg_buf_vol);
         }

         alleg_buf_paused = FALSE;
         IDirectSoundBuffer_Play(alleg_buf, 0, 0, DSBPLAY_LOOPING);
      }
      else
         return;
   }
   else {
      if (!_win_app_foreground &&
          ((switch_mode == SWITCH_PAUSE) || (switch_mode == SWITCH_AMNESIA))) {
         alleg_buf_paused = TRUE;
         IDirectSoundBuffer_Stop(alleg_buf);
         return;
      }
   }

#if USE_NEW_CODE /* this should work, dammit */
   /* write data into the buffer */
   if (digidsbufdirty) {
      _mix_some_samples((uintptr_t)digidsbufdata, 0, TRUE);
      digidsbufdirty = FALSE;
   }

   hr = IDirectSoundBuffer_GetCurrentPosition(alleg_buf, &playcurs, &writecurs);
   if (FAILED(hr))
      return;

   if (((playcurs-writecurs)%(digidsbufsize*2)) < digidsbufsize)
      return;

   /* Consider the buffer used. Even if the buffer was lost, mark the data as old
      so the mixer doesn't stall */
   digidsbufdirty = TRUE;

   hr = IDirectSoundBuffer_Lock(alleg_buf, 0, digidsbufsize,
                                &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2,
                                DSBLOCK_FROMWRITECURSOR);

   /* only try to restore the buffer once. don't wait around forever */
   if (hr == DSERR_BUFFERLOST) {
      if(IDirectSoundBuffer_Restore(alleg_buf) != DS_OK)
         return;

      IDirectSoundBuffer_Play(alleg_buf, 0, 0, DSBPLAY_LOOPING);
      IDirectSoundBuffer_SetVolume(alleg_buf, alleg_buf_vol);
      hr = IDirectSoundBuffer_Lock(alleg_buf, 0, digidsbufsize,
                                   &lpvPtr1, &dwBytes1, &lpvPtr2, &dwBytes2,
                                   DSBLOCK_FROMWRITECURSOR);
   }

   if (FAILED(hr))
      return;

   /* if the first buffer has enough room, put it all there */
   if(dwBytes1 >= digidsbufsize) {
      dwBytes1 = digidsbufsize;
      memcpy(lpvPtr1, digidsbufdata, dwBytes1);
      dwBytes2 = 0;
   }
   /* else, sput the first part into the first buffer, and the rest into the
      second */
   else {
      memcpy(lpvPtr1, digidsbufdata, dwBytes1);
      dwBytes2 = digidsbufsize-dwBytes1;
      memcpy(lpvPtr2, digidsbufdata+dwBytes1, dwBytes2);
   }

   IDirectSoundBuffer_Unlock(alleg_buf, lpvPtr1, dwBytes1, lpvPtr2, dwBytes2);
#else
   hr = IDirectSoundBuffer_GetCurrentPosition(alleg_buf, &playcurs, &writecurs);
   if (FAILED(hr))
      return;

   writecurs /= (digidsbufsize/bufdivs);
   writecurs += 8;

   while (writecurs > (bufdivs-1))
      writecurs -= bufdivs;

   /* avoid locking the buffer if no data to write */
   if (writecurs == digidsbufpos)
      return;

   hr = IDirectSoundBuffer_Lock(alleg_buf, 0, 0, 
                                &lpvPtr1, &dwBytes1, 
                                &lpvPtr2, &dwBytes2,
                                DSBLOCK_FROMWRITECURSOR | DSBLOCK_ENTIREBUFFER);

   if (FAILED(hr))
      return;

   /* write data into the buffer */
   while (writecurs != digidsbufpos) {
      if (lpvPtr2) {
         memcpy((char*)lpvPtr2 + (((dwBytes1+dwBytes2)/bufdivs)*digidsbufpos),
                digidsbufdata + (((dwBytes1+dwBytes2)/bufdivs)*digidsbufpos),
                (dwBytes1+dwBytes2)/bufdivs);
      }
      else {
         memcpy((char*)lpvPtr1 + (((dwBytes1)/bufdivs)*digidsbufpos),
                digidsbufdata + (((dwBytes1)/bufdivs)*digidsbufpos),
                (dwBytes1)/bufdivs);
      }

      if (++digidsbufpos > (bufdivs-1))
         digidsbufpos = 0;

      _mix_some_samples((uintptr_t) (digidsbufdata+(((dwBytes1+dwBytes2)/bufdivs)*digidsbufpos)), 0, TRUE);
   }

   IDirectSoundBuffer_Unlock(alleg_buf, lpvPtr1, dwBytes1, lpvPtr2, dwBytes2);
#endif
}



/* digi_dsoundmix_detect:
 */
static int digi_dsoundmix_detect(int input)
{
   HRESULT hr;
   int id;

   /* deduce our device number from the driver ID code */
   id = ((digi_driver->id >> 8) & 0xFF) - 'A';

   if (input)
      return digi_directsound_capture_detect(driver_guids[id]);

   if (!directsound) {
      /* initialize DirectSound interface */
      hr = DirectSoundCreate(driver_guids[id], &directsound, NULL);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "DirectSound interface creation failed during detect (%s)\n", ds_err(hr));
         return 0;
      }

      _TRACE(PREFIX_I "DirectSound interface successfully created\n");

      /* release DirectSound */
      IDirectSound_Release(directsound);
      directsound = NULL;
   }

   return 1;
}



/* digi_dsoundmix_init:
 */
static int digi_dsoundmix_init(int input, int voices)
{
   LPVOID lpvPtr1, lpvPtr2;
   DWORD dwBytes1, dwBytes2;
   HRESULT hr;
   DSCAPS dscaps;
   DSBUFFERDESC desc;
   WAVEFORMATEX format;
   HWND allegro_wnd = win_get_window();
   char tmp1[128], tmp2[128];
   int v, id;

   /* deduce our device number from the driver ID code */
   id = ((digi_driver->id >> 8) & 0xFF) - 'A';

   if (input)
      return digi_directsound_capture_init(driver_guids[id]);

   digi_driver->voices = voices;

   /* initialize DirectSound interface */
   hr = DirectSoundCreate(driver_guids[id], &directsound, NULL);
   if (FAILED(hr)) { 
      _TRACE(PREFIX_E "Can't create DirectSound interface (%s)\n", ds_err(hr));
      goto Error; 
   }

   /* set cooperative level */
   hr = IDirectSound_SetCooperativeLevel(directsound, allegro_wnd, DSSCL_PRIORITY);
   if (FAILED(hr))
      _TRACE(PREFIX_W "Can't set DirectSound cooperative level (%s)\n", ds_err(hr));
   else
      _TRACE(PREFIX_I "DirectSound cooperation level set to DSSCL_PRIORITY\n");

   /* get hardware capabilities */
   dscaps.dwSize = sizeof(DSCAPS);
   hr = IDirectSound_GetCaps(directsound, &dscaps);
   if (FAILED(hr)) { 
      _TRACE(PREFIX_E "Can't get DirectSound caps (%s)\n", ds_err(hr));
      goto Error; 
   }

   /* For some reason the audio driver on my machine doesn't seem to set either
    * PRIMARY16BIT or PRIMARY8BIT; of course it actually does support 16-bit
    * sound.
    */
   if (((dscaps.dwFlags & DSCAPS_PRIMARY16BIT) || !(dscaps.dwFlags & DSCAPS_PRIMARY8BIT))
   &&  ((_sound_bits >= 16) || (_sound_bits <= 0)))
      _bits = 16;
   else
      _bits = 8;

   if ((dscaps.dwFlags & DSCAPS_PRIMARYSTEREO) && _sound_stereo)
      _stereo = 1;
   else
      _stereo = 0;

   /* Try to set the requested frequency */
   if ((dscaps.dwMaxSecondarySampleRate > (DWORD)_sound_freq) && (_sound_freq > 0))
      _freq = _sound_freq;
   /* If no frequency is specified, make sure it's clamped to a reasonable value
      by default */
   else if ((_sound_freq <= 0) && (dscaps.dwMaxSecondarySampleRate > 44100))
      _freq = 44100;
   else
      _freq = dscaps.dwMaxSecondarySampleRate;

   _TRACE(PREFIX_I "DirectSound caps: %u bits, %s, %uHz\n", _bits, _stereo ? "stereo" : "mono", _freq);

   memset(&desc, 0, sizeof(DSBUFFERDESC));
   desc.dwSize = sizeof(DSBUFFERDESC);
   desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_GLOBALFOCUS;// | DSBCAPS_CTRLVOLUME | DSBCAPS_STICKYFOCUS;

   hr = IDirectSound_CreateSoundBuffer(directsound, &desc, &prim_buf, NULL);
   if (FAILED(hr))
      _TRACE(PREFIX_W "Can't create primary buffer (%s)\nGlobal volume control won't be available\n", ds_err(hr));

   if (prim_buf) {
      hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
      if (FAILED(hr)) {
         _TRACE(PREFIX_W "Can't get primary buffer format (%s)\n", ds_err(hr));
      }
      else {
         format.nChannels = (_stereo ? 2 : 1);
         format.nSamplesPerSec = _freq;
         format.wBitsPerSample = _bits;
         format.nBlockAlign = (format.wBitsPerSample * format.nChannels) >> 3;
         format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

         hr = IDirectSoundBuffer_SetFormat(prim_buf, &format);
         if (FAILED(hr))
            _TRACE(PREFIX_W "Can't set primary buffer format (%s)\n", ds_err(hr));

         hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
         if (FAILED(hr)) {
            _TRACE(PREFIX_W "Can't get primary buffer format (%s)\n", ds_err(hr));
         }
         else {
            _TRACE(PREFIX_I "primary format:\n");
            _TRACE(PREFIX_I "  %u channels\n  %u Hz\n  %u AvgBytesPerSec\n  %u BlockAlign\n  %u bits\n  %u size\n",
                   format.nChannels, format.nSamplesPerSec, format.nAvgBytesPerSec,
                   format.nBlockAlign, format.wBitsPerSample, format.cbSize);
         }
      }
   }

   /* setup volume lookup table */
   alleg_to_dsound_volume[0] = DSBVOLUME_MIN;
   for (v = 1; v < 256; v++) {
      int dB = DSBVOLUME_MAX + 2000.0*log10(v/255.0);
      alleg_to_dsound_volume[v] = MAX(DSBVOLUME_MIN, dB);
   }

   /* create the sound buffer to mix into */
   alleg_buf = create_dsound_buffer(get_config_int(uconvert_ascii("sound", tmp1),
                                                   uconvert_ascii("dsound_numfrags", tmp2),
                                                   6) * 1024,
                                    _freq, _bits, _stereo, 255);
   if(!alleg_buf) {
      _TRACE(PREFIX_E "Can't create mixing buffer\n");
      return -1;
   }

   /* fill the sound buffer with zeros */
   hr = IDirectSoundBuffer_Lock(alleg_buf, 0, 0, &lpvPtr1, &dwBytes1, 
                                &lpvPtr2, &dwBytes2, DSBLOCK_ENTIREBUFFER);

   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't lock sound buffer (%s)\n", ds_err(hr));
      return -1;
   }

   memset(lpvPtr1, 0, dwBytes1);
   digidsbufsize = dwBytes1;

   if (lpvPtr2) {
      _TRACE(PREFIX_E "Second buffer set when locking buffer. Error?\n");
      memset(lpvPtr2, 0, dwBytes2);
      digidsbufsize += dwBytes2;
   }

   IDirectSoundBuffer_Unlock(alleg_buf, lpvPtr1, dwBytes1, lpvPtr2, dwBytes2); 

   _TRACE(PREFIX_I "Sound buffer length is %u\n", digidsbufsize);

   /* shouldn't ever happen */
   if (digidsbufsize&1023) {
      _TRACE(PREFIX_E "Sound buffer is not multiple of 1024, failed\n");
      return -1;
   }

#if USE_NEW_CODE
   digidsbufsize /= 2;
#endif
   digidsbufdata = _AL_MALLOC_ATOMIC(digidsbufsize);
   if (!digidsbufdata) {
      _TRACE(PREFIX_E "Can't create temp buffer\n");
      return -1;
   }

#if USE_NEW_CODE
   if (_mixer_init(digidsbufsize / (_bits /8), _freq, _stereo,
                   ((_bits == 16) ? 1 : 0), &digi_driver->voices) != 0) {
      _TRACE(PREFIX_E "Can't init software mixer\n");
      return -1;
   }
#else
   bufdivs = digidsbufsize/1024;
   if (_mixer_init((digidsbufsize / (_bits /8)) / bufdivs, _freq,
                   _stereo, ((_bits == 16) ? 1 : 0), &digi_driver->voices) != 0) {
      _TRACE(PREFIX_E "Can't init software mixer\n");
      return -1;
   }

   _mix_some_samples((uintptr_t)digidsbufdata, 0, TRUE);
#endif

   /* get primary buffer volume */
   IDirectSoundBuffer_GetVolume(alleg_buf, &initial_volume);
   alleg_buf_vol = initial_volume;

   /* mark the buffer as paused, so the mixer callback will start it */
   alleg_buf_paused = TRUE;
#if USE_NEW_CODE
   digidsbufdirty = TRUE;
#endif
   install_int(digi_dsoundmix_mixer_callback, 20);  /* 50 Hz */

   return 0;

 Error:
   _TRACE(PREFIX_E "digi_directsound_init() failed\n");
   digi_dsoundmix_exit(0);
   return -1;
}



/* digi_dsoundmix_exit:
 */
static void digi_dsoundmix_exit(int input)
{
   if (input) {
      digi_directsound_capture_exit();
      return;
   }
	
   /* stop playing */
   remove_int(digi_dsoundmix_mixer_callback);

   if (digidsbufdata) {
      _AL_FREE(digidsbufdata);
      digidsbufdata = NULL;
   }

   /* destroy mixer buffer */
   if (alleg_buf) {
      IDirectSoundBuffer_Release(alleg_buf);
      alleg_buf = NULL;
   }

   /* destroy primary buffer */
   if (prim_buf) {
      /* restore primary buffer initial volume */
      IDirectSoundBuffer_SetVolume(prim_buf, initial_volume);

      IDirectSoundBuffer_Release(prim_buf);
      prim_buf = NULL;
   }

   /* shutdown DirectSound */
   if (directsound) {
      IDirectSound_Release(directsound);
      directsound = NULL;
   }
}



/* digi_dsoundmix_set_mixer_volume:
 */
static int digi_dsoundmix_set_mixer_volume(int volume)
{
   if (prim_buf) {
      alleg_buf_vol = alleg_to_dsound_volume[CLAMP(0, volume, 255)];
      IDirectSoundBuffer_SetVolume(alleg_buf, alleg_buf_vol);
   }

   return 0;
}



/* digi_dsoundmix_get_mixer_volume:
 */
static int digi_dsoundmix_get_mixer_volume(void)
{
   LONG vol;

   if (!prim_buf)
     return -1;

   IDirectSoundBuffer_GetVolume(alleg_buf, &vol);
   vol = CLAMP(0, pow(10, (vol/2000.0))*255.0 - DSBVOLUME_MAX, 255);

   return vol;
}



/* digi_dsoundmix_buffer_size:
 */
static int digi_dsoundmix_buffer_size(void)
{
   return digidsbufsize / (_bits / 8) / (_stereo ? 2 : 1);
}
