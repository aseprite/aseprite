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
 *      DirectSound sound driver.
 *
 *      By Stefan Schimanski.
 *
 *      Various bugfixes by Patrick Hogan.
 *
 *      Custom loop points support added by Eric Botcazou.
 *
 *      Backward playing support, bidirectional looping support
 *      and bugfixes by Javier Gonzalez.
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

#define PREFIX_I                "al-dsound INFO: "
#define PREFIX_W                "al-dsound WARNING: "
#define PREFIX_E                "al-dsound ERROR: "


static int digi_directsound_detect(int input);
static int digi_directsound_init(int input, int voices);
static void digi_directsound_exit(int input);
static int digi_directsound_set_mixer_volume(int volume);
static int digi_directsound_get_mixer_volume(void);

static void digi_directsound_init_voice(int voice, AL_CONST SAMPLE * sample);
static void digi_directsound_release_voice(int voice);
static void digi_directsound_start_voice(int voice);
static void digi_directsound_stop_voice(int voice);
static void digi_directsound_loop_voice(int voice, int playmode);

static void *digi_directsound_lock_voice(int voice, int start, int end);
static void digi_directsound_unlock_voice(int voice);

static int digi_directsound_get_position(int voice);
static void digi_directsound_set_position(int voice, int position);

static int digi_directsound_get_volume(int voice);
static void digi_directsound_set_volume(int voice, int volume);

static int digi_directsound_get_frequency(int voice);
static void digi_directsound_set_frequency(int voice, int frequency);

static int digi_directsound_get_pan(int voice);
static void digi_directsound_set_pan(int voice, int pan);


/* template driver: will be cloned for each device */
static DIGI_DRIVER digi_directsound = 
{
   0,
   empty_string,
   empty_string,
   empty_string,
   0,   // available voices
   0,   // voice number offset
   16,  // maximum voices we can support
   4,   // default number of voices to use

   /* setup routines */
   digi_directsound_detect,
   digi_directsound_init,
   digi_directsound_exit,
   digi_directsound_set_mixer_volume,
   digi_directsound_get_mixer_volume,

   /* audiostream locking functions */
   digi_directsound_lock_voice,
   digi_directsound_unlock_voice,
   NULL,  // AL_METHOD(int,  buffer_size, (void));

   /* voice control functions */
   digi_directsound_init_voice,
   digi_directsound_release_voice,
   digi_directsound_start_voice,
   digi_directsound_stop_voice,
   digi_directsound_loop_voice,

   /* position control functions */
   digi_directsound_get_position,
   digi_directsound_set_position,

   /* volume control functions */
   digi_directsound_get_volume,
   digi_directsound_set_volume,
   NULL,  // AL_METHOD(void, ramp_volume, (int voice, int time, int endvol));
   NULL,  // AL_METHOD(void, stop_volume_ramp, (int voice));

   /* pitch control functions */
   digi_directsound_get_frequency,
   digi_directsound_set_frequency,
   NULL,  // AL_METHOD(void, sweep_frequency, (int voice, int time, int endfreq));
   NULL,  // AL_METHOD(void, stop_frequency_sweep, (int voice));

   /* pan control functions */
   digi_directsound_get_pan,
   digi_directsound_set_pan,
   NULL,  // AL_METHOD(void, sweep_pan, (int voice, int time, int endpan));
   NULL,  // AL_METHOD(void, stop_pan_sweep, (int voice));

   /* effect control functions */
   NULL,  // AL_METHOD(void, set_echo, (int voice, int strength, int delay));
   NULL,  // AL_METHOD(void, set_tremolo, (int voice, int rate, int depth));
   NULL,  // AL_METHOD(void, set_vibrato, (int voice, int rate, int depth));

   /* input functions */
   0,                           /* int rec_cap_bits; */
   0,                           /* int rec_cap_stereo; */
   digi_directsound_rec_cap_rate,
   digi_directsound_rec_cap_param,
   digi_directsound_rec_source,
   digi_directsound_rec_start,
   digi_directsound_rec_stop,
   digi_directsound_rec_read
};


/* sound driver globals */
static LPDIRECTSOUND directsound = NULL;
static LPDIRECTSOUNDBUFFER prim_buf = NULL;
static long int initial_volume;
static int _freq, _bits, _stereo;
static int alleg_to_dsound_volume[256];
static int alleg_to_dsound_pan[256];


