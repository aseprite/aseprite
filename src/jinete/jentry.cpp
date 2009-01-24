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

#include <allegro.h>
#include <allegro/internal/aintern.h>
#include <stdarg.h>
#include <stdio.h>

#include "jinete/jclipboard.h"
#include "jinete/jfont.h"
#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

#define CHARACTER_LENGTH(f, c) ((f)->vtable->char_length((f), (c)))

typedef struct Entry
{
  int cursor;
  int scroll;
  int select;
  int timer_id;
  bool hidden : 1;
  bool state : 1;		/* show or not the text cursor */
  bool readonly : 1;
  bool password : 1;
  bool recent_focused : 1;
} Entry;

static bool entry_msg_proc(JWidget widget, JMessage msg);
static void entry_request_size(JWidget widget, int *w, int *h);

static int entry_get_cursor_from_mouse(JWidget widget, JMessage msg);

static void entry_forward_word(JWidget widget);
static void entry_backward_word(JWidget widget);

JWidget jentry_new(int maxsize, const char *format, ...)
{
  JWidget widget = jwidget_new(JI_ENTRY);
  Entry* entry = (Entry*)jnew(Entry, 1);
  char buf[4096];

  /* formatted string */
  if (format) {
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);
  }
  /* empty string */
  else {
    ustrcpy(buf, empty_string);
  }

  jwidget_add_hook(widget, JI_ENTRY, entry_msg_proc, entry);

  entry->cursor = 0;
  entry->scroll = 0;
  entry->select = 0;
  entry->timer_id = jmanager_add_timer(widget, 500);
  entry->hidden = FALSE;
  entry->state = FALSE;
  entry->password = FALSE;
  entry->readonly = FALSE;
  entry->recent_focused = FALSE;

  /* TODO support for text alignment and multi-line */
  /* widget->align = JI_LEFT | JI_MIDDLE; */
  widget->text_size = maxsize+1;
  widget->text = (char*)jmalloc(widget->text_size);
  jwidget_set_text(widget, buf);

  jwidget_focusrest(widget, TRUE);
  jwidget_init_theme(widget);

  return widget;
}

void jentry_readonly(JWidget widget, bool state)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));

  entry->readonly = state;
}

void jentry_password(JWidget widget, bool state)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));

  entry->password = state;
}

bool jentry_is_readonly(JWidget widget)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));

  return entry->readonly;
}

bool jentry_is_password(JWidget widget)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));

  return entry->password;
}

void jentry_show_cursor(JWidget widget)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));

  entry->hidden = FALSE;

  jwidget_dirty(widget);
}

void jentry_hide_cursor(JWidget widget)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));

  entry->hidden = TRUE;

  jwidget_dirty(widget);
}

void jentry_set_cursor_pos(JWidget widget, int pos)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));
  const char *text = widget->text;
  int x, c;

  entry->cursor = pos;

  /* backward scroll */
  if (entry->cursor < entry->scroll)
    entry->scroll = entry->cursor;

  /* forward scroll */
  entry->scroll--;
  do {
    x = widget->rc->x1 + widget->border_width.l;
    for (c=++entry->scroll; ; c++) {
      x += CHARACTER_LENGTH(widget->text_font,
			    (c < ustrlen(text))? ugetat(text, c): ' ');

      if (x >= widget->rc->x2-widget->border_width.r)
        break;
    }
  } while (entry->cursor >= c);

  jmanager_start_timer(entry->timer_id);
  entry->state = TRUE;

  jwidget_dirty(widget);
}

void jentry_select_text(JWidget widget, int from, int to)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));
  int end = ustrlen(widget->text);

  entry->select = from;
  jentry_set_cursor_pos(widget, from); // to move scroll
  jentry_set_cursor_pos(widget, (to >= 0)? to: end+to+1);

  jwidget_dirty(widget);
}

void jentry_deselect_text(JWidget widget)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));

  entry->select = -1;

  jwidget_dirty(widget);
}

