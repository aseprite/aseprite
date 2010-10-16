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
 *      Linux console internal mouse driver for Microsoft mouse.
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
#include <termios.h>

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
 *  Microsoft protocol has only seven data bits.  The top bit is set in 
 *  the first byte of the packet, clear in the other two.  The next two 
 *  bits in the header are the button state.  The next two are the top 
 *  bits of the Y offset (second data byte), and the last two are the 
 *  top bits of the X offset (first data byte).
 */
static int processor (unsigned char *buf, int buf_size)
{
   int r, l, m, x, y, z;

   if (buf_size < packet_size) return 0;     /* not enough data, spit it out for now */

   /* if packet is invalid, just eat it */
   if (!(buf[0] & 0x40)) return 1; /* invalid byte */
   if (buf[1] & 0x40) return 1;    /* first data byte is actually a header */
   if (buf[2] & 0x40) return 2;    /* second data byte is actually a header */
   
   /* packet is valid, decode the data */
   l = !!(buf[0] & 0x20);
   r = !!(buf[0] & 0x10);

   if (intellimouse) {
      m = !!(buf[3] & 0x10);
      z = (buf[3] & 0x0f);
      if (z) 
         z = (z-7) >> 3;
   }
   else {
      m = 0;
      z = 0;
   }

   x =  (signed char) (((buf[0] & 0x03) << 6) | (buf[1] & 0x3F));
   y = -(signed char) (((buf[0] & 0x0C) << 4) | (buf[2] & 0x3F));

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
				if (!(buffer[pos] & 0x40)) {
					errors++;
				} else {
					step++;
				}
				break;

			case 1:
			case 2:
				if (buffer[pos] & 0x40) {
					errors++;
					step = 0;
					pos--;
				} else {
					step++;
				}
				break;
		}

	return (errors <= 5) || (errors < size / 20);    /* 5% error allowance */
}


static INTERNAL_MOUSE_DRIVER intdrv = {
   -1,
   processor,
   2
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
	struct termios t;

	/* Find the device filename */
	udevice = get_config_string (uconvert_ascii ("mouse", tmp1),
				     uconvert_ascii ("mouse_device", tmp2),
				     uconvert_ascii ("/dev/mouse", tmp3));

	/* Open mouse device.  Devices are cool. */
	intdrv.device = open (uconvert_toascii (udevice, tmp1), O_RDONLY | O_NONBLOCK);
	if (intdrv.device < 0) {
		uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to open %s: %s"),
			   udevice, ustrerror (errno));
		return -1;
	}

	/* Set parameters */
	tcgetattr (intdrv.device, &t);
	t.c_iflag = IGNBRK | IGNPAR;
	t.c_oflag = t.c_lflag = t.c_line = 0;
	t.c_cflag = CS7 | CREAD | CLOCAL | HUPCL | B1200;
	tcsetattr (intdrv.device, TCSAFLUSH, &t);

	/* Discard any garbage, so the next thing we read is a packet header */
	sync_mouse (intdrv.device);

	return __al_linux_mouse_init (&intdrv);
}

/* ms_mouse_init:
 *  Initialisation for vanilla MS mouse.
 */
static int ms_mouse_init (void)
{
	intellimouse = FALSE;
    	packet_size = 3;
	intdrv.num_buttons = 2;
	return mouse_init ();
}

/* msi_mouse_init:
 *  Initialisation for MS mouse with Intellimouse extension.
 */
static int ims_mouse_init (void)
{
	intellimouse = TRUE;
    	packet_size = 4;
	intdrv.num_buttons = 3;
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

MOUSE_DRIVER mousedrv_linux_ms =
{
	MOUSEDRV_LINUX_MS,
	empty_string,
	empty_string,
	"Linux MS mouse",
	ms_mouse_init,
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

MOUSE_DRIVER mousedrv_linux_ims =
{
	MOUSEDRV_LINUX_IMS,
	empty_string,
	empty_string,
	"Linux MS Intellimouse",
	ims_mouse_init,
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