/* internal driver representation of a voice */
struct DIRECTSOUND_VOICE {
   int bits;
   int bytes_per_sample;
   int freq, pan, vol;
   int stereo;
   int reversed;
   int bidir;
   int len;
   unsigned char *data;
   int loop_offset;
   int loop_len;
   int looping;
   void *lock_buf_a;
   long lock_size_a;
   LPDIRECTSOUNDBUFFER ds_buffer;
   LPDIRECTSOUNDBUFFER ds_loop_buffer;
   LPDIRECTSOUNDBUFFER ds_locked_buffer;
};

static struct DIRECTSOUND_VOICE *ds_voices;


/* dynamically generated driver list */
static _DRIVER_INFO *driver_list = NULL;

#define MAX_DRIVERS  16
static int num_drivers = 0;
static char *driver_names[MAX_DRIVERS];
static LPGUID driver_guids[MAX_DRIVERS];



/* DSEnumCallback:
 *  Callback function for enumerating the available DirectSound drivers.
 */
static BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext)
{
   if (lpGuid) {
      driver_guids[num_drivers] = lpGuid;
      driver_names[num_drivers] = _AL_MALLOC_ATOMIC(strlen(lpcstrDescription)+1);
      if(driver_names[num_drivers])
      {
         _al_sane_strncpy(driver_names[num_drivers], lpcstrDescription, strlen(lpcstrDescription)+1);
         num_drivers++;
      }
   }

   return (num_drivers < MAX_DRIVERS);
}



/* _get_win_digi_driver_list:
 *  System driver hook for listing the available sound drivers. This 
 *  generates the device list at runtime, to match whatever DirectSound
 *  devices are available.
 */
_DRIVER_INFO *_get_win_digi_driver_list(void)
{
   DIGI_DRIVER *driver;
   HRESULT hr;
   int i;

   if (!driver_list) {
      driver_list = _create_driver_list();

      /* enumerate the DirectSound drivers */
      hr = DirectSoundEnumerate(DSEnumCallback, NULL);

      if (hr == DS_OK) {
         /* Allegro mixer to DirectSound drivers */
         for (i=0; i<num_drivers; i++) {
            driver = _get_dsalmix_driver(driver_names[i], driver_guids[i], i);

            _driver_list_append_driver(&driver_list, driver->id, driver, TRUE);
         }

         /* pure DirectSound drivers */
         for (i=0; i<num_drivers; i++) {
            driver = _AL_MALLOC(sizeof(DIGI_DRIVER));
            memcpy(driver, &digi_directsound, sizeof(DIGI_DRIVER));
            driver->id = DIGI_DIRECTX(i);
            driver->ascii_name = driver_names[i];

            _driver_list_append_driver(&driver_list, driver->id, driver, TRUE);
         }
      }

      /* Allegro mixer to WaveOut drivers */
      for (i=0; i<2; i++) {
         driver = _get_woalmix_driver(i);

         _driver_list_append_driver(&driver_list, driver->id, driver, TRUE);
      }
   }

   return driver_list;
}



/* _free_win_digi_driver_list:
 *  Helper function for freeing the dynamically generated driver list.
 */
void _free_win_digi_driver_list(void)
{
   int i = 0;

   if (driver_list) {
      while (driver_list[i].driver) {
         _AL_FREE(driver_list[i].driver);
         i++;
      }

      _destroy_driver_list(driver_list);
      driver_list = NULL;
   }

   for (i = 0; i < num_drivers; i++) {
      _AL_FREE(driver_names[i]);
   }

   _free_win_dsalmix_name_list();
}



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



/* digi_directsound_detect:
 */
static int digi_directsound_detect(int input)
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
         _TRACE(PREFIX_E "DirectSound interface creation failed during detect (%s).\n", ds_err(hr));
         return 0;
      }

      _TRACE(PREFIX_I "DirectSound interface successfully created.\n");

      /* release DirectSound interface */
      IDirectSound_Release(directsound);
      directsound = NULL;
   }

   return 1;
}



/* digi_directsound_init:
 */
