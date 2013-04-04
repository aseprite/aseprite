/* ASEPRITE
 * Copyright (C) 2001-2013  David Capello
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

#include "app/find_widget.h"
#include "app/load_widget.h"
#include "commands/filters/color_curve_editor.h"
#include "filters/color_curve.h"
#include "ui/alert.h"
#include "ui/entry.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/preferred_size_event.h"
#include "ui/rect.h"
#include "ui/system.h"
#include "ui/view.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <allegro.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#define SCR2EDIT_X(xpos)                                        \
  (m_x1 +                                                       \
   ((m_x2 - m_x1 + 1)                                           \
    * ((xpos) - rc->x1 - border_width.l)                        \
    / (jrect_w(rc) - border_width.l - border_width.r)))

#define SCR2EDIT_Y(ypos)                                        \
  (m_y1 +                                                       \
   ((m_y2 - m_y1 + 1)                                           \
    * ((jrect_h(rc) - border_width.t - border_width.b)          \
       - ((ypos) - rc->y1 - border_width.t))                    \
    / (jrect_h(rc) - border_width.t - border_width.b)))

#define EDIT2SCR_X(xpos)                                        \
  (rc->x1 + border_width.l                                      \
   + ((jrect_w(rc) - border_width.l - border_width.r)           \
      * ((xpos) - m_x1)                                         \
      / (m_x2 - m_x1 + 1)))

#define EDIT2SCR_Y(ypos)                                        \
  (rc->y1                                                       \
   + (jrect_h(rc) - border_width.t - border_width.b)            \
   - ((jrect_h(rc) - border_width.t - border_width.b)           \
      * ((ypos) - m_y1)                                         \
      / (m_y2 - m_y1 + 1)))

enum {
  STATUS_STANDBY,
  STATUS_MOVING_POINT,
  STATUS_SCROLLING,
  STATUS_SCALING,
};

using namespace ui;

ColorCurveEditor::ColorCurveEditor(ColorCurve* curve, int x1, int y1, int x2, int y2)
  : Widget(kGenericWidget)
  , m_curve(curve)
{
  this->setFocusStop(true);

  border_width.l = border_width.r = 1;
  border_width.t = border_width.b = 1;
  child_spacing = 0;

  m_curve = curve;
  m_x1 = x1;
  m_y1 = y1;
  m_x2 = x2;
  m_y2 = y2;
  m_status = STATUS_STANDBY;
  m_editPoint = NULL;

  /* TODO */
  /* m_curve->type = CURVE_SPLINE; */
}

bool ColorCurveEditor::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_KEYPRESSED: {
      switch (msg->key.scancode) {

        case KEY_INSERT: {
          int x = SCR2EDIT_X(jmouse_x(0));
          int y = SCR2EDIT_Y(jmouse_y(0));

          // TODO undo?
          m_curve->addPoint(gfx::Point(x, y));

          invalidate();
          CurveEditorChange();
          break;
        }

        case KEY_DEL: {
          gfx::Point* point = getClosestPoint(SCR2EDIT_X(jmouse_x(0)),
                                              SCR2EDIT_Y(jmouse_y(0)),
                                              NULL, NULL);

          // TODO undo?
          if (point) {
            m_curve->removePoint(*point);
            m_editPoint = NULL;

            invalidate();
            CurveEditorChange();
          }
          break;
        }

        default:
          return false;
      }
      return true;
    }

    case JM_DRAW: {
      BITMAP *bmp;
      int x, y, u;

      bmp = create_bitmap(jrect_w(rc), jrect_h(rc));
      clear_to_color(bmp, makecol (0, 0, 0));

      // Draw border
      rect(bmp, 0, 0, bmp->w-1, bmp->h-1, makecol (255, 255, 0));

      // Draw guides
      for (x=1; x<=3; x++)
        vline(bmp, x*bmp->w/4, 1, bmp->h-2, makecol (128, 128, 0));

      for (y=1; y<=3; y++)
        hline(bmp, 1, y*bmp->h/4, bmp->w-2, makecol (128, 128, 0));

      // Get curve values
      std::vector<int> values(m_x2-m_x1+1);
      m_curve->getValues(m_x1, m_x2, values);

      // Draw curve
      for (x=border_width.l;
           x<jrect_w(rc)-border_width.r; x++) {
        u = SCR2EDIT_X(rc->x1+x);
        u = MID(m_x1, u, m_x2);

        y = values[u - m_x1];
        y = MID(m_y1, y, m_y2);

        putpixel(bmp, x, EDIT2SCR_Y(y)-rc->y1,
                 makecol(255, 255, 255));
      }

      // Draw nodes
      for (ColorCurve::iterator it = m_curve->begin(), end = m_curve->end(); it != end; ++it) {
        const gfx::Point& point = *it;

        x = EDIT2SCR_X(point.x) - rc->x1;
        y = EDIT2SCR_Y(point.y) - rc->y1;

        rect(bmp, x-2, y-2, x+2, y+2,
             m_editPoint == &point ? makecol(255, 255, 0):
                                     makecol(0, 0, 255));
      }

      // Blit to screen
      blit(bmp, ji_screen, 0, 0, rc->x1, rc->y1, bmp->w, bmp->h);
      destroy_bitmap(bmp);
      return true;
    }

    case JM_BUTTONPRESSED:
      // Change scroll
      if (msg->any.shifts & KB_SHIFT_FLAG) {
        m_status = STATUS_SCROLLING;
        jmouse_set_cursor(kScrollCursor);
      }
      /* scaling */
