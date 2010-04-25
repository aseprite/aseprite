/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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
#include <cmath>
#include <stdio.h>
#include <stdlib.h>

#include "jinete/jalert.h"
#include "jinete/jentry.h"
#include "jinete/jlist.h"
#include "jinete/jmanager.h"
#include "jinete/jmessage.h"
#include "jinete/jrect.h"
#include "jinete/jsystem.h"
#include "jinete/jview.h"
#include "jinete/jwidget.h"
#include "jinete/jwindow.h"

#include "effect/colcurve.h"
#include "modules/gui.h"
#include "widgets/curvedit.h"

#define SCR2EDIT_X(xpos)						\
  (curve_editor->x1 +							\
   ((curve_editor->x2 - curve_editor->x1 + 1)				\
    * ((xpos) - widget->rc->x1 - widget->border_width.l)		\
    / (jrect_w(widget->rc) - widget->border_width.l - widget->border_width.r)))

#define SCR2EDIT_Y(ypos)						\
  (curve_editor->y1 +							\
   ((curve_editor->y2 - curve_editor->y1 + 1)				\
    * ((jrect_h(widget->rc) - widget->border_width.t - widget->border_width.b) \
       - ((ypos) - widget->rc->y1 - widget->border_width.t))		\
    / (jrect_h(widget->rc) - widget->border_width.t - widget->border_width.b)))

#define EDIT2SCR_X(xpos)						\
  (widget->rc->x1 + widget->border_width.l				\
   + ((jrect_w(widget->rc) - widget->border_width.l - widget->border_width.r) \
      * ((xpos) - curve_editor->x1)					\
      / (curve_editor->x2 - curve_editor->x1 + 1)))

#define EDIT2SCR_Y(ypos)						\
  (widget->rc->y1							\
   + (jrect_h(widget->rc) - widget->border_width.t - widget->border_width.b) \
   - ((jrect_h(widget->rc) - widget->border_width.t - widget->border_width.b) \
      * ((ypos) - curve_editor->y1)					\
      / (curve_editor->y2 - curve_editor->y1 + 1)))

enum {
  STATUS_STANDBY,
  STATUS_MOVING_POINT,
  STATUS_SCROLLING,
  STATUS_SCALING,
};

typedef struct CurveEditor
{
  Curve *curve;
  int x1, y1;
  int x2, y2;
  int status;
  CurvePoint* edit_point;
  int *edit_x;
  int *edit_y;
} CurveEditor;

static CurveEditor* curve_editor_data(JWidget widget);
static bool curve_editor_msg_proc(JWidget widget, JMessage msg);

static CurvePoint* curve_editor_get_more_close_point(JWidget widget, int x, int y, int **edit_x, int **edit_y);

static int edit_node_manual(CurvePoint* point);

JWidget curve_editor_new(Curve *curve, int x1, int y1, int x2, int y2)
{
  JWidget widget = new Widget(curve_editor_type());
  CurveEditor* curve_editor = jnew(CurveEditor, 1);

  jwidget_add_hook(widget, curve_editor_type(),
		   curve_editor_msg_proc, curve_editor);
  jwidget_focusrest(widget, true);

  widget->border_width.l = widget->border_width.r = 1;
  widget->border_width.t = widget->border_width.b = 1;
  widget->child_spacing = 0;

  curve_editor->curve = curve;
  curve_editor->x1 = x1;
  curve_editor->y1 = y1;
  curve_editor->x2 = x2;
  curve_editor->y2 = y2;
  curve_editor->status = STATUS_STANDBY;
  curve_editor->edit_point = NULL;

  /* TODO */
  /* curve_editor->curve->type = CURVE_SPLINE; */

  return widget;
}

int curve_editor_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

Curve *curve_editor_get_curve(JWidget widget)
{
  CurveEditor* curve_editor = curve_editor_data(widget);

  return curve_editor->curve;
}

static CurveEditor* curve_editor_data(JWidget widget)
{
  return reinterpret_cast<CurveEditor*>
    (jwidget_get_data(widget, curve_editor_type()));
}

