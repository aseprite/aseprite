/* Jinete - a GUI library
 * Copyright (C) 2003-2009 David Capello.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of the author nor the names of its contributors may
 *     be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/***********************************************************************
 
  Alert format:
  ------------

     "Title Text"
     "==Centre line of text"
     "--"
     "<<Left line of text"
     ">>Right line of text"
     "||My &Button||&Your Button||&He Button"

    .-----------------------------------------.
    | My Message                              |
    +=========================================+
    |           Centre line of text           |
    | --------------------------------------- |
    | Left line of text                       |
    |                      Right line of text |
    |                                         |
    | [My Button]  [Your Button] [Him Button] |
    `-----~---------~-------------~-----------'
 */

#include <stdarg.h>
#include <stdio.h>
#include <allegro/unicode.h>

#include "jinete/jinete.h"

static JWidget create_alert(char *buf, JList *labels, JList *buttons);

/* creates a new alert-box

   the buttons will have names like: button-1, button-2, etc.
 */
JWidget jalert_new(const char *format, ...)
{
  JList labels, buttons;
  char buf[4096];
  JWidget window;
  va_list ap;

  /* process arguments */
  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  /* create the alert window */
  labels = jlist_new();
  buttons = jlist_new();

  window = create_alert(buf, &labels, &buttons);

  jlist_free(labels);
  jlist_free(buttons);

  /* and return it */
  return window;
}

int jalert(const char *format, ...)
{
  JList labels, buttons;
  JWidget window, killer;
  char buf[4096];
  int c, ret = 0;
  JLink link;
  va_list ap;

  /* process arguments */
  va_start(ap, format);
  vsprintf(buf, format, ap);
  va_end(ap);

  /* create the alert window */
  labels = jlist_new();
  buttons = jlist_new();

  window = create_alert(buf, &labels, &buttons);

  /* was created succefully? */
  if (window) {
    /* open it */
    jwindow_open_fg(window);

    /* check the killer */
    killer = jwindow_get_killer(window);
    if (killer) {
      c = 1;
      JI_LIST_FOR_EACH(buttons, link) {
	if (killer == (JWidget)link->data) {
	  ret = c;
	  break;
	}
	c++;
      }
    }

    /* destroy the window */
    jwidget_free(window);
  }

  jlist_free(labels);
  jlist_free(buttons);

  /* and return it */
  return ret;
}

static JWidget create_alert(char *buf, JList *labels, JList *buttons)
{
  JWidget box1, box2, grid, box3, box4, box5, window = NULL;
  bool title = TRUE;
  bool label = FALSE;
  bool separator = FALSE;
  bool button = FALSE;
  int align = 0;
  char *beg;
  int c, chr;
  JLink link;

  /* process buffer */
  c = 0;
  beg = buf;
  for (; ; c++) {
    if ((!buf[c]) ||
	((buf[c] == buf[c+1]) &&
	 ((buf[c] == '<') ||
	  (buf[c] == '=') ||
	  (buf[c] == '>') ||
	  (buf[c] == '-') ||
	  (buf[c] == '|')))) {
      if (title || label || separator || button) {
	chr = buf[c];
	buf[c] = 0;

	if (title) {
	  window = jwindow_new(beg);
	}
	else if (label) {
	  JWidget label = jlabel_new(beg);
	  jwidget_set_align(label, align);
	  jlist_append(*labels, label);
	}
	else if (separator) {
	  jlist_append(*labels, ji_separator_new(NULL, JI_HORIZONTAL));
	}
	else if (button) {
	  char button_name[256];
	  JWidget button_widget = jbutton_new(beg);
	  jwidget_set_min_size(button_widget, 60, 0);
	  jlist_append(*buttons, button_widget);

	  usprintf(button_name, "button-%d", jlist_length(*buttons));
	  jwidget_set_name(button_widget, button_name);
	}

	buf[c] = chr;
      }

      /* done */
      if (!buf[c])
	break;
      /* next widget */
      else {
	title = label = separator = button = FALSE;
	beg = buf+c+2;
	align = 0;

	switch (buf[c]) {
	  case '<': label=TRUE; align=JI_LEFT; break;
	  case '=': label=TRUE; align=JI_CENTER; break;
	  case '>': label=TRUE; align=JI_RIGHT; break;
	  case '-': separator=TRUE; break;
	  case '|': button=TRUE; break;
	}
	c++;
      }
    }
  }

  if (window) {
    box1 = jbox_new(JI_VERTICAL);
    box2 = jbox_new(JI_VERTICAL);
    grid = jgrid_new(1, FALSE);
    box3 = jbox_new(JI_HORIZONTAL | JI_HOMOGENEOUS);

    /* to identify by the user */
    jwidget_set_name(box2, "labels");
    jwidget_set_name(box3, "buttons");

    /* pseudo separators (only to fill blank space) */
    box4 = jbox_new(0);
    box5 = jbox_new(0);

    jwidget_expansive(box4, TRUE);
    jwidget_expansive(box5, TRUE);
    jwidget_noborders(box4);
    jwidget_noborders(box5);

    /* setup parent <-> children relationship */

    jwidget_add_child(window, box1);

    jwidget_add_child(box1, box4);	/* filler */
    jwidget_add_child(box1, box2);	/* labels */
    jwidget_add_child(box1, box5);	/* filler */
    jwidget_add_child(box1, grid);	/* buttons */

    jgrid_add_child(grid, box3, 1, 1, JI_CENTER | JI_BOTTOM);

    JI_LIST_FOR_EACH(*labels, link)
      jwidget_add_child(box2, (JWidget)link->data);

    JI_LIST_FOR_EACH(*buttons, link)
      jwidget_add_child(box3, (JWidget)link->data);

    /* default button is the last one */
    if (jlist_last(*buttons))
      jwidget_magnetic((JWidget)jlist_last(*buttons)->data, TRUE);
  }

  return window;
}
