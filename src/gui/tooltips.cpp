// ASE gui library
// Copyright (C) 2001-2011  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#include "config.h"

#include <string>
#include <allegro.h>

#include "gfx/size.h"
#include "gui/gui.h"
#include "gui/intern.h"
#include "gui/preferred_size_event.h"

using namespace gfx;

struct TipData
{
  Widget* widget;	// Widget that shows the tooltip
  Frame* window;	// Frame where is the tooltip
  std::string text;
  int timer_id;
};

static int tip_type();
static bool tip_hook(JWidget widget, JMessage msg);

void jwidget_add_tooltip_text(JWidget widget, const char *text)
{
  TipData* tip = reinterpret_cast<TipData*>(jwidget_get_data(widget, tip_type()));

  ASSERT(text != NULL);

  if (tip == NULL) {
    tip = new TipData;

    tip->widget = widget;
    tip->window = NULL;
    tip->text = text;
    tip->timer_id = -1;

    jwidget_add_hook(widget, tip_type(), tip_hook, tip);
  }
  else {
    tip->text = text;
  }
}

/********************************************************************/
/* hook for widgets that want a tool-tip */

static int tip_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

/* hook for the widget in which we added a tooltip */
static bool tip_hook(JWidget widget, JMessage msg)
{
  TipData* tip = reinterpret_cast<TipData*>(jwidget_get_data(widget, tip_type()));

  switch (msg->type) {

    case JM_DESTROY:
      if (tip->timer_id >= 0)
	jmanager_remove_timer(tip->timer_id);

      delete tip;
      break;

    case JM_MOUSEENTER:
      if (tip->timer_id < 0)
	tip->timer_id = jmanager_add_timer(widget, 300);

      jmanager_start_timer(tip->timer_id);
      break;

    case JM_KEYPRESSED:
    case JM_BUTTONPRESSED:
    case JM_MOUSELEAVE:
      if (tip->window) {
	tip->window->closeWindow(NULL);
	delete tip->window;	// widget
	tip->window = NULL;
      }

      if (tip->timer_id >= 0)
	jmanager_stop_timer(tip->timer_id);
      break;

    case JM_TIMER:
      if (msg->timer.timer_id == tip->timer_id) {
	if (!tip->window) {
	  Frame* window = new TipWindow(tip->text.c_str(), true);
/* 	  int x = tip->widget->rc->x1; */
/* 	  int y = tip->widget->rc->y2; */
	  int x = jmouse_x(0)+12*jguiscale();
	  int y = jmouse_y(0)+12*jguiscale();
	  int w, h;

	  tip->window = window;

	  window->remap_window();

	  w = jrect_w(window->rc);
	  h = jrect_h(window->rc);

	  if (x+w > JI_SCREEN_W) {
	    x = jmouse_x(0) - w - 4*jguiscale();
	    y = jmouse_y(0);
	  }

	  window->position_window(MID(0, x, JI_SCREEN_W-w),
				  MID(0, y, JI_SCREEN_H-h));
	  window->open_window();
	}
	jmanager_stop_timer(tip->timer_id);
      }
      break;

  }
  return false;
}

/********************************************************************/
/* TipWindow */

TipWindow::TipWindow(const char *text, bool close_on_buttonpressed)
  : Frame(false, text)
{
  JLink link, next;

  m_close_on_buttonpressed = close_on_buttonpressed;
  m_hot_region = NULL;
  m_filtering = false;

  set_sizeable(false);
  set_moveable(false);
  set_wantfocus(false);
  setAlign(JI_LEFT | JI_TOP);

  // remove decorative widgets
  JI_LIST_FOR_EACH_SAFE(this->children, link, next)
    jwidget_free(reinterpret_cast<JWidget>(link->data));

  jwidget_init_theme(this);
}

TipWindow::~TipWindow()
{
  if (m_filtering) {
    m_filtering = false;
    jmanager_remove_msg_filter(JM_MOTION, this);
    jmanager_remove_msg_filter(JM_BUTTONPRESSED, this);
    jmanager_remove_msg_filter(JM_KEYPRESSED, this);
  }
  if (m_hot_region != NULL) {
    jregion_free(m_hot_region);
  }
}