static int digi_directsound_init(int input, int voices)
{
   HRESULT hr;
   DSCAPS dscaps;
   DSBUFFERDESC desc;
   WAVEFORMATEX format;
   int v, id;
   HWND allegro_wnd = win_get_window();
   
   /* deduce our device number from the driver ID code */
   id = ((digi_driver->id >> 8) & 0xFF) - 'A';

   if (input)
      return digi_directsound_capture_init(driver_guids[id]);

   digi_driver->voices = voices;

   /* initialize DirectSound interface */
   hr = DirectSoundCreate(driver_guids[id], &directsound, NULL);
   if (FAILED(hr)) { 
      _TRACE(PREFIX_E "Can't create DirectSound interface (%s).\n", ds_err(hr));
      goto Error; 
   }

   /* set cooperative level */
   hr = IDirectSound_SetCooperativeLevel(directsound, allegro_wnd, DSSCL_PRIORITY);
   if (FAILED(hr))
      _TRACE(PREFIX_W "Can't set DirectSound cooperative level (%s).\n", ds_err(hr));
   else
      _TRACE(PREFIX_I "DirectSound cooperation level set to PRIORITY.\n");

   /* get hardware capabilities */
   dscaps.dwSize = sizeof(DSCAPS);
   hr = IDirectSound_GetCaps(directsound, &dscaps);
   if (FAILED(hr)) { 
      _TRACE(PREFIX_E "Can't get DirectSound caps (%s).\n", ds_err(hr));
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

   if ((dscaps.dwMaxSecondarySampleRate > (DWORD)_sound_freq) && (_sound_freq > 0))
      _freq = _sound_freq;
   else
      _freq = dscaps.dwMaxSecondarySampleRate;

   _TRACE(PREFIX_I "DirectSound caps: %u bits, %s, %uHz\n", _bits, _stereo ? "stereo" : "mono", _freq);

   /* create primary buffer */
   memset(&desc, 0, sizeof(DSBUFFERDESC));
   desc.dwSize = sizeof(DSBUFFERDESC);
   desc.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRLVOLUME;

   hr = IDirectSound_CreateSoundBuffer(directsound, &desc, &prim_buf, NULL);
   if (hr != DS_OK) { 
      _TRACE(PREFIX_W "Can't create primary buffer (%s)\nGlobal volume control won't be available.\n", ds_err(hr));
   }

   /* get current format */
   if (prim_buf) {
      hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
      if (FAILED(hr)) {
         _TRACE(PREFIX_W "Can't get primary buffer format (%s).\n", ds_err(hr));
      }
      else {
         format.nChannels = _stereo ? 2 : 1;
         format.nSamplesPerSec = _freq;
         format.wBitsPerSample = _bits;
         format.nBlockAlign = (format.wBitsPerSample * format.nChannels) >> 3;
         format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

         hr = IDirectSoundBuffer_SetFormat(prim_buf, &format);
         if (FAILED(hr)) {
            _TRACE(PREFIX_W "Can't set primary buffer format (%s).\n", ds_err(hr));
         }

         /* output primary format */
         hr = IDirectSoundBuffer_GetFormat(prim_buf, &format, sizeof(format), NULL);
         if (FAILED(hr)) {
            _TRACE(PREFIX_W "Can't get primary buffer format (%s).\n", ds_err(hr));
         }
         else {
            _TRACE(PREFIX_I "primary format:\n");
            _TRACE(PREFIX_I "  %u channels\n  %u Hz\n  %u AvgBytesPerSec\n  %u BlockAlign\n  %u bits\n  %u size\n",
            format.nChannels, format.nSamplesPerSec, format.nAvgBytesPerSec,
            format.nBlockAlign, format.wBitsPerSample, format.cbSize);
         }
      }

      /* get primary buffer (global) volume */
      IDirectSoundBuffer_GetVolume(prim_buf, &initial_volume);
   }

   /* initialize physical voices */
   ds_voices = (struct DIRECTSOUND_VOICE *)_AL_MALLOC(sizeof(struct DIRECTSOUND_VOICE) * voices);
   for (v = 0; v < digi_driver->voices; v++) {
      ds_voices[v].ds_buffer = NULL;
      ds_voices[v].ds_loop_buffer = NULL;
      ds_voices[v].ds_locked_buffer = NULL;
   }

   /* setup volume lookup table */
   alleg_to_dsound_volume[0] = DSBVOLUME_MIN;
   for (v = 1; v < 256; v++)
      alleg_to_dsound_volume[v] = MAX(DSBVOLUME_MIN, DSBVOLUME_MAX + 2000.0*log10(v/255.0));

   /* setup pan lookup table */
   alleg_to_dsound_pan[0] = DSBPAN_LEFT;
   for (v = 1; v < 128; v++)
      alleg_to_dsound_pan[v] = MAX(DSBPAN_LEFT, DSBPAN_CENTER + 2000.0*log10(v/127.0));

   alleg_to_dsound_pan[255] = DSBPAN_RIGHT;
   for (v = 128; v < 255; v++)
      alleg_to_dsound_pan[v] = MIN(DSBPAN_RIGHT, DSBPAN_CENTER - 2000.0*log10((255.0-v)/127.0));

   return 0;

 Error:
   _TRACE(PREFIX_E "digi_directsound_init() failed.\n");
   digi_directsound_exit(FALSE);
   return -1;
}



/* digi_directsound_exit:
 */
static void digi_directsound_exit(int input)
{
   int v;

   if (input) {
      digi_directsound_capture_exit();
      return;
   }

   /* destroy physical voices */
   for (v = 0; v < digi_driver->voices; v++)
      digi_directsound_release_voice(v);

   _AL_FREE(ds_voices);
   ds_voices = NULL;

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



/* digi_directsound_set_mixer_volume:
 */
static int digi_directsound_set_mixer_volume(int volume)
{
   int ds_vol;

   if (prim_buf) {
      ds_vol = alleg_to_dsound_volume[CLAMP(0, volume, 255)];
      IDirectSoundBuffer_SetVolume(prim_buf, ds_vol); 
   }

   return 0;
}



/* digi_directsound_get_mixer_volume:
 */
static int digi_directsound_get_mixer_volume(void)
{
   LONG vol;

   if (!prim_buf)
      return -1;

   IDirectSoundBuffer_GetVolume(prim_buf, &vol);
   vol = CLAMP(0, pow(10, (vol/2000.0))*255.0 - DSBVOLUME_MAX, 255);

   return vol;
}



/********************************************************/
/*********** voice management functions *****************/
/********************************************************/


/* create_dsound_buffer:
 *  Worker function for creating a DirectSound buffer.
 */
static LPDIRECTSOUNDBUFFER create_dsound_buffer(int len, int freq, int bits, int stereo, int vol, int pan)
{
   LPDIRECTSOUNDBUFFER snd_buf;
   WAVEFORMATEX wf;
   DSBUFFERDESC dsbdesc;
   HRESULT hr;
   int switch_mode;

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

   /* need default controls (pan, volume, frequency) */
   dsbdesc.dwFlags = DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY;

   switch_mode = get_display_switch_mode();
   if ((switch_mode == SWITCH_BACKAMNESIA) || (switch_mode == SWITCH_BACKGROUND))
      dsbdesc.dwFlags |= DSBCAPS_GLOBALFOCUS;

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

   /* set pan */
   IDirectSoundBuffer_SetPan(snd_buf, alleg_to_dsound_pan[CLAMP(0, pan, 255)]);

   return snd_buf;
}



/* fill_dsound_buffer:
 *  Worker function for copying data to a DirectSound buffer.
 */
static int fill_dsound_buffer(LPDIRECTSOUNDBUFFER snd_buf, int offset, int len, int bits, int stereo, int reversed, unsigned char *data)
{
   void *buf_a;
   unsigned long int size, size_a;
   HRESULT hr;

   /* transform from samples to bytes */
   size = len * (bits / 8) * (stereo ? 2 : 1);
   offset = offset * (bits / 8) * (stereo ? 2 : 1);

   /* lock the buffer portion */
   hr = IDirectSoundBuffer_Lock(snd_buf, offset, size, &buf_a, &size_a, NULL, NULL, 0);

   /* if buffer lost, restore and retry lock */
   if (hr == DSERR_BUFFERLOST) {
      IDirectSoundBuffer_Restore(snd_buf);

      hr = IDirectSoundBuffer_Lock(snd_buf, offset, size, &buf_a, &size_a, NULL, NULL, 0);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "fill_dsound_buffer() failed (%s).\n", ds_err(hr));
         return -1;
      }
   }

   if (reversed) {
      if (stereo) {
         if (bits == 8) {
            /* 8-bit stereo */
            unsigned short *read_p =  (unsigned short *)data + len - 1;
            unsigned short *write_p = (unsigned short *)buf_a;

            while (len--)
               *write_p++ = *read_p--;
         }
         else if (bits == 16) {
            /* 16-bit stereo (conversion needed) */
            unsigned long *read_p =  (unsigned long *)data + len - 1;
            unsigned long *write_p = (unsigned long *)buf_a;

            while (len--)
               *write_p++ = *read_p-- ^ 0x80008000;
         }
      }
      else {
         if (bits == 8) {
            /* 8-bit mono */
            unsigned char *read_p =  (unsigned char *)data + len - 1;
            unsigned char *write_p = (unsigned char *)buf_a;

            while (len--)
               *write_p++ = *read_p--;
         }
         else {
            /* 16-bit mono (conversion needed) */
            unsigned short *read_p =  (unsigned short *)data + len - 1;
            unsigned short *write_p = (unsigned short *)buf_a;

            while (len--)
               *write_p++ = *read_p-- ^ 0x8000;
         }
      }
   }
   else {
      if (bits == 8) {
         /* 8-bit mono and stereo */
         memcpy(buf_a, data, size);
      }
      else if (bits == 16) {
         /* 16-bit mono and stereo (conversion needed) */
         unsigned short *read_p =  (unsigned short *)data;
         unsigned short *write_p = (unsigned short *)buf_a;

         size >>= 1;

         while (size--)
            *write_p++ = *read_p++ ^ 0x8000;
      }
   }

   /* unlock the buffer */
   IDirectSoundBuffer_Unlock(snd_buf, buf_a, size_a, NULL, 0);

   return 0;
}



