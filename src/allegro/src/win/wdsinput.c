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
 *      DirectSound input driver.
 *
 *      By Nick Kochakian.
 *
 *      API compliance improvements, enhanced format detection
 *      and bugfixes by Javier Gonzalez.
 *
 *      See readme.txt for copyright information.
 */


#define DIRECTSOUND_VERSION 0x0500

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

#define PREFIX_I                "al-dsinput INFO: "
#define PREFIX_W                "al-dsinput WARNING: "
#define PREFIX_E                "al-dsinput ERROR: "


/* sound driver globals */
static LPDIRECTSOUNDCAPTURE ds_capture = NULL;
static LPDIRECTSOUNDCAPTUREBUFFER ds_capture_buf = NULL;
static WAVEFORMATEX dsc_buf_wfx;
static unsigned long int ds_capture_buffer_size;
static unsigned long int last_capture_pos, input_wave_bytes_read;
static unsigned char *input_wave_data = NULL;



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



/* create_test_capture_buffer:
 *  Helper function that tries to create a capture buffer with
 *  the specified format and deletes it immediatly.
 */
static int create_test_capture_buffer(WAVEFORMATEX *wfx)
{
   LPDIRECTSOUNDCAPTUREBUFFER dsc_trybuf;
   DSCBUFFERDESC dsc_trybuf_desc;
   HRESULT hr;

   /* create the capture buffer */
   ZeroMemory(&dsc_trybuf_desc, sizeof(DSCBUFFERDESC));
   dsc_trybuf_desc.dwSize  = sizeof(DSCBUFFERDESC);
   dsc_trybuf_desc.dwFlags = 0;
   dsc_trybuf_desc.dwBufferBytes = 1024;
   dsc_trybuf_desc.dwReserved  = 0;
   dsc_trybuf_desc.lpwfxFormat = wfx;

   hr = IDirectSoundCapture_CreateCaptureBuffer(ds_capture, &dsc_trybuf_desc, &dsc_trybuf, NULL);
   
   if (FAILED(hr))
      return -1;

   IDirectSoundCaptureBuffer_Release(dsc_trybuf);
   return 0;
}



/* get_capture_format_support:
 *  Helper function to see if the specified input device
 *  can support a combination of capture settings.
 */
static int get_capture_format_support(int bits, int stereo, int rate,
                                      int autodetect, WAVEFORMATEX *wfx)
{
   int i;
   DSCCAPS dsCaps;
   HRESULT hr;
   WAVEFORMATEX *test_wfx;

   struct {
      unsigned long int type;
      unsigned long int freq;
      unsigned char bits;
      unsigned char channels;
      BOOL stereo;
   } ds_formats[] = {
      { WAVE_FORMAT_4S16,   44100, 16, 2, TRUE  },
      { WAVE_FORMAT_2S16,   22050, 16, 2, TRUE  },
      { WAVE_FORMAT_1S16,   11025, 16, 2, TRUE  },

      { WAVE_FORMAT_4M16,   44100, 16, 1, FALSE },
      { WAVE_FORMAT_2M16,   22050, 16, 1, FALSE },
      { WAVE_FORMAT_1M16,   11025, 16, 1, FALSE },

      { WAVE_FORMAT_4S08,   44100, 8,  2, TRUE  },
      { WAVE_FORMAT_2S08,   22050, 8,  2, TRUE  },
      { WAVE_FORMAT_1S08,   11025, 8,  2, TRUE  },

      { WAVE_FORMAT_4M08,   44100, 8,  1, FALSE },
      { WAVE_FORMAT_2M08,   22050, 8,  1, FALSE },
      { WAVE_FORMAT_1M08,   11025, 8,  1, FALSE },

      { WAVE_INVALIDFORMAT,     0, 0,  0, FALSE }
   };

   if (!ds_capture)
     return -1;

   /* if we have already a capture buffer working
    * we return its format as the only valid one
    */
   if (ds_capture_buf) {
      if (!autodetect) {
         /* we must check that everything that cares is
          * the same as in the capture buffer settings
          */
         if (((bits > 0) && (dsc_buf_wfx.wBitsPerSample != bits)) ||
             ( stereo && (dsc_buf_wfx.nChannels != 2)) ||
             (!stereo && (dsc_buf_wfx.nChannels != 1)) ||
             ((rate > 0) && (dsc_buf_wfx.nSamplesPerSec != (unsigned int) rate)))
            return -1;
      }

      /* return the actual capture buffer settings */
      if (wfx)
         memcpy(wfx, &dsc_buf_wfx, sizeof(WAVEFORMATEX));

      return 0;
   }

   /* we use a two-level checking process:
    *  - the normal check of exposed capabilities,
    *  - the actual creation of the capture buffer,
    * because of the single frequency limitation on some
    * sound cards (e.g SB16 ISA) in full duplex mode.
    */
   dsCaps.dwSize = sizeof(DSCCAPS);
   hr = IDirectSoundCapture_GetCaps(ds_capture, &dsCaps);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't get input device caps (%s).\n", ds_err(hr));
      return -1;
   }

   if (wfx)
      test_wfx = wfx;
   else
      test_wfx = _AL_MALLOC(sizeof(WAVEFORMATEX));

   for (i=0; ds_formats[i].type != WAVE_INVALIDFORMAT; i++)
      /* if the current format is supported */
      if (dsCaps.dwFormats & ds_formats[i].type) {
         if (!autodetect) {
            /* we must check that everything that cares is
             * the same as in the capture buffer settings
             */
            if (((bits > 0) && (ds_formats[i].bits != bits)) ||
                (stereo && !ds_formats[i].stereo) ||
                (!stereo && ds_formats[i].stereo) ||
                ((rate > 0) && (ds_formats[i].freq != (unsigned int) rate)))
               continue;  /* go to next format */
         }

         test_wfx->wFormatTag = WAVE_FORMAT_PCM;
         test_wfx->nChannels = ds_formats[i].channels;
         test_wfx->nSamplesPerSec = ds_formats[i].freq;
         test_wfx->wBitsPerSample = ds_formats[i].bits;
         test_wfx->nBlockAlign = test_wfx->nChannels * (test_wfx->wBitsPerSample / 8);
         test_wfx->nAvgBytesPerSec = test_wfx->nSamplesPerSec * test_wfx->nBlockAlign;
         test_wfx->cbSize = 0;

         if (create_test_capture_buffer(test_wfx) == 0) {
            if (!wfx)
               _AL_FREE(test_wfx);
            return 0;
         }
      }

   if (!wfx)
      _AL_FREE(test_wfx);

   _TRACE(PREFIX_W "No valid recording formats found.\n");
   return -1;
}



