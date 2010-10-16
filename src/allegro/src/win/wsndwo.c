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
 *      Allegro mixer to WaveOut driver.
 *
 *      By Robin Burrows.
 *
 *      Based on original src/unix/uoss.c by Joshua Heyer.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintwin.h"

#ifndef SCAN_DEPEND
   #ifdef ALLEGRO_MINGW32
      #undef MAKEFOURCC
   #endif

   #include <mmsystem.h>
   #include <math.h>

   #ifdef ALLEGRO_MSVC
      #include <mmreg.h>
   #endif
#endif

#ifndef ALLEGRO_WINDOWS
#error something is wrong with the makefile
#endif

#define PREFIX_I                "al-wsnd INFO: "
#define PREFIX_W                "al-wsnd WARNING: "
#define PREFIX_E                "al-wsnd ERROR: "


static int digi_waveout_detect(int input);
static int digi_waveout_init(int input, int voices);
static void digi_waveout_exit(int input);
static int digi_waveout_set_mixer_volume(int volume);
static int digi_waveout_get_mixer_volume(void);
static int digi_waveout_buffer_size(void);


/* template driver: will be cloned for each device */
static DIGI_DRIVER digi_waveout =
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
   digi_waveout_detect,
   digi_waveout_init,
   digi_waveout_exit,
   digi_waveout_set_mixer_volume,
   digi_waveout_get_mixer_volume,

   /* audiostream locking functions */
   NULL,  // AL_METHOD(void *, lock_voice, (int voice, int start, int end));
   NULL,  // AL_METHOD(void, unlock_voice, (int voice));
   digi_waveout_buffer_size,

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
   0,     // int rec_cap_bits;
   0,     // int rec_cap_stereo;
   NULL,  // AL_METHOD(int,  rec_cap_rate, (int bits, int stereo));
   NULL,  // AL_METHOD(int,  rec_cap_parm, (int rate, int bits, int stereo));
   NULL,  // AL_METHOD(int,  rec_source, (int source));
   NULL,  // AL_METHOD(int,  rec_start, (int rate, int bits, int stereo));
   NULL,  // AL_METHOD(void, rec_stop, (void));
   NULL   // AL_METHOD(int,  rec_read, (void *buf));
};


/* sound driver globals */
static HWAVEOUT hWaveOut = NULL;
static LPWAVEHDR lpWaveHdr = NULL;
static unsigned long int initial_volume;
static int digiwobufsize, digiwobufdivs, digiwobufpos;
static char * digiwobufdata = NULL;
static int _freq, _bits, _stereo;
static int waveout_paused = FALSE;



/* _get_woalmix_driver:
 *  System driver hook for listing the available sound drivers. This 
 *  generates the device list at runtime, to match whatever WaveOut
 *  devices are available.
 */
DIGI_DRIVER *_get_woalmix_driver(int num)
{
   DIGI_DRIVER *driver;

   driver = _AL_MALLOC(sizeof(DIGI_DRIVER));
   if (!driver)
      return NULL;

   memcpy(driver, &digi_waveout, sizeof(DIGI_DRIVER));

   driver->id = DIGI_WAVOUTID(num);

   if (num == 0)
      driver->ascii_name = "WaveOut 44100hz 16bit stereo";
   else
      driver->ascii_name = "WaveOut 22050hz 8bit mono";

   return driver;
}



/* digi_waveout_mixer_callback:
 *  Callback function to update sound in WaveOut buffer.
 */
static void digi_waveout_mixer_callback(void)
{ 
   MMTIME mmt;
   MMRESULT mmr;
   int writecurs;
   int switch_mode;

   /* handle display switchs */
   switch_mode = get_display_switch_mode();

   if (waveout_paused) {
      if (_win_app_foreground ||
          (switch_mode == SWITCH_BACKGROUND) || (switch_mode == SWITCH_BACKAMNESIA)) {
         waveout_paused = FALSE;
         waveOutRestart(hWaveOut);
      }
      else
         return;
   }
   else {
      if (!_win_app_foreground &&
          ((switch_mode == SWITCH_PAUSE) || (switch_mode == SWITCH_AMNESIA))) {
         waveout_paused = TRUE;
         waveOutPause(hWaveOut);
         return;
      }
   }

   /* get current state of buffer */
   memset(&mmt, 0, sizeof(MMTIME));
   mmt.wType = TIME_BYTES;

   mmr = waveOutGetPosition(hWaveOut, &mmt, sizeof(MMTIME));
   if (mmr != MMSYSERR_NOERROR)
      return;

   writecurs = (int) mmt.u.cb;

   writecurs /= (digiwobufsize/digiwobufdivs);
   writecurs += 8;

   while (writecurs > (digiwobufdivs-1))
      writecurs -= digiwobufdivs;

   /* write data into the buffer */
   while (writecurs != digiwobufpos) {
      if (++digiwobufpos > (digiwobufdivs-1))
         digiwobufpos = 0;

      _mix_some_samples((unsigned long) (digiwobufdata+((digiwobufsize/digiwobufdivs)*digiwobufpos)), 0, TRUE);
   }
}