/* digi_directsound_init_voice:
 */
static void digi_directsound_init_voice(int voice, AL_CONST SAMPLE *sample)
{
   ds_voices[voice].bits = sample->bits;
   ds_voices[voice].bytes_per_sample = (sample->bits/8) * (sample->stereo ? 2 : 1);
   ds_voices[voice].freq = sample->freq;
   ds_voices[voice].pan = 128;
   ds_voices[voice].vol = 255;
   ds_voices[voice].stereo = sample->stereo;
   ds_voices[voice].reversed = FALSE;
   ds_voices[voice].bidir = FALSE;
   ds_voices[voice].len = sample->len;
   ds_voices[voice].data = (unsigned char *)sample->data;
   ds_voices[voice].loop_offset = sample->loop_start;
   ds_voices[voice].loop_len = sample->loop_end - sample->loop_start;
   ds_voices[voice].looping = FALSE;
   ds_voices[voice].lock_buf_a = NULL;
   ds_voices[voice].lock_size_a = 0;
   ds_voices[voice].ds_locked_buffer = NULL;
   ds_voices[voice].ds_loop_buffer = NULL;
   ds_voices[voice].ds_buffer = create_dsound_buffer(ds_voices[voice].len,
                                                     ds_voices[voice].freq,
                                                     ds_voices[voice].bits,
                                                     ds_voices[voice].stereo,
                                                     ds_voices[voice].vol,
                                                     ds_voices[voice].pan);
   if (!ds_voices[voice].ds_buffer)
      return;

   fill_dsound_buffer(ds_voices[voice].ds_buffer,
                      0,  /* offset */
                      ds_voices[voice].len,
                      ds_voices[voice].bits,
                      ds_voices[voice].stereo,
                      ds_voices[voice].reversed,
                      ds_voices[voice].data);
}