static bool curve_editor_msg_proc(JWidget widget, JMessage msg)
{
  CurveEditor* curve_editor = curve_editor_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      jfree(curve_editor);
      break;

    case JM_REQSIZE: {
#if 0
      msg->reqsize.w =
	+ widget->border_width.l
	+ ((curve_editor->x2 - curve_editor->x1 + 1))
	+ widget->border_width.r;

      msg->reqsize.h =
	+ widget->border_width.t
	+ ((curve_editor->y2 - curve_editor->y1 + 1))
	+ widget->border_width.b;
#else
      msg->reqsize.w = widget->border_width.l + 1 + widget->border_width.r;
      msg->reqsize.h = widget->border_width.t + 1 + widget->border_width.b;
#endif
      return true;
    }

    case JM_KEYPRESSED: {
      switch (msg->key.scancode) {

	case KEY_INSERT: {
	  int x = SCR2EDIT_X(jmouse_x(0));
	  int y = SCR2EDIT_Y(jmouse_y(0));
	  CurvePoint* point = curve_point_new(x, y); 

	  /* TODO undo? */
	  curve_add_point(curve_editor->curve, point);

	  jwidget_dirty(widget);
	  jwidget_emit_signal(widget, SIGNAL_CURVE_EDITOR_CHANGE);
	  break;
	}

	case KEY_DEL: {
	  CurvePoint* point = curve_editor_get_more_close_point
	    (widget,
	     SCR2EDIT_X(jmouse_x(0)),
	     SCR2EDIT_Y(jmouse_y(0)),
	     NULL, NULL);

	  /* TODO undo? */
	  curve_remove_point(curve_editor->curve, point);

	  jwidget_dirty(widget);
	  jwidget_emit_signal(widget, SIGNAL_CURVE_EDITOR_CHANGE);
	  break;
	}

	default:
	  return false;
      }
      return true;
    }

    case JM_DRAW: {
      BITMAP *bmp;
      JLink link;
      CurvePoint* point;
      int *values;
      int x, y, u;

      bmp = create_bitmap(jrect_w(widget->rc), jrect_h(widget->rc));
      clear_to_color(bmp, makecol (0, 0, 0));

      /* draw border */
      rect(bmp, 0, 0, bmp->w-1, bmp->h-1, makecol (255, 255, 0));

      /* draw guides */
      for (x=1; x<=3; x++)
	vline(bmp, x*bmp->w/4, 1, bmp->h-2, makecol (128, 128, 0));

      for (y=1; y<=3; y++)
	hline(bmp, 1, y*bmp->h/4, bmp->w-2, makecol (128, 128, 0));

      /* get curve values */
      values = (int*)jmalloc(sizeof(int) * (curve_editor->x2-curve_editor->x1+1));
      curve_get_values(curve_editor->curve,
		       curve_editor->x1,
		       curve_editor->x2, values);

      /* draw curve */
      for (x=widget->border_width.l;
	   x<jrect_w(widget->rc)-widget->border_width.r; x++) {
	u = SCR2EDIT_X(widget->rc->x1+x);
	u = MID(curve_editor->x1, u, curve_editor->x2);

	y = values[u - curve_editor->x1];
	y = MID(curve_editor->y1, y, curve_editor->y2);

	putpixel(bmp, x, EDIT2SCR_Y(y)-widget->rc->y1,
		 makecol(255, 255, 255));
      }

      jfree(values);

      /* draw nodes */
      JI_LIST_FOR_EACH(curve_editor->curve->points, link) {
	point = reinterpret_cast<CurvePoint*>(link->data);

	x = EDIT2SCR_X(point->x) - widget->rc->x1;
	y = EDIT2SCR_Y(point->y) - widget->rc->y1;

	rect(bmp, x-2, y-2, x+2, y+2,
	     curve_editor->edit_point == point ?
	     makecol(255, 255, 0): makecol(0, 0, 255));
      }

      /* blit to screen */
      blit(bmp, ji_screen,
	   0, 0, widget->rc->x1, widget->rc->y1, bmp->w, bmp->h);
      destroy_bitmap(bmp);
      return true;
    }

    case JM_BUTTONPRESSED:
      /* change scroll */
      if (msg->any.shifts & KB_SHIFT_FLAG) {
	curve_editor->status = STATUS_SCROLLING;
	jmouse_set_cursor(JI_CURSOR_SCROLL);
      }
      /* scaling */
/*       else if (msg->shifts & KB_CTRL_FLAG) { */
/* 	curve_editor->status = STATUS_SCALING; */
/* 	jmouse_set_cursor(JI_CURSOR_SCROLL); */
/*       } */
      /* show manual-entry dialog */
      else if (msg->mouse.right) {
	curve_editor->edit_point =
	  curve_editor_get_more_close_point(widget,
					    SCR2EDIT_X(msg->mouse.x),
					    SCR2EDIT_Y(msg->mouse.y),
					    NULL, NULL);
	if (curve_editor->edit_point) {
	  jwidget_dirty(widget);
	  jwidget_flush_redraw(widget);

	  if (edit_node_manual(curve_editor->edit_point))
	    jwidget_emit_signal(widget, SIGNAL_CURVE_EDITOR_CHANGE);

	  curve_editor->edit_point = NULL;
	  jwidget_dirty(widget);
	}
	
	return true;
      }
      /* edit node */
      else {
	curve_editor->edit_point =
	  curve_editor_get_more_close_point(widget,
					    SCR2EDIT_X(msg->mouse.x),
					    SCR2EDIT_Y(msg->mouse.y),
					    &curve_editor->edit_x,
					    &curve_editor->edit_y);

	curve_editor->status = STATUS_MOVING_POINT;
	jmouse_set_cursor(JI_CURSOR_HAND);
      }

      widget->captureMouse();
      /* continue in motion message... */

    case JM_MOTION:
      if (widget->hasCapture()) {
	switch (curve_editor->status) {

	  case STATUS_SCROLLING: {
	    JWidget view = jwidget_get_view(widget);
	    JRect vp = jview_get_viewport_position(view);
	    int scroll_x, scroll_y;

	    jview_get_scroll(view, &scroll_x, &scroll_y);
	    jview_set_scroll(view,
			     scroll_x+jmouse_x(1)-jmouse_x(0),
			     scroll_y+jmouse_y(1)-jmouse_y(0));

	    jmouse_control_infinite_scroll(vp);
	    jrect_free(vp);
	    break;
	  }

/* 	  case STATUS_SCALING: { */
/* 	    JID view_id = jwidget_get_view(widget); */
/* 	    JRect vp = jview_get_viewport_pos(view_id); */
/* 	    int scroll_x, scroll_y; */

/* 	    jview_get_scroll(view_id, &scroll_x, &scroll_y); */
/* 	    jview_update(view_id); */
/* 	    jview_set_scroll(view_id, */
/* 				scroll_x-(vp.x+vp.w/2), */
/* 				scroll_y-(vp.y+vp.h/2)); */

/* 	    jmouse_control_infinite_scroll(vp.x, vp.y, vp.w, vp.h); */
/* 	    break; */
/* 	  } */

	  case STATUS_MOVING_POINT:
	    if (curve_editor->edit_point) {
/* 	      int old_x = *curve_editor->edit_x; */
/* 	      int old_y = *curve_editor->edit_y; */
/* 	      int offset_x, offset_y; */

	      *curve_editor->edit_x = SCR2EDIT_X(msg->mouse.x);
	      *curve_editor->edit_y = SCR2EDIT_Y(msg->mouse.y);

	      *curve_editor->edit_x = MID(curve_editor->x1,
					  *curve_editor->edit_x,
					  curve_editor->x2);

	      *curve_editor->edit_y = MID(curve_editor->y1,
					  *curve_editor->edit_y,
					  curve_editor->y2);

/* 	      if (curve_editor->edit_x == &curve_editor->edit_key->x && */
/* 		  curve_editor->edit_y == &curve_editor->edit_key->y) { */
/* 		offset_x = (*curve_editor->edit_x) - old_x; */
/* 		offset_y = (*curve_editor->edit_y) - old_y; */

/* 		curve_editor->edit_key->px += offset_x; */
/* 		curve_editor->edit_key->py += offset_y; */
/* 		curve_editor->edit_key->nx += offset_x; */
/* 		curve_editor->edit_key->ny += offset_y; */
/* 	      } */

	      /* TODO this should be optional */
	      jwidget_emit_signal(widget, SIGNAL_CURVE_EDITOR_CHANGE);

	      jwidget_dirty(widget);
	    }
	    break;
	}

	return true;
      }
#if 0				/* TODO */
      /* if the mouse move above a curve_editor, the focus change to
	 this widget immediately */
      else if (!jwidget_has_focus(widget)) {
	jmanager_set_focus(widget);
      }
#endif
      break;

    case JM_BUTTONRELEASED:
      if (widget->hasCapture()) {
	widget->releaseMouse();

	switch (curve_editor->status) {

	  case STATUS_SCROLLING:
	    jmouse_set_cursor(JI_CURSOR_NORMAL);
	    break;

/* 	  case STATUS_SCALING: */
/* 	    jmouse_set_cursor(JI_CURSOR_NORMAL); */
/* 	    break; */

	  case STATUS_MOVING_POINT:
	    jmouse_set_cursor(JI_CURSOR_NORMAL);
	    jwidget_emit_signal(widget, SIGNAL_CURVE_EDITOR_CHANGE);

	    curve_editor->edit_point = NULL;
	    jwidget_dirty(widget);
	    break;
	}

	curve_editor->status = STATUS_STANDBY;
	return true;
      }
      break;
  }

  return false;
}