/* digi_directsound_capture_init:
 */
int digi_directsound_capture_init(LPGUID guid)
{
   DSCCAPS dsCaps;
   WAVEFORMATEX wfx;
   HRESULT hr;
   LPVOID temp;

   /* the DirectSoundCapture interface is not part of DirectX 3 */
   if (_dx_ver < 0x0500)
      return -1;  

   /* create the device:
    *  we use CoCreateInstance() instead of DirectSoundCaptureCreate() to avoid
    *  the dll loader blocking the start of Allegro under DirectX 3.
    */

   hr = CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IDirectSoundCapture, &temp);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't create DirectSoundCapture interface (%s).\n", ds_err(hr));
      goto Error;
   }

   ds_capture = temp;

   /* initialize the device */
   hr = IDirectSoundCapture_Initialize(ds_capture, guid);

   if (FAILED(hr)) {
      hr = IDirectSoundCapture_Initialize(ds_capture, NULL);
      if (FAILED(hr)) {
         _TRACE(PREFIX_E "Can't initialize DirectSoundCapture interface (%s).\n", ds_err(hr));
         goto Error;
      }
   }

   /* get the device caps */
   dsCaps.dwSize = sizeof(DSCCAPS);
   hr = IDirectSoundCapture_GetCaps(ds_capture, &dsCaps);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't get input device caps (%s).\n", ds_err(hr));
      goto Error;
   }

   /* cool little 'autodetection' process :) */
   if (get_capture_format_support(0, FALSE, 0, TRUE, &wfx) != 0) {
      _TRACE(PREFIX_E "The DirectSound hardware doesn't support any capture types.\n");
      goto Error;
   }

   /* set capabilities */
   digi_input_driver->rec_cap_bits = wfx.wBitsPerSample;
   digi_input_driver->rec_cap_stereo = (wfx.nChannels == 2) ? 1 : 0;

   return 0;

 Error:
   /* shutdown DirectSoundCapture */
   digi_directsound_capture_exit();

   return -1;
}



/* digi_directsound_capture_exit:
 */
void digi_directsound_capture_exit(void)
{
   /* destroy capture buffer */
   digi_directsound_rec_stop();

   /* shutdown DirectSoundCapture */
   if (ds_capture) {
      IDirectSoundCapture_Release(ds_capture);
      ds_capture = NULL;
   }
}



