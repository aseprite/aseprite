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

#include <allegro.h>
#include <stdio.h>
#include <stdarg.h>

#include "jinete.h"

#include "core/app.h"
#include "core/core.h"
#include "modules/gui.h"
#include "widgets/statebar.h"

#endif

static JWidget wid_console = NULL;
static JWidget wid_view = NULL;
static JWidget wid_textbox = NULL;
static JWidget wid_cancel = NULL;
static int console_counter = 0;
static bool console_locked;
static bool want_close_flag = FALSE;

void console_open(void)
{
  console_counter++;

  /* TODO verify if the ji_screen works */
/*   if (!screen || */
  if (!ji_screen ||
      !is_interactive () ||
      wid_console ||
      console_counter > 1)
    return;
  else {
    JWidget window, box1, view, textbox, button;

    window = jwindow_new (_("Processing..."));
    if (!window)
      return;

    box1 = jbox_new (JI_VERTICAL);
    view = jview_new ();
    textbox = jtextbox_new (NULL, JI_WORDWRAP);
    button = jbutton_new (_("&Cancel"));

    if (!box1 || !textbox || !button)
      return;

    jview_attach (view, textbox);

    jwidget_add_child (box1, view);
    jwidget_add_child (box1, button);
    jwidget_add_child (window, box1);

    jwidget_hide (view);
    jwidget_magnetic (button, TRUE);
    jwidget_expansive (view, TRUE);

    /* force foreground mode */
/*     ji_find_widget (window)->in_foreground = TRUE; */

    wid_console = window;
    wid_view = view;
    wid_textbox = textbox;
    wid_cancel = button;
    console_locked = FALSE;
    want_close_flag = FALSE;
  }
}

void console_close (void)
{
  console_counter--;

  if ((wid_console) && (console_counter == 0)) {
    if (console_locked &&
	!want_close_flag &&
	jwidget_is_visible (wid_console))
      jwindow_open_fg (wid_console);

    jwidget_free (wid_console);
    wid_console = NULL;
    want_close_flag = FALSE;
  }
}

void console_printf(const char *format, ...)
{
  char buf[1024];
  va_list ap;

  va_start (ap, format);
  uvsprintf (buf, format, ap);
  va_end (ap);

  if (wid_console) {
    const char *text;
    char *final;

    /* open the window */
    if (jwidget_is_hidden (wid_console)) {
      jwindow_open (wid_console);
      jmanager_refresh_screen ();
    }

    /* update the textbox */
    if (!console_locked) {
      console_locked = TRUE;

      jwidget_set_static_size(wid_view, JI_SCREEN_W*9/10, JI_SCREEN_H*6/10);
      jwidget_show(wid_view);

      jwindow_remap(wid_console);
      jwindow_center(wid_console);
      jwidget_dirty(wid_console);
    }

    text = jwidget_get_text (wid_textbox);
    if (!text)
      final = jstrdup (buf);
    else {
      final = jmalloc (ustrlen (text) + ustrlen (buf) + 1);

      ustrcpy (final, empty_string);
      ustrcat (final, text);
      ustrcat (final, buf);
    }

    jwidget_set_text (wid_textbox, final);
    jfree (final);
  }
  else
    printf (buf);
}

void user_printf (const char *format, ...)
{
  char buf[1024];
  va_list ap;

  va_start (ap, format);
  uvsprintf (buf, format, ap);
  va_end (ap);

/*   if (script_is_running ()) */
/*     plugin_printf (buf); */
/*   else */
  allegro_message (buf);
}

void do_progress(int progress)
{
  JWidget status_bar = app_get_status_bar();

  if (status_bar) {
    status_bar_do_progress(status_bar, progress);

    jwidget_flush_redraw(status_bar);
    jmanager_dispatch_messages();
    gui_feedback();
  }
}

void add_progress(int max)
{
  JWidget status_bar = app_get_status_bar();

  if (status_bar)
    status_bar_add_progress(status_bar, max);

  jmouse_hide();
}

void del_progress(void)
{
  JWidget status_bar = app_get_status_bar();

  if (status_bar)
    status_bar_del_progress(status_bar);

  jmouse_show();
}