/* digi_directsound_release_voice:
 */
static void digi_directsound_release_voice(int voice)
{
   /* just in case */
   digi_directsound_unlock_voice(voice);

   if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_Release(ds_voices[voice].ds_buffer);
      ds_voices[voice].ds_buffer = NULL;
   }

   if (ds_voices[voice].ds_loop_buffer) {
      IDirectSoundBuffer_Release(ds_voices[voice].ds_loop_buffer);
      ds_voices[voice].ds_loop_buffer = NULL;
   }
}



/* digi_directsound_start_voice:
 */
static void digi_directsound_start_voice(int voice)
{
   if (ds_voices[voice].looping && ds_voices[voice].ds_loop_buffer) {
      IDirectSoundBuffer_Play(ds_voices[voice].ds_loop_buffer, 0, 0, DSBPLAY_LOOPING);
   }
   else if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_Play(ds_voices[voice].ds_buffer, 0, 0,
                              ds_voices[voice].looping ? DSBPLAY_LOOPING : 0);
   }
}



/* digi_directsound_stop_voice:
 */
static void digi_directsound_stop_voice(int voice)
{
   if (ds_voices[voice].looping && ds_voices[voice].ds_loop_buffer) {
      IDirectSoundBuffer_Stop(ds_voices[voice].ds_loop_buffer);
   }
   else if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_Stop(ds_voices[voice].ds_buffer);
   }
}