/* digi_directsound_capture_detect:
 */
int digi_directsound_capture_detect(LPGUID guid)
{
   HRESULT hr;
   LPVOID temp;

   /* the DirectSoundCapture interface is not part of DirectX 3 */
   if (_dx_ver < 0x500)
      return 0;  

   if (!ds_capture) {
      /* create the device:
       *  we use CoCreateInstance() instead of DirectSoundCaptureCreate() to avoid
       *  the dll loader blocking the start of Allegro under DirectX 3.
       */
      hr = CoCreateInstance(&CLSID_DirectSoundCapture, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IDirectSoundCapture, &temp);

      if (FAILED(hr)) {
         _TRACE(PREFIX_E "DirectSoundCapture interface creation failed during detect (%s).\n", ds_err(hr));
         return 0;
      }

      ds_capture = temp;

      /* initialize the device */
      hr = IDirectSoundCapture_Initialize(ds_capture, guid);

      if (FAILED(hr)) {
         hr = IDirectSoundCapture_Initialize(ds_capture, NULL);
         if (FAILED(hr)) {
            _TRACE(PREFIX_E "DirectSoundCapture interface initialization failed during detect (%s).\n", ds_err(hr));
            return 0;
         }
      }

      _TRACE(PREFIX_I "DirectSoundCapture interface successfully created.\n");

      /* release DirectSoundCapture interface */
      IDirectSoundCapture_Release(ds_capture);
      ds_capture = NULL;
   }

   return 1;
}



/* digi_directsound_rec_cap_rate:
 *  Gets the maximum input frequency for the specified parameters.
 */
int digi_directsound_rec_cap_rate(int bits, int stereo)
{
   WAVEFORMATEX wfx;

   if (get_capture_format_support(bits, stereo, 0, FALSE, &wfx) != 0)
      return 0;

   return wfx.nSamplesPerSec;
}



/* digi_directsound_rec_cap_param:
 *  Determines if the combination of provided parameters can be
 *  used for recording.
 */
int digi_directsound_rec_cap_param(int rate, int bits, int stereo)
{
   if (get_capture_format_support(bits, stereo, rate, FALSE, NULL) == 0)
      return 2;

   if (get_capture_format_support(bits, stereo, 44100, FALSE, NULL) == 0)
      return -44100;

   if (get_capture_format_support(bits, stereo, 22050, FALSE, NULL) == 0)
      return -22050;

   if (get_capture_format_support(bits, stereo, 11025, FALSE, NULL) == 0)
      return -11025;

   return 0;
}



/* digi_directsound_rec_source:
 *  Sets the source for the audio recording.
 */
int digi_directsound_rec_source(int source)
{
   /* since DirectSoundCapture doesn't allow us to
    * select a input source manually, we return -1
    */
   return -1;
}



/* digi_directsound_rec_start:
 *  Start recording with the specified parameters.
 */
int digi_directsound_rec_start(int rate, int bits, int stereo)
{
   DSCBUFFERDESC dscBufDesc;
   HRESULT hr;

   if (!ds_capture || ds_capture_buf)
      return 0;

   /* check if we support the desired format */
   if ((bits <= 0) || (rate <= 0))
      return 0;

   if (get_capture_format_support(bits, stereo, rate, FALSE, &dsc_buf_wfx) != 0)
      return 0;

   digi_driver->rec_cap_bits = dsc_buf_wfx.wBitsPerSample;
   digi_driver->rec_cap_stereo = (dsc_buf_wfx.nChannels == 2) ? 1 : 0;

   /* create the capture buffer */
   ZeroMemory(&dscBufDesc, sizeof(DSCBUFFERDESC));
   dscBufDesc.dwSize  = sizeof(DSCBUFFERDESC);
   dscBufDesc.dwFlags = 0;
   dscBufDesc.dwBufferBytes = dsc_buf_wfx.nAvgBytesPerSec;
   dscBufDesc.dwReserved  = 0;
   dscBufDesc.lpwfxFormat = &dsc_buf_wfx;
   ds_capture_buffer_size = dscBufDesc.dwBufferBytes;

   hr = IDirectSoundCapture_CreateCaptureBuffer(ds_capture, &dscBufDesc, &ds_capture_buf, NULL);
   if (FAILED(hr)) {
      _TRACE(PREFIX_E "Can't create the DirectSound capture buffer (%s).\n", ds_err(hr));
      return 0;
   }

   hr = IDirectSoundCaptureBuffer_Start(ds_capture_buf, DSCBSTART_LOOPING);
   if (FAILED(hr)) {
      IDirectSoundCaptureBuffer_Release(ds_capture_buf);
      ds_capture_buf = NULL;

      _TRACE(PREFIX_E "Can't start the DirectSound capture buffer (%s).\n", ds_err(hr));
      return 0;
   }

   last_capture_pos = 0;
   input_wave_data = _AL_MALLOC(ds_capture_buffer_size);
   input_wave_bytes_read = 0;

   return ds_capture_buffer_size;
}