void jtheme_entry_info(JWidget widget,
		       int *scroll, int *cursor, int *state,
		       int *selbeg, int *selend)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));

  if (scroll) *scroll = entry->scroll;
  if (cursor) *cursor = entry->cursor;
  if (state) *state = !entry->hidden && entry->state;

  if ((entry->select >= 0) &&
      (entry->cursor != entry->select)) {
    *selbeg = MIN(entry->cursor, entry->select);
    *selend = MAX(entry->cursor, entry->select)-1;
  }
  else {
    *selbeg = -1;
    *selend = -1;
  }
}

static bool entry_msg_proc(JWidget widget, JMessage msg)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));

  switch (msg->type) {

    case JM_DESTROY:
      jmanager_remove_timer(entry->timer_id);
      jfree(entry);
      break;

    case JM_REQSIZE:
      entry_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_TIMER:
      if (jwidget_has_focus(widget) &&
	  msg->timer.timer_id == entry->timer_id) {
	entry->state = entry->state ? FALSE: TRUE;
	jwidget_dirty(widget);
      }
      break;

    case JM_FOCUSENTER:
      jmanager_start_timer(entry->timer_id);

      entry->state = TRUE;
      jwidget_dirty(widget);

      jentry_select_text(widget, 0, -1);
      entry->recent_focused = TRUE;
      break;

    case JM_FOCUSLEAVE:
      jwidget_dirty(widget);

      jmanager_stop_timer(entry->timer_id);
      
      jentry_deselect_text(widget);
      entry->recent_focused = FALSE;
      break;

    case JM_KEYPRESSED:
      if (jwidget_has_focus(widget) && !jentry_is_readonly(widget)) {
	char* text = (char*)jmalloc(widget->text_size);
	int c, selbeg, selend;

	jtheme_entry_info(widget, NULL, NULL, NULL, &selbeg, &selend);

	ustrcpy(text, widget->text);

	switch (msg->key.scancode) {

	  case KEY_LEFT:
	    /* selection */
	    if (msg->any.shifts & KB_SHIFT_FLAG) {
	      if (entry->select < 0)
		entry->select = entry->cursor;
	    }
	    else
	      entry->select = -1;

	    /* backward word */
	    if (msg->any.shifts & KB_CTRL_FLAG)
	      entry_backward_word(widget);
	    /* backward char */
	    else if (entry->cursor > 0)
	      entry->cursor--;
	    break;

	  case KEY_RIGHT:
	    /* selection */
	    if (msg->any.shifts & KB_SHIFT_FLAG) {
	      if (entry->select < 0)
		entry->select = entry->cursor;
	    }
	    else
	      entry->select = -1;

	    /* forward word */
	    if (msg->any.shifts & KB_CTRL_FLAG)
	      entry_forward_word(widget);
	    /* forward char */
	    else if (entry->cursor < ustrlen (text))
	      entry->cursor++;
	    break;

	  case KEY_HOME:
	    /* selection */
	    if (msg->any.shifts & KB_SHIFT_FLAG) {
	      if (entry->select < 0)
		entry->select = entry->cursor;
	    }
	    else
	      entry->select = -1;

	    entry->cursor = 0;
	    break;

	  case KEY_END:
	    /* selection */
	    if (msg->any.shifts & KB_SHIFT_FLAG) {
	      if (entry->select < 0)
		entry->select = entry->cursor;
	    }
	    else
	      entry->select = -1;

	    entry->cursor = ustrlen(text);
	    break;

	  case KEY_DEL:
	    /* delete the entire selection */
	    if (selbeg >= 0) {
	      /* *cut* text! */
	      if (msg->any.shifts & KB_SHIFT_FLAG) {
		char buf[1024];
		ustrcpy(buf, empty_string);
		for (c=selbeg; c<=selend; c++)
		  uinsert(buf, ustrlen(buf), ugetat(text, c));
		jclipboard_set_text(buf);
	      }

	      /* remove text */
	      for (c=0; c<selend-selbeg+1; c++)
		uremove(text, selbeg);

	      entry->cursor = selbeg;
	    }
	    /* delete the next character */
	    else {
	      if (entry->cursor < ustrlen (text))
		uremove(text, entry->cursor);
	    }

	    entry->select = -1;
	    break;

	  case KEY_INSERT:
	    /* *paste* text */
	    if (msg->any.shifts & KB_SHIFT_FLAG) {
	      const char *clipboard;

	      if ((clipboard = jclipboard_get_text ())) {
		/* delete the entire selection */
		if (selbeg >= 0) {
		  for (c=0; c<selend-selbeg+1; c++)
		    uremove(text, selbeg);

		  entry->cursor = selbeg;
		  entry->select = -1;
		}

		/* paste text */
		for (c=0; c<ustrlen (clipboard); c++)
		  if (ustrsizez(text) < widget->text_size)
		    uinsert(text, entry->cursor+c, ugetat(clipboard, c));
		  else
		    break;

		jentry_set_cursor_pos(widget, entry->cursor+c);
	      }
	    }
	    /* *copy* text */
	    else if ((selbeg >= 0) && (msg->any.shifts & KB_CTRL_FLAG)) {
	      char buf[1024];
	      ustrcpy(buf, empty_string);
	      for (c=selbeg; c<=selend; c++)
		uinsert(buf, ustrlen(buf), ugetat(text, c));
	      jclipboard_set_text(buf);
	    }
	    break;

	  case KEY_BACKSPACE:
	    /* delete the entire selection */
	    if (selbeg >= 0) {
	      for (c=0; c<selend-selbeg+1; c++)
		uremove(text, selbeg);

	      entry->cursor = selbeg;
	    }
	    /* delete the previous character */
	    else {
	      if (entry->cursor > 0)
		uremove(text, --entry->cursor);
	    }

	    entry->select = -1;
	    break;

	  default:
	    if (msg->key.ascii >= 32) {
	      /* delete the entire selection */
	      if (selbeg >= 0) {
		for (c=0; c<selend-selbeg+1; c++)
		  uremove(text, selbeg);

		entry->cursor = selbeg;
	      }

	      /* put the character */
	      if (ustrsizez(text) < widget->text_size)
		uinsert(text, entry->cursor++, msg->key.ascii);

	      entry->select = -1;
	      break;
	    }
	    else {
	      jfree(text);
	      return FALSE;
	    }
	}

	if (ustrcmp(widget->text, text) != 0) {
	  jwidget_set_text(widget, text);
	  jwidget_emit_signal(widget, JI_SIGNAL_ENTRY_CHANGE);
	}

	jfree(text);

	jentry_set_cursor_pos(widget, entry->cursor);
	jwidget_dirty(widget);
	return TRUE;
      }
      break;

    case JM_BUTTONPRESSED:
      jwidget_capture_mouse(widget);

    case JM_MOTION:
      if (jwidget_has_capture(widget)) {
	const char *text = widget->text;
	bool move, dirty;
	int c, x;

	move = TRUE;
	dirty = FALSE;

	/* backward scroll */
	if (msg->mouse.x < widget->rc->x1) {
	  if (entry->scroll > 0) {
	    entry->cursor = --entry->scroll;
	    move = FALSE;
	    dirty = TRUE;
	    jwidget_dirty(widget);
	  }
	}
	/* forward scroll */
	else if (msg->mouse.x >= widget->rc->x2) {
	  if (entry->scroll < ustrlen(text)) {
	    entry->scroll++;
	    x = widget->rc->x1 + widget->border_width.l;
	    for (c=entry->scroll; ; c++) {
	      x += CHARACTER_LENGTH(widget->text_font,
				   (c < ustrlen(text))? ugetat(text, c): ' ');
	      if (x > widget->rc->x2-widget->border_width.r) {
		c--;
		break;
	      }
	      else if (!ugetat (text, c))
		break;
	    }
	    entry->cursor = c;
	    move = FALSE;
	    dirty = TRUE;
	    jwidget_dirty(widget);
	  }
	}

	/* move cursor */
	if (move) {
	  c = entry_get_cursor_from_mouse(widget, msg);

	  if (entry->cursor != c) {
	    entry->cursor = c;
	    dirty = TRUE;
	    jwidget_dirty(widget);
	  }
	}

	/* move selection */
	if (entry->recent_focused) {
	  entry->recent_focused = FALSE;
	  entry->select = entry->cursor;
	}
	else if (msg->type == JM_BUTTONPRESSED)
	  entry->select = entry->cursor;

	/* show the cursor */
	if (dirty) {
	  jmanager_start_timer(entry->timer_id);
	  entry->state = TRUE;
	}

	return TRUE;
      }
      break;

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture(widget))
	jwidget_release_mouse(widget);
      return TRUE;

    case JM_DOUBLECLICK:
      entry_forward_word(widget);
      entry->select = entry->cursor;
      entry_backward_word(widget);
      jwidget_dirty(widget);
      return TRUE;

    case JM_MOUSEENTER:
    case JM_MOUSELEAVE:
      /* TODO theme stuff */
      if (jwidget_is_enabled(widget))
	jwidget_dirty(widget);
      break;
  }

  return FALSE;
}

