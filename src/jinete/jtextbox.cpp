/* Jinete - a GUI library
 * Copyright (C) 2003-2010 David Capello.
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

#include "config.h"

#include <allegro/keyboard.h>
#include <math.h>

#include "jinete/jhook.h"
#include "jinete/jintern.h"
#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"

static bool textbox_msg_proc(JWidget widget, JMessage msg);
static void textbox_request_size(JWidget widget, int *w, int *h);

JWidget jtextbox_new(const char *text, int align)
{
  Widget* widget = new Widget(JI_TEXTBOX);

  jwidget_add_hook(widget, JI_TEXTBOX, textbox_msg_proc, NULL);
  jwidget_focusrest(widget, true);
  widget->setAlign(align);
  widget->setText(text);

  jwidget_init_theme(widget);

  return widget;
}

static bool textbox_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      textbox_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_DRAW:
      widget->theme->draw_textbox(widget, &msg->draw.rect);
      return true;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_SET_TEXT) {
	JWidget view = jwidget_get_view(widget);
	if (view)
	  jview_update(view);
      }
      break;

    case JM_KEYPRESSED:
      if (widget->hasFocus()) {
	JWidget view = jwidget_get_view(widget);
	if (view) {
	  JRect vp = jview_get_viewport_position(view);
	  int textheight = jwidget_get_text_height(widget);
	  int scroll_x, scroll_y;

	  jview_get_scroll(view, &scroll_x, &scroll_y);

	  switch (msg->key.scancode) {

	    case KEY_LEFT:
	      jview_set_scroll(view, scroll_x-jrect_w(vp)/2, scroll_y);
	      break;

	    case KEY_RIGHT:
	      jview_set_scroll(view, scroll_x+jrect_w(vp)/2, scroll_y);
	      break;

	    case KEY_UP:
	      jview_set_scroll(view, scroll_x, scroll_y-jrect_h(vp)/2);
	      break;

	    case KEY_DOWN:
	      jview_set_scroll(view, scroll_x, scroll_y+jrect_h(vp)/2);
	      break;

	    case KEY_PGUP:
	      jview_set_scroll(view, scroll_x,
			       scroll_y-(jrect_h(vp)-textheight));
	      break;

	    case KEY_PGDN:
	      jview_set_scroll(view, scroll_x,
			       scroll_y+(jrect_h(vp)-textheight));
	      break;

	    case KEY_HOME:
	      jview_set_scroll(view, scroll_x, 0);
	      break;

	    case KEY_END:
	      jview_set_scroll(view, scroll_x,
			       jrect_h(widget->rc) - jrect_h(vp));
	      break;

	    default:
	      jrect_free(vp);
	      return false;
	  }
	  jrect_free(vp);
	}
	return true;
      }
      break;

    case JM_BUTTONPRESSED: {
      JWidget view = jwidget_get_view(widget);
      if (view) {
	widget->captureMouse();
	jmouse_set_cursor(JI_CURSOR_SCROLL);
	return true;
      }
      break;
    }

    case JM_MOTION: {
      JWidget view = jwidget_get_view(widget);
      if (view && widget->hasCapture()) {
	JRect vp = jview_get_viewport_position(view);
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	jview_set_scroll(view,
			 scroll_x + jmouse_x(1) - jmouse_x(0),
			 scroll_y + jmouse_y(1) - jmouse_y(0));

	jmouse_control_infinite_scroll(vp);
	jrect_free(vp);
      }
      break;
    }

    case JM_BUTTONRELEASED: {
      JWidget view = jwidget_get_view(widget);
      if (view && widget->hasCapture()) {
	widget->releaseMouse();
	jmouse_set_cursor(JI_CURSOR_NORMAL);
	return true;
      }
      break;
    }

    case JM_WHEEL: {
      JWidget view = jwidget_get_view(widget);
      if (view) {
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	jview_set_scroll(view,
			 scroll_x,
			 scroll_y +
			 (jmouse_z(1) - jmouse_z(0))
			 *jwidget_get_text_height(widget)*3);
      }
      break;
    }
  }

  return false;
}

static void textbox_request_size(JWidget widget, int *w, int *h)
{
  /* TODO */
/*   *w = widget->border_width.l + widget->border_width.r; */
/*   *h = widget->border_width.t + widget->border_width.b; */
  *w = 0;
  *h = 0;

  _ji_theme_textbox_draw(NULL, widget, w, h, 0, 0);

  if (widget->getAlign() & JI_WORDWRAP) {
    JWidget view = jwidget_get_view(widget);
    int width, min = *w;

    if (view) {
      JRect vp = jview_get_viewport_position(view);
      width = jrect_w(vp);
      jrect_free(vp);
    }
    else {
      width = jrect_w(widget->rc);
    }

    *w = MAX(min, width);
    _ji_theme_textbox_draw(NULL, widget, w, h, 0, 0);

    *w = min;
  }
}