/* digi_directsound_rec_stop:
 *  Stops recording.
 */
void digi_directsound_rec_stop(void)
{
   if (ds_capture_buf) {
      IDirectSoundCaptureBuffer_Stop(ds_capture_buf);
      IDirectSoundCaptureBuffer_Release(ds_capture_buf);
      ds_capture_buf = NULL;
   }

   if (input_wave_data) {
      _AL_FREE(input_wave_data);
      input_wave_data = NULL;
   }
}



/* digi_directsound_rec_read:
 *  Reads the input buffer.
 */
int digi_directsound_rec_read(void *buf)
{
   unsigned char *input_ptr1, *input_ptr2, *linear_input_ptr;
   unsigned long int input_bytes1, input_bytes2, bytes_to_lock;
   unsigned long int capture_pos;
   HRESULT hr;
   BOOL buffer_filled = FALSE;
   LPVOID temp1, temp2;

   if (!ds_capture || !ds_capture_buf || !input_wave_data)
      return 0;

   IDirectSoundCaptureBuffer_GetCurrentPosition(ds_capture_buf, &capture_pos, NULL);

   /* check if we are not still in the same capture position */
   if (last_capture_pos == capture_pos)
      return 0;

   /* check how many bytes we need to lock since last check */
   if (capture_pos > last_capture_pos) {
      bytes_to_lock = capture_pos - last_capture_pos;
   }
   else {
      bytes_to_lock = ds_capture_buffer_size - last_capture_pos;
      bytes_to_lock += capture_pos;
   }

   hr = IDirectSoundCaptureBuffer_Lock(ds_capture_buf, last_capture_pos,
                                       bytes_to_lock, &temp1,
                                       &input_bytes1, &temp2,
                                       &input_bytes2, 0);
   if (FAILED(hr))
      return 0;

   input_ptr1 = temp1;
   input_ptr2 = temp2;

   /* let's get the data aligned linearly */
   linear_input_ptr = _AL_MALLOC(bytes_to_lock);
   memcpy(linear_input_ptr, input_ptr1, input_bytes1);

   if (input_ptr2)
      memcpy(linear_input_ptr + input_bytes1, input_ptr2, input_bytes2);

   IDirectSoundCaptureBuffer_Unlock(ds_capture_buf, input_ptr1,
                                    input_bytes1, input_ptr2, input_bytes2);

   if ((input_wave_bytes_read + bytes_to_lock) >= ds_capture_buffer_size) {
      /* we fill the buffer */
      long int bytes_left_to_fill = ds_capture_buffer_size - input_wave_bytes_read;
      long int bytes_to_internal = bytes_to_lock - bytes_left_to_fill;

      /* copy old buffer to user buffer */
      memcpy((char*)buf, input_wave_data, input_wave_bytes_read);

      /* and the rest of bytes we would need to fill in the buffer */
      memcpy((char*)buf + input_wave_bytes_read, linear_input_ptr, bytes_left_to_fill);

      /* and the rest of the data to the internal buffer */
      input_wave_bytes_read = bytes_to_internal;
      memcpy(input_wave_data, linear_input_ptr + bytes_left_to_fill, bytes_to_internal);

      buffer_filled = TRUE;
  
      /* if we are using 16-bit data, we need to convert it to unsigned format */
      if (digi_driver->rec_cap_bits == 16) {
         unsigned int i;
         unsigned short *buf16 = (unsigned short *)buf;

         for (i = 0; i < ds_capture_buffer_size/2; i++)
            buf16[i] ^= 0x8000;
      }
   }
   else {
      /* we won't fill the buffer */
      memcpy(input_wave_data + input_wave_bytes_read, linear_input_ptr, bytes_to_lock);
      input_wave_bytes_read += bytes_to_lock;
   }

   _AL_FREE(linear_input_ptr);

   last_capture_pos = capture_pos;

   if (buffer_filled)
      return 1;
   else
      return 0;
}
