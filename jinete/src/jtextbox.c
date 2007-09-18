/* jinete - a GUI library
 * Copyright (C) 2003-2005 by David A. Capello
 *
 * Jinete is gift-ware.
 */

#include <allegro/keyboard.h>
#include <math.h>

#include "jinete/hook.h"
#include "jinete/intern.h"
#include "jinete/manager.h"
#include "jinete/message.h"
#include "jinete/rect.h"
#include "jinete/system.h"
#include "jinete/theme.h"
#include "jinete/view.h"
#include "jinete/widget.h"

static bool textbox_msg_proc (JWidget widget, JMessage msg);
static void textbox_request_size (JWidget widget, int *w, int *h);

JWidget jtextbox_new (const char *text, int align)
{
  JWidget widget = jwidget_new (JI_TEXTBOX);

  jwidget_add_hook (widget, JI_TEXTBOX, textbox_msg_proc, NULL);
  jwidget_focusrest (widget, TRUE);
  jwidget_set_align (widget, align);
  jwidget_set_text (widget, text);

  jwidget_init_theme (widget);

  return widget;
}

static bool textbox_msg_proc (JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      textbox_request_size (widget, &msg->reqsize.w, &msg->reqsize.h);
      return TRUE;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_SET_TEXT) {
	JWidget view = jwidget_get_view (widget);
	if (view)
	  jview_update (view);
      }
      break;

    case JM_CHAR:
      if (jwidget_has_focus (widget)) {
	JWidget view = jwidget_get_view (widget);
	if (view) {
	  JRect vp = jview_get_viewport_position (view);
	  int textheight = jwidget_get_text_height (widget);
	  int scroll_x, scroll_y;

	  jview_get_scroll (view, &scroll_x, &scroll_y);

	  switch (msg->key.scancode) {

	    case KEY_LEFT:
	      jview_set_scroll (view, scroll_x-jrect_w(vp)/2, scroll_y);
	      break;

	    case KEY_RIGHT:
	      jview_set_scroll (view, scroll_x+jrect_w(vp)/2, scroll_y);
	      break;

	    case KEY_UP:
	      jview_set_scroll (view, scroll_x, scroll_y-jrect_h(vp)/2);
	      break;

	    case KEY_DOWN:
	      jview_set_scroll (view, scroll_x, scroll_y+jrect_h(vp)/2);
	      break;

	    case KEY_PGUP:
	      jview_set_scroll (view, scroll_x,
				  scroll_y-(jrect_h(vp)-textheight));
	      break;

	    case KEY_PGDN:
	      jview_set_scroll (view, scroll_x,
				  scroll_y+(jrect_h(vp)-textheight));
	      break;

	    case KEY_HOME:
	      jview_set_scroll (view, scroll_x, 0);
	      break;

	    case KEY_END:
	      jview_set_scroll (view, scroll_x,
				  jrect_h(widget->rc) - jrect_h(vp));
	      break;

	    default:
	      jrect_free (vp);
	      return FALSE;
	  }
	  jrect_free (vp);
	}
	return TRUE;
      }
      break;

    case JM_BUTTONPRESSED: {
      JWidget view = jwidget_get_view (widget);
      if (view) {
	jwidget_hard_capture_mouse (widget);
	ji_mouse_set_cursor (JI_CURSOR_MOVE);
	return TRUE;
      }
      break;
    }

    case JM_MOTION: {
      JWidget view = jwidget_get_view (widget);
      if (view && jwidget_has_capture (widget)) {
	JRect vp = jview_get_viewport_position (view);
	int scroll_x, scroll_y;

	jview_get_scroll (view, &scroll_x, &scroll_y);
	jview_set_scroll (view,
			    scroll_x + ji_mouse_x (1) - ji_mouse_x (0),
			    scroll_y + ji_mouse_y (1) - ji_mouse_y (0));

	ji_mouse_control_infinite_scroll (vp);
	jrect_free (vp);
      }
      break;
    }

    case JM_BUTTONRELEASED: {
      JWidget view = jwidget_get_view (widget);
      if (view && jwidget_has_capture (widget)) {
	jwidget_release_mouse (widget);
	ji_mouse_set_cursor (JI_CURSOR_NORMAL);
	return TRUE;
      }
      break;
    }

    case JM_WHEEL: {
      JWidget view = jwidget_get_view (widget);
      if (view) {
	int scroll_x, scroll_y;

	jview_get_scroll(view, &scroll_x, &scroll_y);
	jview_set_scroll(view,
			   scroll_x,
			   scroll_y +
			   (ji_mouse_z (1) - ji_mouse_z (0))
			   *jwidget_get_text_height(widget)*3);
      }
      break;
    }
  }

  return FALSE;
}

static void textbox_request_size (JWidget widget, int *w, int *h)
{
  /* XXX */
/*   *w = widget->border_width.l + widget->border_width.r; */
/*   *h = widget->border_width.t + widget->border_width.b; */
  *w = 0;
  *h = 0;

  _ji_theme_textbox_draw (NULL, widget, w, h);

  if (widget->align & JI_WORDWRAP) {
    JWidget view = jwidget_get_view (widget);
    int width, min = *w;

    if (view) {
      JRect vp = jview_get_viewport_position (view);
      width = jrect_w(vp);
      jrect_free (vp);
    }
    else {
      width = jrect_w(widget->rc);
    }

    *w = MAX (min, width);
    _ji_theme_textbox_draw (NULL, widget, w, h);

    *w = min;
  }
}

