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
 *      Linux console internal mouse driver for PS/2.
 *
 *      By George Foot.
 *
 *      See readme.txt for copyright information.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/time.h>

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "linalleg.h"


static int intellimouse;
static int packet_size;


/* processor:
 *  Processes the first packet in the buffer, if any, returning the number
 *  of bytes eaten.
 *
 *  PS/2 mouse protocol is pretty simple; packets consist of three bytes,
 *  one header byte and two data bytes.  In the header, the bottom three
 *  bits show the current button state.  The next bit should always be
 *  set, but apparently some mice don't do this.  The next two bits are 
 *  the X and Y sign bits, and the last two are the X and Y overflow 
 *  flags.  The two data bytes are the X and Y deltas; prepend the 
 *  corresponding sign bit to get a nine-bit two's complement number.
 *
 *  Finding the header byte can be tricky in you lose sync; bit 3 isn't
 *  that useful, not only because some mice don't set it, but also 
 *  because a data byte could have it set.  The overflow bits are very
 *  rarely set, so we just test for these being zero.  Of course the 
 *  data bytes can have zeros here too, but as soon as you move the 
 *  mouse down or left the data bytes become negative and have ones 
 *  here instead.
 */
static int processor (unsigned char *buf, int buf_size)
{
   int r, l, m, x, y, z;

   if (buf_size < packet_size) return 0;  /* not enough data, spit it out for now */

   /* if data is invalid, return no motion and all buttons released */
   if (intellimouse) {
      if ((buf[0] & 0xc8) != 0x08) return 1; /* invalid byte, eat it */
   }
   else {
      if ((buf[0] & 0xc0) != 0x00) return 1; /* invalid byte, eat it */
   }

   /* data is valid, process it */
   l = !!(buf[0] & 1);
   r = !!(buf[0] & 2);
   m = !!(buf[0] & 4);

   x = buf[1];
   y = buf[2];
   if (buf[0] & 0x10) x -= 256;
   if (buf[0] & 0x20) y -= 256;

   if (intellimouse) {
      z = buf[3] & 0xf;
      if (z) 
	 z = (z-7) >> 3;
   }
   else
      z = 0;

   __al_linux_mouse_handler(x, y, z, l+(r<<1)+(m<<2));
   return packet_size; /* yum */
}

/* analyse_data:
 *  Analyses the given data, returning 0 if it is unparsable, nonzero
 *  if there's a reasonable chance that this driver can work with that
 *  data.
 */
static int analyse_data (AL_CONST char *buffer, int size)
{
	int pos = 0;
	int packets = 0, errors = 0;

	int step = 0;

	for (pos = 0; pos < size; pos++)
		switch (step) {
			case 3:
				packets++;
				step = 0;
			case 0:
				if (buffer[pos] & 0xC0) {
					errors++;
				} else {
					step++;
				}
				break;

			case 1:
			case 2:
				step++;
				break;
		}

	return (errors <= 5) || (errors < size / 20);    /* 5% error allowance */
}


static INTERNAL_MOUSE_DRIVER intdrv = {
   -1,
   processor,
   3
};

/* sync_mouse:
 *  To find the start of a packet, we just read all the data that's 
 *  waiting.  This isn't a particularly good way, obviously. :)
 */
static void sync_mouse (int fd)
{
	fd_set set;
	int result;
	struct timeval tv;
	char bitbucket;

	do {
		FD_ZERO (&set);
		FD_SET (fd, &set);
		tv.tv_sec = tv.tv_usec = 0;
		result = select (FD_SETSIZE, &set, NULL, NULL, &tv);
		if (result > 0) read (fd, &bitbucket, 1);
	} while (result > 0);
}

/* wakeup_im:
 *  Intellimouse needs some special initialisation.
 */
static void wakeup_im (int fd)
{
	unsigned char init[] = { 243, 200, 243, 100, 243, 80 };
	int ret;
	do {
		ret = write (fd, init, sizeof (init));
		if ((ret < 0) && (errno != EINTR))
			break;
	} while (ret < (int)sizeof (init));
}

/* mouse_init:
 *  Here we open the mouse device, initialise anything that needs it, 
 *  and chain to the framework init routine.
 */
static int mouse_init (void)
{
	char tmp1[128], tmp2[128];
	AL_CONST char *udevice;
	int flags;

        static AL_CONST char * AL_CONST default_devices[] = {
		"/dev/mouse",
		"/dev/input/mice",
        };

	/* Find the device filename */
	udevice = get_config_string (uconvert_ascii ("mouse", tmp1),
				     uconvert_ascii ("mouse_device", tmp2),
				     NULL);

	/* Open mouse device.  Devices are cool. */
        flags = O_NONBLOCK | (intellimouse ? O_RDWR : O_RDONLY);
        if (udevice) {
		intdrv.device = open (uconvert_toascii (udevice, tmp1), flags);
        }
        else {
		size_t n;
		for (n = 0; n < sizeof(default_devices) / sizeof(default_devices[0]); n++) {
			intdrv.device = open (default_devices[n], flags);
			if (intdrv.device >= 0)
				break;
		}
        }

	if (intdrv.device < 0) {
		uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to open %s: %s"),
			   udevice ? udevice : get_config_text("one of the default mice devices"),
			   ustrerror (errno));
		return -1;
	}

	/* Put Intellimouse into wheel mode */
	if (intellimouse)
		wakeup_im (intdrv.device);

	/* Discard any garbage, so the next thing we read is a packet header */
	sync_mouse (intdrv.device);

	return __al_linux_mouse_init (&intdrv);
}

/* ps2_mouse_init:
 *  Plain PS/2 mouse init.
 */
static int ps2_mouse_init (void)
{
	intellimouse = FALSE;
	packet_size = 3;
    	return mouse_init ();
}

/* ps2i_mouse_init:
 *  PS/2 Intellimouse init.
 */
static int ips2_mouse_init (void)
{
	intellimouse = TRUE;
    	packet_size = 4;
    	return mouse_init ();
}

/* mouse_exit:
 *  Chain to the framework, then uninitialise things.
 */
static void mouse_exit (void)
{
	__al_linux_mouse_exit();
	close (intdrv.device);
}

MOUSE_DRIVER mousedrv_linux_ps2 =
{
	MOUSEDRV_LINUX_PS2,
	empty_string,
	empty_string,
	"Linux PS/2 mouse",
	ps2_mouse_init,
	mouse_exit,
	NULL, /* poll() */
	NULL, /* timer_poll() */
	__al_linux_mouse_position,
	__al_linux_mouse_set_range,
	__al_linux_mouse_set_speed,
	__al_linux_mouse_get_mickeys,
	analyse_data,
	NULL, /* enable_hardware_cursor */
	NULL
};

MOUSE_DRIVER mousedrv_linux_ips2 =
{
	MOUSEDRV_LINUX_IPS2,
	empty_string,
	empty_string,
	"Linux PS/2 Intellimouse",
	ips2_mouse_init,
	mouse_exit,
	NULL, /* poll() */
	NULL, /* timer_poll() */
	__al_linux_mouse_position,
	__al_linux_mouse_set_range,
	__al_linux_mouse_set_speed,
	__al_linux_mouse_get_mickeys,
	analyse_data,
	NULL, /* enable_hardware_cursor */
	NULL
};
