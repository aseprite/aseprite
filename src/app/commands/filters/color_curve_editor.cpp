// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/commands/filters/color_curve_editor.h"

#include "filters/color_curve.h"
#include "ui/alert.h"
#include "ui/entry.h"
#include "ui/manager.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/scale.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/theme.h"
#include "ui/view.h"
#include "ui/widget.h"
#include "ui/window.h"

#include "color_curve_point.xml.h"

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
  , m_hotPoint(nullptr)
  , m_editPoint(nullptr)
{
  setFocusStop(true);
  setDoubleBuffered(true);

  setBorder(gfx::Border(1));
  setChildSpacing(0);

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
          addPoint(screenToView(get_mouse_position()));
          break;
        }

        case kKeyDel: {
          gfx::Point* point = getClosestPoint(screenToView(get_mouse_position()));
          if (point)
            removePoint(point);
          break;
        }

        default:
          return false;
      }
      return true;
    }

    case kMouseDownMessage: {
      gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
      gfx::Point viewPos = screenToView(mousePos);
      m_editPoint = getClosestPoint(viewPos);

      if (!m_editPoint) {
        addPoint(viewPos);

        invalidate();
        CurveEditorChange();
        break;
      }

      // Show manual-entry dialog
      if (static_cast<MouseMessage*>(msg)->right()) {
        invalidate();
        flushRedraw();

        if (editNodeManually(*m_editPoint))
          CurveEditorChange();

        m_hotPoint = nullptr;
        m_editPoint = nullptr;
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

    case kMouseMoveMessage: {
      gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
      gfx::Point* oldHotPoint = m_hotPoint;
      m_hotPoint = getClosestPoint(screenToView(mousePos));

      switch (m_status) {

        case STATUS_STANDBY:
          if (!m_hotPoint || m_hotPoint != oldHotPoint)
            invalidate();
          break;

        case STATUS_MOVING_POINT:
          if (m_editPoint) {
            gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
            *m_editPoint = screenToView(mousePos);
            m_editPoint->x = MID(m_viewBounds.x, m_editPoint->x, m_viewBounds.x+m_viewBounds.w-1);
            m_editPoint->y = MID(m_viewBounds.y, m_editPoint->y, m_viewBounds.y+m_viewBounds.h-1);

            // TODO this should be optional
            CurveEditorChange();

            invalidate();
            return true;
          }
          break;
      }
      break;
    }

    case kMouseUpMessage:
      if (hasCapture()) {
        releaseMouse();

        switch (m_status) {

          case STATUS_MOVING_POINT:
            ui::set_mouse_cursor(kArrowCursor);
            CurveEditorChange();
            m_hotPoint = nullptr;
            m_editPoint = nullptr;
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

void ColorCurveEditor::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(1 + border().width(),
                           1 + border().height()));
}

void ColorCurveEditor::onPaint(ui::PaintEvent& ev)
{
  ui::Graphics* g = ev.graphics();
  gfx::Rect rc = clientBounds();
  gfx::Rect client = clientChildrenBounds();
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

    gfx::Rect box(0, 0, 5*guiscale(), 5*guiscale());
    box.offset(pt.x-box.w/2, pt.y-box.h/2);

    g->drawRect(gfx::rgba(0, 0, 255), box);

    if (m_editPoint == &point) {
      box.enlarge(4*guiscale());
      g->drawRect(gfx::rgba(255, 255, 0), box);
    }
    else if (m_hotPoint == &point) {
      box.enlarge(2*guiscale());
      g->drawRect(gfx::rgba(255, 255, 0), box);
    }
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

    if (dist < 16*guiscale() &&
        (!point_found || dist <= dist_min)) {
      point_found = &point;
      dist_min = dist;
    }
  }

  return point_found;
}

bool ColorCurveEditor::editNodeManually(gfx::Point& viewPt)
{
  gfx::Point point_copy = viewPt;

  app::gen::ColorCurvePoint window;
  window.x()->setTextf("%d", viewPt.x);
  window.y()->setTextf("%d", viewPt.y);

  window.openWindowInForeground();

  if (window.closer() == window.ok()) {
    viewPt.x = window.x()->textInt();
    viewPt.y = window.y()->textInt();
    viewPt.x = MID(0, viewPt.x, 255);
    viewPt.y = MID(0, viewPt.y, 255);
    return true;
  }
  else if (window.closer() == window.deleteButton()) {
    removePoint(&viewPt);
    return true;
  }
  else {
    viewPt = point_copy;
    return false;
  }
}

gfx::Point ColorCurveEditor::viewToClient(const gfx::Point& viewPt)
{
  gfx::Rect client = clientChildrenBounds();
  return gfx::Point(
    client.x + client.w * (viewPt.x - m_viewBounds.x) / m_viewBounds.w,
    client.y + client.h-1 - (client.h-1) * (viewPt.y - m_viewBounds.y) / m_viewBounds.h);
}

gfx::Point ColorCurveEditor::screenToView(const gfx::Point& screenPt)
{
  return clientToView(screenPt - bounds().origin());
}

gfx::Point ColorCurveEditor::clientToView(const gfx::Point& clientPt)
{
  gfx::Rect client = clientChildrenBounds();
  return gfx::Point(
    m_viewBounds.x + m_viewBounds.w * (clientPt.x - client.x) / client.w,
    m_viewBounds.y + m_viewBounds.h-1 - (m_viewBounds.h-1) * (clientPt.y - client.y) / client.h);
}

void ColorCurveEditor::addPoint(const gfx::Point& viewPoint)
{
  // TODO Undo history
  m_curve->addPoint(viewPoint);

  invalidate();
  CurveEditorChange();
}

void ColorCurveEditor::removePoint(gfx::Point* viewPoint)
{
  // TODO Undo history
  m_curve->removePoint(*viewPoint);

  m_hotPoint = nullptr;
  m_editPoint = nullptr;

  invalidate();
  CurveEditorChange();
}

} // namespace app