static void entry_request_size(JWidget widget, int *w, int *h)
{
  *w =
    + widget->border_width.l
    + ji_font_char_len(widget->text_font, 'w') * MIN(widget->text_size, 6)
    + 2 + widget->border_width.r;

  *w = MIN(*w, JI_SCREEN_W/2);

  *h = 
    + widget->border_width.t
    + text_height(widget->text_font)
    + widget->border_width.b;
}

static int entry_get_cursor_from_mouse(JWidget widget, JMessage msg)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));
  int c, x, w, mx, cursor = entry->cursor;

  mx = msg->mouse.x;
  mx = MID(widget->rc->x1+widget->border_width.l,
	   mx,
	   widget->rc->x2-widget->border_width.r-1);

  x = widget->rc->x1 + widget->border_width.l;
  for (c=entry->scroll; ugetat (widget->text, c); c++) {
    w = CHARACTER_LENGTH(widget->text_font, ugetat(widget->text, c));
    if (x+w >= widget->rc->x2-widget->border_width.r)
      break;
    if ((mx >= x) && (mx < x+w)) {
      cursor = c;
      break;
    }
    x += w;
  }

  if (!ugetat (widget->text, c))
    if ((mx >= x) &&
	(mx <= widget->rc->x2-widget->border_width.r-1))
      cursor = c;

  return cursor;
}