/*       else if (msg->shifts & KB_CTRL_FLAG) { */
/*      m_status = STATUS_SCALING; */
/*      jmouse_set_cursor(kScrollCursor); */
/*       } */
      // Show manual-entry dialog
      else if (msg->mouse.right) {
        m_editPoint = getClosestPoint(SCR2EDIT_X(msg->mouse.x),
                                      SCR2EDIT_Y(msg->mouse.y),
                                      NULL, NULL);
        if (m_editPoint) {
          invalidate();
          this->flushRedraw();

          if (editNodeManually(*m_editPoint))
            CurveEditorChange();

          m_editPoint = NULL;
          invalidate();
        }

        return true;
      }
      // Edit node
      else {
        m_editPoint = getClosestPoint(SCR2EDIT_X(msg->mouse.x),
                                      SCR2EDIT_Y(msg->mouse.y),
                                      &m_editX,
                                      &m_editY);

        m_status = STATUS_MOVING_POINT;
        jmouse_set_cursor(kHandCursor);
      }

      captureMouse();
      // continue in motion message...

    case JM_MOTION:
      if (hasCapture()) {
        switch (m_status) {

          case STATUS_SCROLLING: {
            View* view = View::getView(this);
            gfx::Rect vp = view->getViewportBounds();
            gfx::Point scroll = view->getViewScroll();

            scroll.x += jmouse_x(1)-jmouse_x(0);
            scroll.y += jmouse_y(1)-jmouse_y(0);

            view->setViewScroll(scroll);

            jmouse_control_infinite_scroll(vp);
            break;
          }

          case STATUS_MOVING_POINT:
            if (m_editPoint) {
              *m_editX = SCR2EDIT_X(msg->mouse.x);
              *m_editY = SCR2EDIT_Y(msg->mouse.y);
              *m_editX = MID(m_x1, *m_editX, m_x2);
              *m_editY = MID(m_y1, *m_editY, m_y2);

              // TODO this should be optional
              CurveEditorChange();

              invalidate();
            }
            break;
        }

        return true;
      }
#if 0                           // TODO
      // If the mouse move above a curve_editor, the focus change to
      // this widget immediately
      else if (!jwidget_has_focus(this)) {
        jmanager_set_focus(this);
      }
#endif
      break;

    case JM_BUTTONRELEASED:
      if (hasCapture()) {
        releaseMouse();

        switch (m_status) {

          case STATUS_SCROLLING:
            jmouse_set_cursor(kArrowCursor);
            break;

/*        case STATUS_SCALING: */
/*          jmouse_set_cursor(kArrowCursor); */
/*          break; */

          case STATUS_MOVING_POINT:
            jmouse_set_cursor(kArrowCursor);
            CurveEditorChange();

            m_editPoint = NULL;
            invalidate();
            break;
        }

        m_status = STATUS_STANDBY;
        return true;
      }
      break;
  }

  return Widget::onProcessMessage(msg);
}

void ColorCurveEditor::onPreferredSize(PreferredSizeEvent& ev)
{
  ev.setPreferredSize(gfx::Size(border_width.l + 1 + border_width.r,
                                border_width.t + 1 + border_width.b));
}

gfx::Point* ColorCurveEditor::getClosestPoint(int x, int y, int** edit_x, int** edit_y)
{
#define CALCDIST(xx, yy)                                \
  dx = point->xx-x;                                     \
  dy = point->yy-y;                                     \
  dist = std::sqrt(static_cast<double>(dx*dx + dy*dy)); \
                                                        \
  if (!point_found || dist <= dist_min) {               \
    point_found = point;                                \
    dist_min = dist;                                    \
                                                        \
    if (edit_x) *edit_x = &point->xx;                   \
    if (edit_y) *edit_y = &point->yy;                   \
  }

  gfx::Point* point;
  gfx::Point* point_found = NULL;
  int dx, dy;
  double dist, dist_min = 0;

  for (ColorCurve::iterator it = m_curve->begin(), end = m_curve->end(); it != end; ++it) {
    point = &(*it);
    CALCDIST(x, y);
  }

  return point_found;
}

int ColorCurveEditor::editNodeManually(gfx::Point& point)
{
  Widget* entry_x, *entry_y, *button_ok;
  gfx::Point point_copy = point;
  int res;

  UniquePtr<Window> window(app::load_widget<Window>("color_curve.xml", "point_properties"));

  entry_x = window->findChild("x");
  entry_y = window->findChild("y");
  button_ok = window->findChild("button_ok");

  entry_x->setTextf("%d", point.x);
  entry_y->setTextf("%d", point.y);

  window->openWindowInForeground();

  if (window->getKiller() == button_ok) {
    point.x = entry_x->getTextDouble();
    point.y = entry_y->getTextDouble();
    res = true;
  }
  else {
    point = point_copy;
    res = false;
  }

  return res;
}