static CurvePoint* curve_editor_get_more_close_point(JWidget widget,
						     int x, int y,
						     int **edit_x,
						     int **edit_y)
{
#define CALCDIST(xx, yy)				\
  dx = point->xx-x;					\
  dy = point->yy-y;					\
  dist = std::sqrt(static_cast<double>(dx*dx + dy*dy));	\
							\
  if (!point_found || dist <= dist_min) {		\
    point_found = point;				\
    dist_min = dist;					\
							\
    if (edit_x) *edit_x = &point->xx;			\
    if (edit_y) *edit_y = &point->yy;			\
  }

  CurveEditor* curve_editor = curve_editor_data(widget);
  CurvePoint* point;
  CurvePoint* point_found = NULL;
  JLink link;
  int dx, dy;
  double dist, dist_min = 0;

  JI_LIST_FOR_EACH(curve_editor->curve->points, link) {
    point = reinterpret_cast<CurvePoint*>(link->data);
    CALCDIST(x, y);
  }

/*   if (curve_editor->curve->union_type == PROP_SPLINE && edit_x && edit_y) { */
/*     for (it=curve_editor->curve->keys; it; it=it->next) { */
/*       key = it->data; */

/*       if (it->prev) { */
/* 	CALCDIST(px, py); */
/*       } */

/*       if (it->next) { */
/* 	CALCDIST(nx, ny); */
/*       } */
/*     } */
/*   } */

  return point_found;
}

static int edit_node_manual(CurvePoint* point)
{
  JWidget entry_x, entry_y, button_ok;
  CurvePoint point_copy = *point;
  int res;

  FramePtr window(load_widget("color_curve.xml", "point_properties"));

  entry_x = jwidget_find_name(window, "x");
  entry_y = jwidget_find_name(window, "y");
  button_ok = jwidget_find_name(window, "button_ok");

  entry_x->setTextf("%d", point->x);
  entry_y->setTextf("%d", point->y);

  window->open_window_fg();

  if (window->get_killer() == button_ok) {
    point->x = entry_x->getTextDouble();
    point->y = entry_y->getTextDouble();
    res = true;
  }
  else {
    point->x = point_copy.x;
    point->y = point_copy.y;
    res = false;
  }

  return res;
}