/* digi_waveout_detect:
 */
static int digi_waveout_detect(int input)
{
   if (input)
      return 0;

   /* always present */
   return 1;
}



/* digi_waveout_init:
 */
static int digi_waveout_init(int input, int voices)
{
   MMRESULT mmr;
   WAVEFORMATEX format;
   int id;

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }

   /* deduce our device number from the driver ID code */
   id = ((digi_driver->id >> 8) & 0xFF) - 'A';

   digi_driver->voices = voices;

   /* get hardware capabilities */
   digiwobufdivs = 32;
   digiwobufpos = 0;
	
   if (id == 0) {
      _bits = 16;
      _freq = 44100;
      _stereo = 1;
      digiwobufsize = digiwobufdivs * 1024;
   }
   else {
      _bits = 8;
      _freq = 22050;
      _stereo = 0;
      digiwobufsize = digiwobufdivs * 512;
   }

   digiwobufdata = _AL_MALLOC_ATOMIC(digiwobufsize);
   if (!digiwobufdata) {
      _TRACE (PREFIX_E "_AL_MALLOC_ATOMIC() failed\n");
      goto Error;
   }

   format.wFormatTag = WAVE_FORMAT_PCM;
   format.nChannels = _stereo ? 2 : 1;
   format.nSamplesPerSec = _freq;
   format.wBitsPerSample = _bits;
   format.nBlockAlign = (format.wBitsPerSample * format.nChannels) >> 3;
   format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;

   mmr = waveOutOpen(&hWaveOut, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL);
   if (mmr != MMSYSERR_NOERROR) {
      _TRACE (PREFIX_E "Can't open WaveOut\n");
      goto Error;
   }

   lpWaveHdr = _AL_MALLOC(sizeof(WAVEHDR));
   lpWaveHdr->lpData = digiwobufdata;
   lpWaveHdr->dwBufferLength = digiwobufsize;
   lpWaveHdr->dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
   lpWaveHdr->dwLoops = 0x7fffffffL;

   mmr = waveOutPrepareHeader(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
   if (mmr != MMSYSERR_NOERROR) {
      _TRACE (PREFIX_E "waveOutPrepareHeader() failed\n");
      goto Error;
   } 

   mmr = waveOutWrite(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
   if (mmr != MMSYSERR_NOERROR) {
      _TRACE (PREFIX_E "waveOutWrite() failed\n" );
      goto Error;
   }

   if (_mixer_init((digiwobufsize / (_bits /8)) / digiwobufdivs, _freq,
                    _stereo, ((_bits == 16) ? 1 : 0), &digi_driver->voices) != 0) {
      _TRACE(PREFIX_E "Can't init software mixer\n");
      goto Error;
   }

   _mix_some_samples((unsigned long) digiwobufdata, 0, TRUE);

   /* get volume */
   waveOutGetVolume(hWaveOut, &initial_volume);

   /* start playing */
   install_int(digi_waveout_mixer_callback, 20);  /* 50 Hz */

   return 0;

 Error:
   _TRACE(PREFIX_E "digi_waveout_init() failed\n");
   digi_waveout_exit(0);
   return -1;
}



/* digi_waveout_exit:
 */
static void digi_waveout_exit(int input)
{
   /* stop playing */
   remove_int(digi_waveout_mixer_callback);

   if (hWaveOut) {
      waveOutReset(hWaveOut);

      /* restore initial volume */
      waveOutSetVolume(hWaveOut, initial_volume);

      waveOutUnprepareHeader(hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
      waveOutClose(hWaveOut);
      hWaveOut = NULL;
   }

   if (lpWaveHdr) {
      _AL_FREE(lpWaveHdr);
      lpWaveHdr = NULL;
   }

   if (digiwobufdata) {
      _AL_FREE(digiwobufdata);
      digiwobufdata = NULL;
   }
}



/* digi_waveout_set_mixer_volume:
 */
static int digi_waveout_set_mixer_volume(int volume)
{
   DWORD realvol;

   if (hWaveOut) {
      realvol = (DWORD) volume;
      realvol |= realvol<<8;
      realvol |= realvol<<16;
      waveOutSetVolume(hWaveOut, realvol);
   }

   return 0;
}



/* digi_waveout_get_mixer_volume:
 */
static int digi_waveout_get_mixer_volume(void)
{
   DWORD vol;
   
   if (!hWaveOut)
      return -1;

   if (waveOutGetVolume(hWaveOut, &vol) != MMSYSERR_NOERROR)
      return -1;

   vol &= 0xffff;
   return vol / (0xffff / 255);
}



/* digi_waveout_buffer_size:
 */
static int digi_waveout_buffer_size(void)
{
   return digiwobufsize / (_bits / 8) / (_stereo ? 2 : 1);
}
