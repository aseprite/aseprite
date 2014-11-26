/* Aseprite
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/color_curve_editor.h"

#include "app/find_widget.h"
#include "app/load_widget.h"
#include "filters/color_curve.h"
#include "ui/alert.h"
#include "ui/entry.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/preferred_size_event.h"
#include "ui/system.h"
#include "ui/view.h"
#include "ui/widget.h"
#include "ui/window.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace app {

using namespace ui;
using namespace filters;

enum {
  STATUS_STANDBY,
  STATUS_MOVING_POINT,
  STATUS_SCALING,
};

ColorCurveEditor::ColorCurveEditor(ColorCurve* curve, const gfx::Rect& viewBounds)
  : Widget(kGenericWidget)
  , m_curve(curve)
  , m_viewBounds(viewBounds)
  , m_editPoint(NULL)
{
  setFocusStop(true);
  setDoubleBuffered(true);

  border_width.l = border_width.r = 1;
  border_width.t = border_width.b = 1;
  child_spacing = 0;

  m_status = STATUS_STANDBY;

  // TODO
  // m_curve->type = CURVE_SPLINE;
}

bool ColorCurveEditor::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kKeyDownMessage: {
      switch (static_cast<KeyMessage*>(msg)->scancode()) {

        case kKeyInsert: {
          // TODO undo?
          m_curve->addPoint(screenToView(get_mouse_position()));

          invalidate();
          CurveEditorChange();
          break;
        }

        case kKeyDel: {
          gfx::Point* point = getClosestPoint(screenToView(get_mouse_position()));

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

    case kMouseDownMessage: {
      gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
      m_editPoint = getClosestPoint(screenToView(mousePos));
      if (!m_editPoint)
        break;

      // Show manual-entry dialog
      if (static_cast<MouseMessage*>(msg)->right()) {
        invalidate();
        flushRedraw();

        if (editNodeManually(*m_editPoint))
          CurveEditorChange();

        m_editPoint = NULL;
        invalidate();
        return true;
      }
      // Edit node
      else {
        m_status = STATUS_MOVING_POINT;
        ui::set_mouse_cursor(kHandCursor);
      }

      captureMouse();

      // continue in motion message...
    }

    case kMouseMoveMessage:
      if (hasCapture()) {
        switch (m_status) {

          case STATUS_MOVING_POINT:
            if (m_editPoint) {
              gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
              *m_editPoint = screenToView(mousePos);
              m_editPoint->x = MID(m_viewBounds.x, m_editPoint->x, m_viewBounds.x+m_viewBounds.w-1);
              m_editPoint->y = MID(m_viewBounds.y, m_editPoint->y, m_viewBounds.y+m_viewBounds.h-1);

              // TODO this should be optional
              CurveEditorChange();

              invalidate();
            }
            break;

        }

        return true;
      }
      break;

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();

        switch (m_status) {

          case STATUS_MOVING_POINT:
            ui::set_mouse_cursor(kArrowCursor);
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

void ColorCurveEditor::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.getGraphics();
  gfx::Rect rc = getClientBounds();
  gfx::Rect client = getClientChildrenBounds();
  gfx::Point pt;
  int c;

  g->fillRect(gfx::rgba(0, 0, 0), rc);
  g->drawRect(gfx::rgba(255, 255, 0), rc);

  // Draw guides
  for (c=1; c<=3; c++)
    g->drawVLine(gfx::rgba(128, 128, 0), c*client.w/4, client.y, client.h);

  for (c=1; c<=3; c++)
    g->drawHLine(gfx::rgba(128, 128, 0), client.x, c*client.h/4, client.w);

  // Get curve values
  std::vector<int> values(m_viewBounds.w);
  m_curve->getValues(m_viewBounds.x, m_viewBounds.x+m_viewBounds.w-1, values);

  // Draw curve
  for (c = client.x; c < client.x+client.w; ++c) {
    pt = clientToView(gfx::Point(c, 0));
    pt.x = MID(m_viewBounds.x, pt.x, m_viewBounds.x+m_viewBounds.w-1);
    pt.y = values[pt.x - m_viewBounds.x];
    pt.y = MID(m_viewBounds.y, pt.y, m_viewBounds.y+m_viewBounds.h-1);
    pt = viewToClient(pt);

    g->putPixel(gfx::rgba(255, 255, 255), c, pt.y);
  }

  // Draw nodes
  for (const gfx::Point& point : *m_curve) {
    pt = viewToClient(point);
    g->drawRect(
      m_editPoint == &point ?
        gfx::rgba(255, 255, 0):
        gfx::rgba(0, 0, 255),
      gfx::Rect(pt.x-2, pt.y-2, 5, 5));
  }
}

gfx::Point* ColorCurveEditor::getClosestPoint(const gfx::Point& viewPt)
{
  gfx::Point* point_found = NULL;
  double dist_min = 0;

  for (gfx::Point& point : *m_curve) {
    int dx = point.x - viewPt.x;
    int dy = point.y - viewPt.y;
    double dist = std::sqrt(static_cast<double>(dx*dx + dy*dy));
 
    if (!point_found || dist <= dist_min) {
      point_found = &point;
      dist_min = dist;
    }
  }

  return point_found;
}

bool ColorCurveEditor::editNodeManually(gfx::Point& viewPt)
{
  Widget* entry_x, *entry_y, *button_ok;
  gfx::Point point_copy = viewPt;
  bool res;

  base::UniquePtr<Window> window(app::load_widget<Window>("color_curve.xml", "point_properties"));

  entry_x = window->findChild("x");
  entry_y = window->findChild("y");
  button_ok = window->findChild("button_ok");

  entry_x->setTextf("%d", viewPt.x);
  entry_y->setTextf("%d", viewPt.y);

  window->openWindowInForeground();

  if (window->getKiller() == button_ok) {
    viewPt.x = entry_x->getTextDouble();
    viewPt.y = entry_y->getTextDouble();
    res = true;
  }
  else {
    viewPt = point_copy;
    res = false;
  }

  return res;
}

gfx::Point ColorCurveEditor::viewToClient(const gfx::Point& viewPt)
{
  gfx::Rect client = getClientChildrenBounds();
  return gfx::Point(
    client.x + client.w * (viewPt.x - m_viewBounds.x) / m_viewBounds.w,
    client.y + client.h-1 - (client.h-1) * (viewPt.y - m_viewBounds.y) / m_viewBounds.h);
}

gfx::Point ColorCurveEditor::screenToView(const gfx::Point& screenPt)
{
  return clientToView(screenPt - getBounds().getOrigin());
}

gfx::Point ColorCurveEditor::clientToView(const gfx::Point& clientPt)
{
  gfx::Rect client = getClientChildrenBounds();
  return gfx::Point(
    m_viewBounds.x + m_viewBounds.w * (clientPt.x - client.x) / client.w,
    m_viewBounds.y + m_viewBounds.h-1 - (m_viewBounds.h-1) * (clientPt.y - client.y) / client.h);
}

} // namespace app