/* update_voice_buffers:
 *  Worker function for updating the voice buffers with the current
 *  parameters.
 */
static void update_voice_buffers(int voice, int reversed, int bidir, int loop)
{
   int update_main_buffer = FALSE;
   int update_loop_buffer = FALSE;
   int update_reversed = (ds_voices[voice].reversed != reversed);
   int update_loop = (ds_voices[voice].looping != loop);
   int update_bidir = (ds_voices[voice].bidir != bidir);

   /* no work to do? */
   if (!update_reversed && !update_bidir && !update_loop)
      return;

   /* we need to change looping or bidir? */
   if (update_loop || update_bidir) {
      ds_voices[voice].looping = loop;
      ds_voices[voice].bidir = bidir;

      if (!loop && ds_voices[voice].ds_loop_buffer) {
         /* we don't need a loop buffer anymore, destroy it */
         if (ds_voices[voice].ds_locked_buffer == ds_voices[voice].ds_loop_buffer)
            digi_directsound_unlock_voice(voice);
         IDirectSoundBuffer_Release(ds_voices[voice].ds_loop_buffer);
         ds_voices[voice].ds_loop_buffer = NULL;
      }
      else if (loop && !ds_voices[voice].ds_loop_buffer) {
         /* we haven't got one, create it */
         ds_voices[voice].ds_loop_buffer = create_dsound_buffer(ds_voices[voice].loop_len * (bidir ? 2 : 1),
                                                                ds_voices[voice].freq,
                                                                ds_voices[voice].bits,
                                                                ds_voices[voice].stereo,
                                                                ds_voices[voice].vol,
                                                                ds_voices[voice].pan);
         update_loop_buffer = TRUE;
      }
      else if (update_bidir) {
         /* we need to resize the loop buffer */
         if (ds_voices[voice].ds_locked_buffer == ds_voices[voice].ds_loop_buffer)
            digi_directsound_unlock_voice(voice);
         IDirectSoundBuffer_Release(ds_voices[voice].ds_loop_buffer);
         ds_voices[voice].ds_loop_buffer = create_dsound_buffer(ds_voices[voice].loop_len * (bidir ? 2 : 1),
                                                                ds_voices[voice].freq,
                                                                ds_voices[voice].bits,
                                                                ds_voices[voice].stereo,
                                                                ds_voices[voice].vol,
                                                                ds_voices[voice].pan);
         update_loop_buffer = TRUE;
      }
   }

   /* we need to change the order? */
   if (update_reversed) {
      ds_voices[voice].reversed = reversed;

      update_main_buffer = TRUE;
      update_loop_buffer = TRUE;
   }

   /* we need to update the main buffer? */
   if (update_main_buffer) {
      fill_dsound_buffer(ds_voices[voice].ds_buffer,
                         0,  /* offset */
                         ds_voices[voice].len,
                         ds_voices[voice].bits,
                         ds_voices[voice].stereo,
                         ds_voices[voice].reversed,
                         ds_voices[voice].data);
   }

   /* we need to update the loop buffer? */
   if (update_loop_buffer && ds_voices[voice].ds_loop_buffer) {
      fill_dsound_buffer(ds_voices[voice].ds_loop_buffer,
                         0,  /* offset */
                         ds_voices[voice].loop_len,
                         ds_voices[voice].bits,
                         ds_voices[voice].stereo,
                         ds_voices[voice].reversed,
                         ds_voices[voice].data + ds_voices[voice].loop_offset * ds_voices[voice].bytes_per_sample);

      if (ds_voices[voice].bidir)
         fill_dsound_buffer(ds_voices[voice].ds_loop_buffer,
                            ds_voices[voice].loop_len,
                            ds_voices[voice].loop_len,
                            ds_voices[voice].bits,
                            ds_voices[voice].stereo,
                            !ds_voices[voice].reversed,
                            ds_voices[voice].data + ds_voices[voice].loop_offset * ds_voices[voice].bytes_per_sample);

      /* rewind the buffer */
      IDirectSoundBuffer_SetCurrentPosition(ds_voices[voice].ds_loop_buffer, 0);
   }
}



