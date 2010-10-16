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
 *      Console control functions for Linux Allegro.
 *
 *      Originally by Marek Habersack, mangled by George Foot.
 *
 *      See readme.txt for copyright information.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "allegro.h"
#include "allegro/internal/aintern.h"
#include "allegro/platform/aintunix.h"
#include "linalleg.h"

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <linux/kd.h>
#include <sys/mman.h>
#include <sys/stat.h>


/* Console number and open file descriptor */
int __al_linux_vt = -1;
int __al_linux_console_fd = -1;
int __al_linux_prev_vt = -1;
int __al_linux_got_text_message = FALSE;

/* Startup termios and working copy */
struct termios __al_linux_startup_termio;
struct termios __al_linux_work_termio;


/* get_tty:
 *  Compares the inodes of /dev/ttyn (1 <= n <= 24) with the inode of the
 *  passed file, returning whichever it matches, 0 if none, -1 on error.
 */
static int get_tty (int fd)
{
   char name[16];
   int tty;
   ino_t inode;
   struct stat st;

   if (fstat (fd, &st))
      return -1;

   inode = st.st_ino;
   for (tty = 1; tty <= 24; tty++) {
      snprintf (name, sizeof(name), "/dev/tty%d", tty);
      name[sizeof(name)-1] = 0;
      if (!stat (name, &st) && (inode == st.st_ino))
	 break;
   }

   return (tty <= 24) ? tty : 0;
}


/* init_console:
 *  Initialises this subsystem.
 */
static int init_console(void)
{
   char tmp[256];

   /* Find our tty's VT number */
   __al_linux_vt = get_tty(STDIN_FILENO);

   if (__al_linux_vt < 0) {
      uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Error finding our VT: %s"), ustrerror(errno));
      return 1;
   }

   if (__al_linux_vt != 0) {
      /* Open our current console */
      if ((__al_linux_console_fd = open("/dev/tty", O_RDWR)) < 0) {
	 uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text("Unable to open %s: %s"),
		    uconvert_ascii("/dev/tty", tmp), ustrerror(errno));
	 return 1;
      }
   } else {
      int tty, console_fd, fd, child;
      unsigned short mask;
      char tty_name[16];
      struct vt_stat vts; 

      /* Now we need to find a VT we can use.  It must be readable and
       * writable by us, if we're not setuid root.  VT_OPENQRY itself
       * isn't too useful because it'll only ever come up with one 
       * suggestion, with no guarrantee that we actually have access 
       * to it.
       *
       * At some stage I think this is a candidate for config
       * file overriding, but for now we'll stat the first N consoles
       * to see which ones we can write to (hopefully at least one!),
       * so that we can use that one to do ioctls.  We used to use 
       * /dev/console for that purpose but it looks like it's not 
       * always writable by enough people.
       *
       * Having found and opened a writable device, we query the state
       * of the first sixteen (fifteen really) consoles, and try 
       * opening each unused one in turn.
       */

      if ((console_fd = open ("/dev/console", O_WRONLY)) < 0) {
	 int n;
	 uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, uconvert_ascii("%s /dev/console: %s", tmp),
		    get_config_text("Unable to open"), ustrerror (errno));
	 /* Try some ttys instead... */
	 for (n = 1; n <= 24; n++) {
	     snprintf (tty_name, sizeof(tty_name), "/dev/tty%d", n);
	     tty_name[sizeof(tty_name)-1] = 0;
	     if ((console_fd = open (tty_name, O_WRONLY)) >= 0) break;
	 }
	 if (n > 24) return 1; /* leave the error message about /dev/console */
      }

      /* Get the state of the console -- in particular, the free VT field */
      if (ioctl (console_fd, VT_GETSTATE, &vts)) {
	 uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, uconvert_ascii("VT_GETSTATE: %s", tmp), ustrerror (errno));
	 close (console_fd);
	 return 1;
      }

      __al_linux_prev_vt = vts.v_active;

      /* We attempt to set our euid to 0; if we were run with euid 0 to
       * start with, we'll be able to do this now.  Otherwise, we'll just
       * ignore the error returned since it might not be a problem if the
       * ttys we look at are owned by the user running the program. */
      seteuid(0);

      /* tty0 is not really a console, so start counting at 2. */
      fd = -1;
      for (tty = 1, mask = 2; mask; tty++, mask <<= 1) {
	 if (!(vts.v_state & mask)) {
	    snprintf (tty_name, sizeof(tty_name), "/dev/tty%d", tty);
	    tty_name[sizeof(tty_name)-1] = 0;
	    if ((fd = open (tty_name, O_RDWR)) != -1) {
	       close (fd);
	       break;
	    }
	 }
      }

      seteuid (getuid());

      if (!mask) {
	 ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to find a usable VT"));
	 close (console_fd);
	 return 1;
      }

      /* OK, now fork into the background, detach from the current console,
       * and attach to the new one. */
      child = fork();

      if (child < 0) {
	 /* fork failed */
	 uszprintf (allegro_error, ALLEGRO_ERROR_SIZE, uconvert_ascii ("fork: %s", tmp), ustrerror (errno));
	 close (console_fd);
	 return 1;
      }

      if (child) {
	 /* We're the parent -- write a note to the user saying where the
	  * app went, then quit */
	 fprintf (stderr, "Allegro application is running on VT %d\n", tty);
	 exit (0);
      }

      /* We're the child.  Detach from our controlling terminal, and start
       * a new session. */
      close (console_fd);
      ioctl (0, TIOCNOTTY, 0);
      setsid();

      /* Open the new one again.  It becomes our ctty, because we started a
       * new session above. */
      seteuid(0);
      fd = open (tty_name, O_RDWR);
      seteuid(getuid());

      if (fd == -1) {
	 ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("Unable to reopen new console"));
	 return 1;
      }

      /* Try to switch to it -- should succeed, since it's our ctty */
      ioctl (fd, VT_ACTIVATE, tty);

      __al_linux_vt = tty;
      __al_linux_console_fd = fd;

      /* Check we can reliably wait until we have the display */
      if (__al_linux_wait_for_display()) {
	 close (fd);
	 ustrzcpy (allegro_error, ALLEGRO_ERROR_SIZE, get_config_text ("VT_WAITACTIVE failure"));
	 return 1;
      }

      /* dup2 it to stdin, stdout and stderr if necessary */
      if (isatty(0)) dup2 (fd, 0);
      if (isatty(1)) dup2 (fd, 1);
      if (isatty(2)) dup2 (fd, 2);
   }

   /* Get termio settings and make a working copy */
   tcgetattr(__al_linux_console_fd, &__al_linux_startup_termio);
   __al_linux_work_termio = __al_linux_startup_termio;

   return 0;
}