/**
 * @param region The new hot-region. This pointer is holded by the @a widget.
 * So you can't destroy it after calling this routine.
 */
void TipWindow::set_hotregion(JRegion region)
{
  ASSERT(region != NULL);

  if (m_hot_region != NULL)
    jregion_free(m_hot_region);

  if (!m_filtering) {
    m_filtering = true;
    jmanager_add_msg_filter(JM_MOTION, this);
    jmanager_add_msg_filter(JM_BUTTONPRESSED, this);
    jmanager_add_msg_filter(JM_KEYPRESSED, this);
  }
  m_hot_region = region;
}

bool TipWindow::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      if (m_filtering) {
	m_filtering = false;
	jmanager_remove_msg_filter(JM_MOTION, this);
	jmanager_remove_msg_filter(JM_BUTTONPRESSED, this);
	jmanager_remove_msg_filter(JM_KEYPRESSED, this);
      }
      break;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_INIT_THEME) {
	int w = 0, h = 0;

	this->border_width.l = 3 * jguiscale();
	this->border_width.t = 3 * jguiscale();
	this->border_width.r = 3 * jguiscale();
	this->border_width.b = 3 * jguiscale();

	_ji_theme_textbox_draw(NULL, this, &w, &h, 0, 0);

	this->border_width.t = h - 3 * jguiscale();

	/* setup the background color */
	setBgColor(makecol(255, 255, 200));
	return true;
      }
      break;

    case JM_MOUSELEAVE:
      if (m_hot_region == NULL)
	this->closeWindow(NULL);
      break;

    case JM_KEYPRESSED:
      if (m_filtering && msg->key.scancode < KEY_MODIFIERS)
	this->closeWindow(NULL);
      break;

    case JM_BUTTONPRESSED:
      /* if the user click outside the window, we have to close the
	 tooltip window */
      if (m_filtering) {
	Widget* picked = this->pick(msg->mouse.x, msg->mouse.y);
	if (!picked || picked->getRoot() != this) {
	  this->closeWindow(NULL);
	}
      }

      /* this is used when the user click inside a small text
	 tooltip */
      if (m_close_on_buttonpressed)
	this->closeWindow(NULL);
      break;

    case JM_MOTION:
      if (m_hot_region != NULL &&
	  jmanager_get_capture() == NULL) {
	struct jrect box;

	/* if the mouse is outside the hot-region we have to close the window */
	if (!jregion_point_in(m_hot_region,
			      msg->mouse.x, msg->mouse.y, &box)) {
	  this->closeWindow(NULL);
	}
      }
      break;

  }

  return Frame::onProcessMessage(msg);
}

void TipWindow::onPreferredSize(PreferredSizeEvent& ev)
{
  Size resultSize(0, 0);

  _ji_theme_textbox_draw(NULL, this, &resultSize.w, &resultSize.h, 0, 0);
  resultSize.h = this->border_width.t + this->border_width.b;

  if (!jlist_empty(this->children)) {
    Size maxSize(0, 0);
    Size reqSize;
    JWidget child;
    JLink link;

    JI_LIST_FOR_EACH(this->children, link) {
      child = (JWidget)link->data;

      reqSize = child->getPreferredSize();

      maxSize.w = MAX(maxSize.w, reqSize.w);
      maxSize.h = MAX(maxSize.h, reqSize.h);
    }

    resultSize.w = MAX(resultSize.w, this->border_width.l + maxSize.w + this->border_width.r);
    resultSize.h += maxSize.h;
  }

  ev.setPreferredSize(resultSize);
}

void TipWindow::onPaint(PaintEvent& ev)
{
  JRect pos = jwidget_get_rect(this);
  int oldt;

  for (int i=0; i<jguiscale(); ++i) {
    jdraw_rect(pos, makecol(0, 0, 0));
    jrect_shrink(pos, 1);
  }

  jdraw_rectfill(pos, this->getBgColor());

  oldt = this->border_width.t;
  this->border_width.t = 3 * jguiscale();
  _ji_theme_textbox_draw(ji_screen, this, NULL, NULL,
			 this->getBgColor(),
			 ji_color_foreground());
  this->border_width.t = oldt;

  jrect_free(pos);
}
