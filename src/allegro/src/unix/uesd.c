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
 *      ESD sound driver.
 *
 *      By Michael Bukin.
 *
 *      Bug fixes by Peter Wang and Eduard Bloch.
 *
 *      See readme.txt for copyright information.
 */


#include "allegro.h"

#if (defined ALLEGRO_WITH_ESDDIGI) && ((!defined ALLEGRO_WITH_MODULES) || (defined ALLEGRO_MODULE))

#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"

#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <esd.h>

static int _al_esd_fd;
static int _al_esd_bufsize;
static unsigned char *_al_esd_bufdata;
static int _al_esd_bits, _al_esd_signed, _al_esd_rate, _al_esd_stereo;
static esd_format_t _al_esd_format;

static int _al_esd_detect(int input);
static int _al_esd_init(int input, int voices);
static void _al_esd_exit(int input);
static int _al_esd_set_mixer_volume(int volume);
static int _al_esd_buffer_size(void);

static char _al_esd_desc[256] = EMPTY_STRING;

DIGI_DRIVER digi_esd =
{
   DIGI_ESD,
   empty_string,
   empty_string,
   "Enlightened Sound Daemon",
   0,
   0,
   MIXER_MAX_SFX,
   MIXER_DEF_SFX,

   _al_esd_detect,
   _al_esd_init,
   _al_esd_exit,
   _al_esd_set_mixer_volume,
   NULL,

   NULL,
   NULL,
   _al_esd_buffer_size,
   _mixer_init_voice,
   _mixer_release_voice,
   _mixer_start_voice,
   _mixer_stop_voice,
   _mixer_loop_voice,

   _mixer_get_position,
   _mixer_set_position,

   _mixer_get_volume,
   _mixer_set_volume,
   _mixer_ramp_volume,
   _mixer_stop_volume_ramp,

   _mixer_get_frequency,
   _mixer_set_frequency,
   _mixer_sweep_frequency,
   _mixer_stop_frequency_sweep,

   _mixer_get_pan,
   _mixer_set_pan,
   _mixer_sweep_pan,
   _mixer_stop_pan_sweep,

   _mixer_set_echo,
   _mixer_set_tremolo,
   _mixer_set_vibrato,
   0, 0,
   0,
   0,
   0,
   0,
   0,
   0
};



/* _al_esd_buffer_size:
 *  Returns the current DMA buffer size, for use by the audiostream code.
 */
static int _al_esd_buffer_size(void)
{
   return _al_esd_bufsize / (_al_esd_bits / 8) / (_al_esd_stereo ? 2 : 1);
}



/* _al_esd_update:
 *  Update data.
 */
static void _al_esd_update(int threaded)
{
   fd_set wfds;
   struct timeval timeout;

   FD_ZERO(&wfds);
   FD_SET(_al_esd_fd, &wfds);
   timeout.tv_sec = 0;
   timeout.tv_usec = 0;

   if (select(_al_esd_fd+1, NULL, &wfds, NULL, &timeout) > 0) {
      write(_al_esd_fd, _al_esd_bufdata, _al_esd_bufsize);
      _mix_some_samples((uintptr_t) _al_esd_bufdata, 0, _al_esd_signed);
   }
}



/* _al_esd_detect:
 *  Detect driver presence.
 */
static int _al_esd_detect(int input)
{
   int fd;
   AL_CONST char *server;
   char tmp1[128], tmp2[128], tmp3[16];
   char s[256];
	 
   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return FALSE;
   }

   /* We don't want esdlib to spawn ESD while we are detecting it.  */
   putenv("ESD_NO_SPAWN=1");

   /* Get ESD server name.  */
   server = get_config_string(uconvert_ascii("sound", tmp1),
			      uconvert_ascii("esd_server", tmp2),
			      uconvert_ascii("", tmp3));

   /* Try to open ESD server.  */
   fd = esd_open_sound(uconvert_toascii(server, s));
   if (fd < 0) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s: can not open"),
		(ugetc(server) ? server : get_config_text("No server")));
      return FALSE;
   }

   esd_close(fd);
   return TRUE;
}



/* _al_esd_init:
 *  ESD init routine.
 */
static int _al_esd_init(int input, int voices)
{
   AL_CONST char *server;
   char tmp1[128], tmp2[128], tmp3[16];
   char s[256];

   if (input) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Input is not supported"));
      return -1;
   }

   server = get_config_string(uconvert_ascii("sound", tmp1),
			      uconvert_ascii("esd_server", tmp2),
			      uconvert_ascii("", tmp3));

   _al_esd_bits = (_sound_bits == 8) ? 8 : 16;
   _al_esd_stereo = (_sound_stereo) ? 1 : 0;
   _al_esd_rate = (_sound_freq > 0) ? _sound_freq : ESD_DEFAULT_RATE;
   _al_esd_signed = 1;

   _al_esd_format = (((_al_esd_bits == 16) ? ESD_BITS16 : ESD_BITS8)
		     | (_al_esd_stereo ? ESD_STEREO : ESD_MONO)
		     | ESD_STREAM | ESD_PLAY);

   _al_esd_fd = esd_play_stream_fallback(_al_esd_format, _al_esd_rate,
					 uconvert_toascii(server, s), NULL);
   if (_al_esd_fd < 0) {
      uszprintf(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("%s: can not open"),
		(ugetc(server) ? server : get_config_text("No server")));
      return -1;
   }

   _al_esd_bufsize = ESD_BUF_SIZE;
   _al_esd_bufdata = _AL_MALLOC_ATOMIC(_al_esd_bufsize);
   if (_al_esd_bufdata == 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not allocate audio buffer"));
      close(_al_esd_fd);
      return -1;
   }

   digi_esd.voices = voices;

   if (_mixer_init(_al_esd_bufsize / (_al_esd_bits / 8), _al_esd_rate,
		   _al_esd_stereo, ((_al_esd_bits == 16) ? 1 : 0),
		   &digi_esd.voices) != 0) {
      ustrzcpy(allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Can not init software mixer"));
      _AL_FREE(_al_esd_bufdata);
      _al_esd_bufdata = 0;
      close(_al_esd_fd);
      return -1;
   }

   _mix_some_samples((uintptr_t) _al_esd_bufdata, 0, _al_esd_signed);

   /* Add audio interrupt.  */
   _unix_bg_man->register_func(_al_esd_update);

   uszprintf(_al_esd_desc, sizeof(_al_esd_desc), get_config_text("%s: %d bits, %s, %d bps, %s"),
			  server, _al_esd_bits,
			  uconvert_ascii((_al_esd_signed ? "signed" : "unsigned"), tmp1), _al_esd_rate,
			  uconvert_ascii((_al_esd_stereo ? "stereo" : "mono"), tmp2));

   digi_driver->desc = _al_esd_desc;

   return 0;
}



/* _al_esd_exit:
 *  Shutdown ESD driver.
 */
static void _al_esd_exit(int input)
{
   if (input) {
      return;
   }

   _unix_bg_man->unregister_func(_al_esd_update);

   _AL_FREE(_al_esd_bufdata);
   _al_esd_bufdata = 0;

   _mixer_exit();

   close(_al_esd_fd);
}



/* _al_esd_set_mixer_volume:
 *  Set mixer volume.
 */
static int _al_esd_set_mixer_volume(int volume)
{
   return 0;
}



#ifdef ALLEGRO_MODULE

/* _module_init:
 *  Called when loaded as a dynamically linked module.
 */
void _module_init(int system_driver)
{
   _unix_register_digi_driver(DIGI_ESD, &digi_esd, TRUE, TRUE);
}

#endif

#endif