/* done_console
 *  Undo `init_console'.
 */
static int done_console (void)
{
   char msg[256];
   int ret;

   if (__al_linux_prev_vt >= 0) {
      if (__al_linux_got_text_message) {
	 snprintf(msg, sizeof(msg), "\nProgram finished: press %s+F%d to switch back to the previous console\n", 
		      (__al_linux_prev_vt > 12) ? "AltGR" : "Alt", 
		      __al_linux_prev_vt%12);
	 msg[sizeof(msg)-1] = 0;

	 do {
	    ret = write(STDERR_FILENO, msg, strlen(msg));
	    if ((ret < 0) && (errno != EINTR))
	       break;
	 } while (ret < (int)strlen(msg));

	 __al_linux_got_text_message = FALSE;
      }
      else
	 ioctl (__al_linux_console_fd, VT_ACTIVATE, __al_linux_prev_vt);

      __al_linux_prev_vt = -1;
   }

   tcsetattr (__al_linux_console_fd, TCSANOW, &__al_linux_startup_termio);
   close (__al_linux_console_fd);
   __al_linux_console_fd = -1;

   return 0;
}


static int console_users = 0;

/* __al_linux_use_console:
 *   Init Linux console if not initialized yet.
 */
int __al_linux_use_console(void)
{
   console_users++;
   if (console_users > 1) return 0;

   if (init_console()) {
      console_users--;
      return 1;
   }

   /* Initialise the console switching system */
   set_display_switch_mode (SWITCH_PAUSE);
   return __al_linux_init_vtswitch();
}


/* __al_linux_leave_console:
 *   Close Linux console if no driver uses it any more.
 */
int __al_linux_leave_console(void)
{
   ASSERT (console_users > 0);
   console_users--;
   if (console_users > 0) return 0;

   /* shut down the console switching system */
   if (__al_linux_done_vtswitch()) return 1;
   if (done_console()) return 1;
   
   return 0;
}


static int graphics_mode = 0;

/* __al_linux_console_graphics:
 *   Puts the Linux console into graphics mode.
 */
int __al_linux_console_graphics (void)
{
   if (__al_linux_use_console()) return 1;

   if (graphics_mode) return 0;  /* shouldn't happen */

   __al_linux_display_switch_lock(TRUE, TRUE);
   ioctl(__al_linux_console_fd, KDSETMODE, KD_GRAPHICS);
   __al_linux_wait_for_display();

   graphics_mode = 1;

   return 0;
}


/* __al_linux_console_text:
 *  Returns the console to text mode.
 */
int __al_linux_console_text (void)
{
   int ret;

   if (!graphics_mode)
      return 0;  /* shouldn't happen */

   ioctl(__al_linux_console_fd, KDSETMODE, KD_TEXT);

   do {
      ret = write(__al_linux_console_fd, "\e[H\e[J\e[0m", 10);
      if ((ret < 0) && (errno != EINTR))
	 break;
   } while (ret < 10);

   graphics_mode = 0;
   
   __al_linux_display_switch_lock(FALSE, FALSE);
   __al_linux_leave_console();

   return 0;
}


/* __al_linux_wait_for_display:
 *  Waits until we have the display.
 */
int __al_linux_wait_for_display (void)
{
   int x;
   do {
      x = ioctl (__al_linux_console_fd, VT_WAITACTIVE, __al_linux_vt);
   } while (x && errno != EINTR);
   return x;
}