/* digi_directsound_loop_voice:
 */
static void digi_directsound_loop_voice(int voice, int playmode)
{
   int reversed = (playmode & PLAYMODE_BACKWARD ? TRUE : FALSE);
   int bidir = (playmode & PLAYMODE_BIDIR ? TRUE : FALSE);
   int loop = (playmode & PLAYMODE_LOOP ? TRUE : FALSE);

   /* update the voice buffers */
   update_voice_buffers(voice, reversed, bidir, loop);
}



/* digi_directsound_lock_voice:
 */
static void *digi_directsound_lock_voice(int voice, int start, int end)
{
   LPDIRECTSOUNDBUFFER ds_locked_buffer;
   unsigned long size_a;
   void *buf_a;
   HRESULT hr;

   /* just in case */
   digi_directsound_unlock_voice(voice);

   if (ds_voices[voice].looping && ds_voices[voice].ds_loop_buffer)
      ds_locked_buffer = ds_voices[voice].ds_loop_buffer;
   else
      ds_locked_buffer = ds_voices[voice].ds_buffer;

   start = start * ds_voices[voice].bytes_per_sample;
   end = end * ds_voices[voice].bytes_per_sample;

   hr = IDirectSoundBuffer_Lock(ds_locked_buffer, start, end - start, &buf_a, &size_a, NULL, NULL, 0);

   /* if buffer lost, restore and retry lock */
   if (hr == DSERR_BUFFERLOST) {
      IDirectSoundBuffer_Restore(ds_locked_buffer);

      hr = IDirectSoundBuffer_Lock(ds_locked_buffer, start, end - start, &buf_a, &size_a, NULL, NULL, 0);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "digi_directsound_lock_voice() failed (%s).\n", ds_err(hr));
         return NULL;
      }
   }

   ds_voices[voice].ds_locked_buffer = ds_locked_buffer;
   ds_voices[voice].lock_buf_a = buf_a;
   ds_voices[voice].lock_size_a = size_a;

   return buf_a;
}



/* digi_directsound_unlock_voice:
 */
static void digi_directsound_unlock_voice(int voice)
{
   HRESULT hr;
   int lock_bytes;

   if (ds_voices[voice].ds_locked_buffer && ds_voices[voice].lock_buf_a) {

      if (ds_voices[voice].bits == 16) {
         unsigned short *p = (unsigned short *)ds_voices[voice].lock_buf_a;

         lock_bytes = ds_voices[voice].lock_size_a >> 1;

         while (lock_bytes--)
            *p++ ^= 0x8000;
      }

      hr = IDirectSoundBuffer_Unlock(ds_voices[voice].ds_locked_buffer,
                                     ds_voices[voice].lock_buf_a,
                                     ds_voices[voice].lock_size_a,
                                     NULL,
                                     0);

      if (FAILED(hr)) {
         _TRACE(PREFIX_E "digi_directsound_unlock_voice() failed (%s).\n", ds_err(hr));
      }

      ds_voices[voice].ds_locked_buffer = NULL;
      ds_voices[voice].lock_buf_a = NULL;
      ds_voices[voice].lock_size_a = 0;
   }
}



/* digi_directsound_get_position:
 */
static int digi_directsound_get_position(int voice)
{
   HRESULT hr;
   unsigned long int play_cursor;
   unsigned long int write_cursor;
   unsigned long int status;
   int pos;

   if (ds_voices[voice].looping && ds_voices[voice].ds_loop_buffer) {
      /* is buffer playing? */
      hr = IDirectSoundBuffer_GetStatus(ds_voices[voice].ds_loop_buffer, &status);
      if (FAILED(hr) || !(status & DSBSTATUS_PLAYING))
         return -1;

      hr = IDirectSoundBuffer_GetCurrentPosition(ds_voices[voice].ds_loop_buffer,
                                                 &play_cursor, &write_cursor);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "digi_directsound_get_position() failed (%s).\n", ds_err(hr));
         return -1;
      }

      pos = play_cursor / ds_voices[voice].bytes_per_sample;

      /* handle bidir data */
      if (ds_voices[voice].bidir && (pos >= ds_voices[voice].loop_len))
         pos = (ds_voices[voice].loop_len - 1) - (pos - ds_voices[voice].loop_len);

      /* handle reversed data */
      if (ds_voices[voice].reversed)
         pos = ds_voices[voice].loop_len - 1 - pos;

      return ds_voices[voice].loop_offset + pos;
   }
   else if (ds_voices[voice].ds_buffer) {
      /* is buffer playing? */
      hr = IDirectSoundBuffer_GetStatus(ds_voices[voice].ds_buffer, &status);
      if (FAILED(hr) || !(status & DSBSTATUS_PLAYING))
         return -1;

      hr = IDirectSoundBuffer_GetCurrentPosition(ds_voices[voice].ds_buffer,
                                                 &play_cursor, &write_cursor);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "digi_directsound_get_position() failed (%s).\n", ds_err(hr));
         return -1;
      }

      pos = play_cursor / ds_voices[voice].bytes_per_sample;

      /* handle reversed data */
      if (ds_voices[voice].reversed)
         pos = ds_voices[voice].len - 1 - pos;

      return pos;
   }
   else
      return -1;
}



