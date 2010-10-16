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
 *      Linux console internal mouse driver for `gpmdata' repeater.
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


#define ASCII_NAME "GPM repeater"
#define DEVICE_FILENAME "/dev/gpmdata"


/* processor:
 *  Processes the first packet in the buffer, if any, returning the number
 *  of bytes eaten.
 * 
 *  The GPM repeater uses Mouse Systems protocol.  Packets contain five
 *  bytes.  The low bits of the first byte represent the button state.
 *  The following bytes represent motion left, up, right and down 
 *  respectively.  I think.
 * 
 *  We recognise the start of a packet by looking for a byte with the top
 *  bit set and the next four bits not set -- while this can occur in the 
 *  data bytes, it's pretty unlikely.
 */
static int processor (unsigned char *buf, int buf_size)
{
   int r, m, l, x, y, z;

   if (buf_size < 5) return 0;  /* not enough data, spit it out for now */

   if ((buf[0] & 0xf8) != 0x80) return 1; /* invalid byte, eat it */

   /* packet is valid, process the data */
   r = !(buf[0] & 1);
   m = !(buf[0] & 2);
   l = !(buf[0] & 4);
   x = (signed char)buf[1] + (signed char)buf[3];
   y = (signed char)buf[2] + (signed char)buf[4];
   z = 0;
   __al_linux_mouse_handler(x, y, z, l+(r<<1)+(m<<2));
   return 5; /* yum */
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

/* mouse_init:
 *  Here we open the mouse device, initialise anything that needs it, 
 *  and chain to the framework init routine.
 */
static int mouse_init (void)
{
	char tmp1[128], tmp2[128], tmp3[128];
	AL_CONST char *udevice;

	/* Find the device filename */
	udevice = get_config_string (uconvert_ascii ("mouse", tmp1),
				     uconvert_ascii ("mouse_device", tmp2),
				     uconvert_ascii (DEVICE_FILENAME, tmp3));

	/* Open mouse device.  Devices are cool. */
	intdrv.device = open (uconvert_toascii (udevice, tmp1), O_RDONLY | O_NONBLOCK);
	if (intdrv.device < 0) {
		uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to open %s: %s"),
			   udevice, ustrerror (errno));
		return -1;
	}

	/* Discard any garbage, so the next thing we read is a packet header */
	sync_mouse (intdrv.device);

	return __al_linux_mouse_init (&intdrv);
}

/* mouse_exit:
 *  Chain to the framework, then uninitialise things.
 */
static void mouse_exit (void)
{
	__al_linux_mouse_exit();
	close (intdrv.device);
}

MOUSE_DRIVER mousedrv_linux_gpmdata =
{
	MOUSEDRV_LINUX_GPMDATA,
	empty_string,
	empty_string,
	ASCII_NAME,
	mouse_init,
	mouse_exit,
	NULL, /* poll() */
	NULL, /* timer_poll() */
	__al_linux_mouse_position,
	__al_linux_mouse_set_range,
	__al_linux_mouse_set_speed,
	__al_linux_mouse_get_mickeys,
	NULL,  /* analyse_data */
	NULL,
       NULL
};

