/* ase -- allegro-sprite-editor: the ultimate sprites factory
 * Copyright (C) 2001-2005, 2007  David A. Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifndef USE_PRECOMPILED_HEADER

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <allegro.h>
#include <allegro/internal/aintern.h>

#include "jinete/base.h"
#include "jinete/list.h"
#include "jinete/system.h"

#include "modules/sprites.h"
#include "util/session.h"

#endif

/* call do_shutdown_exit routine?  */
static bool do_shutdown = FALSE;

/* try to use signals in mingw32 */
#ifdef ALLEGRO_MINGW32
  static void *old_sig_abrt;
  static void *old_sig_fpe;
  static void *old_sig_ill;
  static void *old_sig_segv;
  static void *old_sig_term;
  static void *old_sig_int;
#ifdef SIGQUIT
  static void *old_sig_quit;
#endif
#endif

static void do_shutdown_exit(void);
#ifdef ALLEGRO_MINGW32
static void signal_handler(int sig);
#endif

void init_shutdown_handler(void)
{
  atexit(do_shutdown_exit);
  _add_exit_func(do_shutdown_exit, "do_shutdown_exit");

#ifdef ALLEGRO_MINGW32
  old_sig_abrt = signal(SIGABRT, signal_handler);
  old_sig_fpe  = signal(SIGFPE,  signal_handler);
  old_sig_ill  = signal(SIGILL,  signal_handler);
  old_sig_segv = signal(SIGSEGV, signal_handler);
  old_sig_term = signal(SIGTERM, signal_handler);
  old_sig_int  = signal(SIGINT,  signal_handler);
#ifdef SIGQUIT
  old_sig_quit = signal(SIGQUIT, signal_handler);
#endif
#endif

  do_shutdown = TRUE;
}

void remove_shutdown_handler(void)
{
  do_shutdown = FALSE;

  _remove_exit_func(do_shutdown_exit);

#if defined(ALLEGRO_WINDOWS) && !defined(_MSC_VER)
  /* remove emergency exit signal handlers */
  signal(SIGABRT, old_sig_abrt);
  signal(SIGFPE,  old_sig_fpe);
  signal(SIGILL,  old_sig_ill);
  signal(SIGSEGV, old_sig_segv);
  signal(SIGTERM, old_sig_term);
  signal(SIGINT,  old_sig_int);
#ifdef SIGQUIT
  signal(SIGQUIT, old_sig_quit);
#endif
#endif
}

static void do_shutdown_exit(void)
{
  char buf[1024], path[1024], name[1024];
  int c;

  _remove_exit_func(do_shutdown_exit);

  /* do shutdown? */
  if (!do_shutdown)
    return;

  /* remove this shutdown handler */
  remove_shutdown_handler();

  /* is necessary create a session file? */
  if (jlist_length(get_sprite_list()) > 0) {
    /* get a file name for the session file */
    get_executable_name(path, sizeof(path));

    for (c=0; c<1000; c++) {
      usprintf(name, "data/session/ase%03d.ses", c);
      replace_filename(buf, path, name, sizeof(buf));
      if (!exists(buf))
	break;
    }

    if (screen) {
      jmouse_hide();
      text_mode(makecol(0, 0, 0));
      textprintf(screen, font, 0, 0, makecol(255, 255, 255),
		 "Saving session in %s", get_filename(buf));
      jmouse_show();
    }
    else
      printf("Saving session in %s", get_filename (buf));

    /* save the active session */
    save_session(buf);

    /* show the end message */
    PRINTF("Session saved in file: \"%s\"\n", buf);
  }

  /* set to text mode */
  set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
}

#ifdef ALLEGRO_MINGW32
static void signal_handler(int num)
{
  do_shutdown_exit();
  fprintf(stderr, "Shutting down ASE due to signal #%d\n", num);
  raise(num);
}
#endif