/* digi_directsound_set_position:
 */
static void digi_directsound_set_position(int voice, int position)
{
   HRESULT hr;
   int pos;

   if (ds_voices[voice].ds_loop_buffer) {
      pos = CLAMP(0, position - ds_voices[voice].loop_offset, ds_voices[voice].loop_len-1);

      /* handle bidir data: todo? */

      /* handle reversed data */
      if (ds_voices[voice].reversed)
         pos = ds_voices[voice].loop_len - 1 - pos;

      hr = IDirectSoundBuffer_SetCurrentPosition(ds_voices[voice].ds_loop_buffer,
                                                 pos * ds_voices[voice].bytes_per_sample);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "digi_directsound_set_position() failed (%s).\n", ds_err(hr));
      }
   }

   if (ds_voices[voice].ds_buffer) {
      pos = position;

      /* handle reversed data */
      if (ds_voices[voice].reversed)
         pos = ds_voices[voice].len - 1 - pos;

      hr = IDirectSoundBuffer_SetCurrentPosition(ds_voices[voice].ds_buffer,
                                                 pos * ds_voices[voice].bytes_per_sample);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "digi_directsound_set_position() failed (%s).\n", ds_err(hr));
      }
   }
}



/* digi_directsound_get_volume:
 */
static int digi_directsound_get_volume(int voice)
{
   return ds_voices[voice].vol;
}



/* digi_directsound_set_volume:
 */
static void digi_directsound_set_volume(int voice, int volume)
{
   int ds_vol;

   ds_voices[voice].vol = volume;

   if (ds_voices[voice].ds_buffer) {
      ds_vol = alleg_to_dsound_volume[CLAMP(0, volume, 255)];
      IDirectSoundBuffer_SetVolume(ds_voices[voice].ds_buffer, ds_vol);

      if (ds_voices[voice].ds_loop_buffer)
         IDirectSoundBuffer_SetVolume(ds_voices[voice].ds_loop_buffer, ds_vol);
   }
}



/* digi_directsound_get_frequency:
 */
static int digi_directsound_get_frequency(int voice)
{
   return ds_voices[voice].freq;
}



/* digi_directsound_set_frequency:
 */
static void digi_directsound_set_frequency(int voice, int frequency)
{
   ds_voices[voice].freq = frequency;

   if (ds_voices[voice].ds_buffer) {
      IDirectSoundBuffer_SetFrequency(ds_voices[voice].ds_buffer, frequency);

      if (ds_voices[voice].ds_loop_buffer)
         IDirectSoundBuffer_SetFrequency(ds_voices[voice].ds_loop_buffer, frequency);
   }
}



/* digi_directsound_get_pan:
 */
static int digi_directsound_get_pan(int voice)
{
   return ds_voices[voice].pan;
}



/* digi_directsound_set_pan:
 */
static void digi_directsound_set_pan(int voice, int pan)
{
   int ds_pan;

   ds_voices[voice].pan = pan;

   if (ds_voices[voice].ds_buffer) {
      ds_pan = alleg_to_dsound_pan[CLAMP(0, pan, 255)];
      IDirectSoundBuffer_SetPan(ds_voices[voice].ds_buffer, ds_pan);

      if (ds_voices[voice].ds_loop_buffer)
         IDirectSoundBuffer_SetPan(ds_voices[voice].ds_loop_buffer, ds_pan);
   }
}

