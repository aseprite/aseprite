// ASE gui library
// Copyright (C) 2001-2010  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <allegro.h>

#include "gui/jfont.h"
#include "gui/jmanager.h"
#include "gui/jmessage.h"
#include "gui/jrect.h"
#include "gui/jsystem.h"
#include "gui/jtheme.h"
#include "gui/preferred_size_event.h"
#include "gui/slider.h"
#include "gui/widget.h"

static int slider_press_x;
static int slider_press_value;
static int slider_press_left;

Slider::Slider(int min, int max, int value)
  : Widget(JI_SLIDER)
{
  m_min = min;
  m_max = max;
  m_value = MID(min, value, max);

  jwidget_focusrest(this, true);
  jwidget_init_theme(this);
}

void Slider::setRange(int min, int max)
{
  m_min = min;
  m_max = max;
  m_value = MID(min, m_value, max);

  dirty();
}

void Slider::setValue(int value)
{
  int old_value = m_value;

  m_value = MID(m_min, value, m_max);

  if (m_value != old_value)
    dirty();

  // It DOES NOT emit CHANGE signal! to avoid recursive calls.
}

int Slider::getValue() const
{
  return m_value;
}

void Slider::getSliderThemeInfo(int* min, int* max, int* value)
{
  if (min) *min = m_min;
  if (max) *max = m_max;
  if (value) *value = m_value;
}

bool Slider::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_DRAW:
      this->theme->draw_slider(this, &msg->draw.rect);
      return true;

    case JM_FOCUSENTER:
    case JM_FOCUSLEAVE:
      if (isEnabled())
	dirty();
      break;

    case JM_BUTTONPRESSED:
      if (!isEnabled())
	return true;

      setSelected(true);
      captureMouse();

      slider_press_x = msg->mouse.x;
      slider_press_value = m_value;
      slider_press_left = msg->mouse.left;

      setupSliderCursor();

      /* continue to JM_MOTION */

    case JM_MOTION:
      if (hasCapture()) {
	int value, accuracy, range;
	JRect rc = jwidget_get_child_rect(this);

	range = m_max - m_min + 1;

	/* with left click */
	if (slider_press_left) {
	  value = m_min + range * (msg->mouse.x - rc->x1) / jrect_w(rc);
	}
	/* with right click */
	else {
	  accuracy = MID(1, jrect_w(rc) / range, jrect_w(rc));

	  value = slider_press_value +
	    (msg->mouse.x - slider_press_x) / accuracy;
	}

	value = MID(m_min, value, m_max);

	if (m_value != value) {
	  this->setValue(value);
	  onChange();
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
	  slider_press_value = m_value;
	}

	jrect_free(rc);
	return true;
      }
      break;

    case JM_BUTTONRELEASED:
      if (hasCapture()) {
	setSelected(false);
	releaseMouse();
	setupSliderCursor();

	onSliderReleased();
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
      if (isEnabled())
	dirty();
      break;

    case JM_KEYPRESSED:
      if (hasFocus()) {
	int min = m_min;
	int max = m_max;
	int value = m_value;

	switch (msg->key.scancode) {
	  case KEY_LEFT:  value = MAX(value-1, min); break;
	  case KEY_RIGHT: value = MIN(value+1, max); break;
	  case KEY_PGDN:  value = MAX(value-(max-min+1)/4, min); break;
	  case KEY_PGUP:  value = MIN(value+(max-min+1)/4, max); break;
	  case KEY_HOME:  value = min; break;
	  case KEY_END:   value = max; break;
	  default:
	    goto not_used;
	}

	if (m_value != value) {
	  this->setValue(value);
	  onChange();
	}

	return true;
      }
      break;

    case JM_WHEEL:
      if (isEnabled()) {
	int value = m_value + jmouse_z(0) - jmouse_z(1);

	value = MID(m_min, value, m_max);

	if (m_value != value) {
	  this->setValue(value);
	  onChange();
	}
	return true;
      }
      break;

    case JM_SETCURSOR:
      setupSliderCursor();
      return true;
  }

not_used:;
  return Widget::onProcessMessage(msg);
}

void Slider::onPreferredSize(PreferredSizeEvent& ev)
{
  int w, h, min_w, max_w;
  char buf[256];

  usprintf(buf, "%d", m_min);
  min_w = ji_font_text_len(this->getFont(), buf);

  usprintf(buf, "%d", m_max);
  max_w = ji_font_text_len(this->getFont(), buf);

  w = MAX(min_w, max_w);
  h = text_height(this->getFont());

  w += this->border_width.l + this->border_width.r;
  h += this->border_width.t + this->border_width.b;

  ev.setPreferredSize(w, h);
}

void Slider::onChange()
{
  Change(); // Emit Change signal
}

void Slider::onSliderReleased()
{
  SliderReleased();
}

void Slider::setupSliderCursor()
{
  if (hasCapture()) {
    if (slider_press_left)
      jmouse_set_cursor(JI_CURSOR_NORMAL);
    else
      jmouse_set_cursor(JI_CURSOR_SIZE_L);
  }
  else
    jmouse_set_cursor(JI_CURSOR_NORMAL);
}
