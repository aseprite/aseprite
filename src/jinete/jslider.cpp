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

#include <allegro.h>

#include "jinete/jfont.h"
#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jtheme.h"
#include "jinete/jwidget.h"

typedef struct Slider
{
  int min;
  int max;
  int value;
} Slider;

static int slider_press_x;
static int slider_press_value;
static int slider_press_left;

static bool slider_msg_proc(JWidget widget, JMessage msg);
static void slider_request_size(JWidget widget, int *w, int *h);
static void slider_setcursor(JWidget widget);

JWidget jslider_new(int min, int max, int value)
{
  Widget* widget = new Widget(JI_SLIDER);
  Slider* slider = jnew(Slider, 1);

  slider->min = min;
  slider->max = max;
  slider->value = MID(min, value, max);

  jwidget_add_hook(widget, JI_SLIDER, slider_msg_proc, slider);
  jwidget_focusrest(widget, true);
  jwidget_init_theme(widget);

  return widget;
}

void jslider_set_range(JWidget widget, int min, int max)
{
  Slider* slider = reinterpret_cast<Slider*>(jwidget_get_data(widget, JI_SLIDER));

  slider->min = min;
  slider->max = max;
  slider->value = MID(min, slider->value, max);

  jwidget_dirty(widget);
}

void jslider_set_value(JWidget widget, int value)
{
  Slider* slider = reinterpret_cast<Slider*>(jwidget_get_data(widget, JI_SLIDER));
  int old_value = slider->value;

  slider->value = MID(slider->min, value, slider->max);

  if (slider->value != old_value)
    jwidget_dirty(widget);

  /* it DOES NOT emit CHANGE signal! to avoid recursive calls */
}

int jslider_get_value(JWidget widget)
{
  Slider* slider = reinterpret_cast<Slider*>(jwidget_get_data(widget, JI_SLIDER));

  return slider->value;
}

void jtheme_slider_info(JWidget widget, int *min, int *max, int *value)
{
  Slider* slider = reinterpret_cast<Slider*>(jwidget_get_data(widget, JI_SLIDER));

  if (min) *min = slider->min;
  if (max) *max = slider->max;
  if (value) *value = slider->value;
}

static bool slider_msg_proc(JWidget widget, JMessage msg)
{
  Slider* slider = reinterpret_cast<Slider*>(jwidget_get_data(widget, JI_SLIDER));

  switch (msg->type) {

    case JM_DESTROY:
      jfree(slider);
      break;

    case JM_REQSIZE:
      slider_request_size(widget, &msg->reqsize.w, &msg->reqsize.h);
      return true;

    case JM_DRAW:
      widget->theme->draw_slider(widget, &msg->draw.rect);
      return true;

    case JM_FOCUSENTER:
    case JM_FOCUSLEAVE:
      if (widget->isEnabled())
	jwidget_dirty(widget);
      break;

    case JM_BUTTONPRESSED:
      if (!widget->isEnabled())
	return true;

      widget->setSelected(true);
      widget->captureMouse();

      slider_press_x = msg->mouse.x;
      slider_press_value = slider->value;
      slider_press_left = msg->mouse.left;

      slider_setcursor(widget);

      /* continue to JM_MOTION */

    case JM_MOTION:
      if (widget->hasCapture()) {
	int value, accuracy, range;
	JRect rc = jwidget_get_child_rect(widget);

	range = slider->max - slider->min + 1;

	/* with left click */
	if (slider_press_left) {
	  value = slider->min + range * (msg->mouse.x - rc->x1) / jrect_w(rc);
	}
	/* with right click */
	else {
	  accuracy = MID(1, jrect_w(rc) / range, jrect_w(rc));

	  value = slider_press_value +
	    (msg->mouse.x - slider_press_x) / accuracy;
	}

	value = MID(slider->min, value, slider->max);

	if (slider->value != value) {
	  jslider_set_value(widget, value);
	  jwidget_emit_signal(widget, JI_SIGNAL_SLIDER_CHANGE);
	}

	/* for left click */
	if (slider_press_left) {
	  int x = jmouse_x(0);

	  if (x < rc->x1-1)
	    x = rc->x1-1;
	  else if (x > rc->x2)
	    x = rc->x2;

	  if (x != jmouse_x(0))
	    jmouse_set_position(x, jmouse_y(0));
	}
	/* for right click */
	else if (jmouse_control_infinite_scroll(rc)) {
	  slider_press_x = jmouse_x(0);
	  slider_press_value = slider->value;
	}

	jrect_free(rc);
	return true;
      }
      break;

    case JM_BUTTONRELEASED:
      if (widget->hasCapture()) {
	widget->setSelected(false);
	widget->releaseMouse();
	slider_setcursor(widget);
      }
      break;

    case JM_MOUSEENTER:
    case JM_MOUSELEAVE:
/*       if (jwidget_is_enabled(widget) && */
/* 	  jwidget_has_capture(widget)) { */
/* 	/\* swap the select status *\/ */
/* 	if (jwidget_is_selected(widget)) */
/* 	  jwidget_deselect(widget); */
/* 	else */
/* 	  jwidget_select(widget); */

/* 	/\* TODO switch slider signal *\/ */
/*       } */

      /* TODO theme stuff */
      if (widget->isEnabled())
	jwidget_dirty(widget);
      break;

    case JM_KEYPRESSED:
      if (widget->hasFocus()) {
	int min = slider->min;
	int max = slider->max;
	int value = slider->value;

	switch (msg->key.scancode) {
	  case KEY_LEFT:  value = MAX(value-1, min); break;
	  case KEY_RIGHT: value = MIN(value+1, max); break;
	  case KEY_PGDN:  value = MAX(value-(max-min+1)/4, min); break;
	  case KEY_PGUP:  value = MIN(value+(max-min+1)/4, max); break;
	  case KEY_HOME:  value = min; break;
	  case KEY_END:   value = max; break;
	  default:
	    return false;
	}

	if (slider->value != value) {
	  jslider_set_value(widget, value);
	  jwidget_emit_signal(widget, JI_SIGNAL_SLIDER_CHANGE);
	}

	return true;
      }
      break;

    case JM_WHEEL:
      if (widget->isEnabled()) {
	int value = slider->value + jmouse_z(0) - jmouse_z(1);

	value = MID(slider->min, value, slider->max);

	if (slider->value != value) {
	  jslider_set_value(widget, value);
	  jwidget_emit_signal(widget, JI_SIGNAL_SLIDER_CHANGE);
	}
	return true;
      }
      break;

    case JM_SETCURSOR:
      slider_setcursor(widget);
      return true;
  }

  return false;
}

static void slider_request_size(JWidget widget, int *w, int *h)
{
  Slider* slider = reinterpret_cast<Slider*>(jwidget_get_data(widget, JI_SLIDER));
  int min_w, max_w;
  char buf[256];

  usprintf(buf, "%d", slider->min);
  min_w = ji_font_text_len(widget->getFont(), buf);

  usprintf(buf, "%d", slider->max);
  max_w = ji_font_text_len(widget->getFont(), buf);

  *w = MAX(min_w, max_w);
  *h = text_height(widget->getFont());

  *w += widget->border_width.l + widget->border_width.r;
  *h += widget->border_width.t + widget->border_width.b;
}

static void slider_setcursor(JWidget widget)
{
  if (widget->hasCapture()) {
    if (slider_press_left)
      jmouse_set_cursor(JI_CURSOR_NORMAL);
    else
      jmouse_set_cursor(JI_CURSOR_SIZE_L);
  }
  else
    jmouse_set_cursor(JI_CURSOR_NORMAL);
}