#define IS_WORD_CHAR(ch)				\
  (!((!ch) || (uisspace(ch)) ||				\
    ((ch) == '/') || ((ch) == OTHER_PATH_SEPARATOR)))

static void entry_forward_word(JWidget widget)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));
  int ch;

  for (; entry->cursor<ustrlen (widget->text); entry->cursor++) {
    ch = ugetat(widget->text, entry->cursor);
    if (IS_WORD_CHAR (ch))
      break;
  }

  for (; entry->cursor<ustrlen(widget->text); entry->cursor++) {
    ch = ugetat(widget->text, entry->cursor);
    if (!IS_WORD_CHAR(ch)) {
      entry->cursor++;
      break;
    }
  }
}

static void entry_backward_word(JWidget widget)
{
  Entry* entry = reinterpret_cast<Entry*>(jwidget_get_data(widget, JI_ENTRY));
  int ch;

  for (entry->cursor--; entry->cursor >= 0; entry->cursor--) {
    ch = ugetat(widget->text, entry->cursor);
    if (IS_WORD_CHAR(ch))
      break;
  }

  for (; entry->cursor >= 0; entry->cursor--) {
    ch = ugetat(widget->text, entry->cursor);
    if (!IS_WORD_CHAR(ch)) {
      entry->cursor++;
      break;
    }
  }

  if (entry->cursor < 0)
    entry->cursor = 0;
}
