/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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

#include <allegro.h>
#include <stdio.h>
#include <stdarg.h>

#include "jinete/jinete.h"

#include "core/app.h"
#include "core/core.h"
#include "modules/gui.h"
#include "widgets/statebar.h"

static JWidget wid_console = NULL;
static JWidget wid_view = NULL;
static JWidget wid_textbox = NULL;
static JWidget wid_cancel = NULL;
static int console_counter = 0;
static bool console_locked;
static bool want_close_flag = FALSE;

void console_open()
{
  console_counter++;

  /* TODO verify if the ji_screen works */
/*   if (!screen || */
  if (!ji_screen ||
      !is_interactive() ||
      wid_console ||
      console_counter > 1)
    return;
  else {
    JWidget window, grid, view, textbox, button;

    window = jwindow_new(_("Processing..."));
    if (!window)
      return;

    grid = jgrid_new(1, FALSE);
    view = jview_new();
    textbox = jtextbox_new(NULL, JI_WORDWRAP);
    button = jbutton_new(_("&Cancel"));

    if (!grid || !textbox || !button)
      return;

    jview_attach(view, textbox);

    jwidget_set_min_size(button, 60, 0);

    jgrid_add_child(grid, view, 1, 1, JI_HORIZONTAL | JI_VERTICAL);
    jgrid_add_child(grid, button, 1, 1, JI_CENTER);
    jwidget_add_child(window, grid);

    jwidget_hide(view);
    jwidget_magnetic(button, TRUE);
    jwidget_expansive(view, TRUE);

    /* force foreground mode */
/*     ji_find_widget(window)->in_foreground = TRUE; */

    wid_console = window;
    wid_view = view;
    wid_textbox = textbox;
    wid_cancel = button;
    console_locked = FALSE;
    want_close_flag = FALSE;
  }
}

void console_close()
{
  console_counter--;

  if ((wid_console) && (console_counter == 0)) {
    if (console_locked
	&& !want_close_flag
	&& jwidget_is_visible(wid_console)) {
      /* open in foreground */
      jwindow_open_fg(wid_console);
    }

    jwidget_free(wid_console);
    wid_console = NULL;
    want_close_flag = FALSE;
  }
}

void console_printf(const char *format, ...)
{
  char buf[1024];
  va_list ap;

  va_start(ap, format);
  uvsprintf(buf, format, ap);
  va_end(ap);

  if (wid_console) {
    const char* text;
    char* final;

    /* open the window */
    if (jwidget_is_hidden(wid_console)) {
      jwindow_open(wid_console);
      jmanager_refresh_screen();
    }

    /* update the textbox */
    if (!console_locked) {
      JRect rect = jrect_new(0, 0, JI_SCREEN_W*9/10, JI_SCREEN_H*6/10);
      console_locked = TRUE;
      
      jwidget_show(wid_view);

      jwindow_remap(wid_console);
      jwidget_set_rect(wid_console, rect);
      jwindow_center(wid_console);
      jwidget_dirty(wid_console);
    }

    text = jwidget_get_text(wid_textbox);
    if (!text)
      final = jstrdup(buf);
    else {
      final = (char*)jmalloc(ustrlen(text) + ustrlen(buf) + 1);

      ustrcpy(final, empty_string);
      ustrcat(final, text);
      ustrcat(final, buf);
    }

    jwidget_set_text(wid_textbox, final);
    jfree(final);
  }
  else {
    fputs(buf, stdout);
    fflush(stdout);
  }
}

void user_printf(const char *format, ...)
{
  char buf[1024];
  va_list ap;

  va_start(ap, format);
  uvsprintf(buf, format, ap);
  va_end(ap);

/*   if (script_is_running()) */
/*     plugin_printf(buf); */
/*   else */
  allegro_message(buf);
}
