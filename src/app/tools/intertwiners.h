// Aseprite
// Copyright (C) 2018-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "base/pi.h"

namespace app {
namespace tools {

struct LineData2 {
  Intertwine::LineData head;
  Stroke& output;
  LineData2(ToolLoop* loop, const Stroke::Pt& a, const Stroke::Pt& b,
            Stroke& output)
    : head(loop, a, b)
    , output(output) {
  }
};

static void addPointsWithoutDuplicatingLastOne(int x, int y, LineData2* data)
{
  data->head.doStep(x, y);
  if (data->output.empty() ||
      data->output.lastPoint() != data->head.pt) {
    data->output.addPoint(data->head.pt);
  }
}

class IntertwineNone : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    for (int c=0; c<stroke.size(); ++c)
      doPointshapeStrokePt(stroke[c], loop);
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    joinStroke(loop, stroke);
  }
};

class IntertwineFirstPoint : public Intertwine {
public:
  // Snap angle because the angle between the first point and the last
  // point might be useful for the ink (e.g. the gradient ink)
  bool snapByAngle() override { return true; }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.empty())
      return;

    gfx::Point mid;

    if (loop->getController()->isTwoPoints() &&
        (int(loop->getModifiers()) & int(ToolLoopModifiers::kFromCenter))) {
      int n = 0;
      for (auto& pt : stroke) {
        mid.x += pt.x;
        mid.y += pt.y;
        ++n;
      }
      mid.x /= n;
      mid.y /= n;
    }
    else {
      mid = stroke[0].toPoint();
    }

    doPointshapePoint(mid.x, mid.y, loop);
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    joinStroke(loop, stroke);
  }
};

class IntertwineAsLines : public Intertwine {
  // It was introduced to know if joinStroke function
  // was executed inmediatelly after a "Last" trace policy (i.e. after the
  // user confirms a line draw while he is holding down the SHIFT key), so
  // we have to ignore printing the first pixel of the line.
  bool m_retainedTracePolicyLast = false;

  // In freehand-like tools, on each mouse movement we draw only the
  // line between the last two mouse points in the stroke (the
  // complete stroke is not re-painted again), so we want to indicate
  // if this is the first stroke of all (the only one that needs the
  // first pixel of the line algorithm)
  bool m_firstStroke = true;

public:
  bool snapByAngle() override { return true; }

  void prepareIntertwine() override {
    m_retainedTracePolicyLast = false;
    m_firstStroke = true;
  }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    // Required for LineFreehand controller in the first stage, when
    // we are drawing the line and the trace policy is "Last". Each
    // new joinStroke() is like a fresh start.  Without this fix, the
    // first stage on LineFreehand will draw a "star" like pattern
    // with lines from the first point to the last point.
    if (loop->getTracePolicy() == TracePolicy::Last)
      m_retainedTracePolicyLast = true;

    if (stroke.size() == 0)
      return;
    else if (stroke.size() == 1) {
      doPointshapeStrokePt(stroke[0], loop);
    }
    else {
      Stroke pts;
      for (int c=0; c+1<stroke.size(); ++c) {
        auto lineAlgo = getLineAlgo(loop, stroke[c], stroke[c+1]);
        LineData2 lineData(loop, stroke[c], stroke[c+1], pts);
        lineAlgo(stroke[c].x, stroke[c].y,
                 stroke[c+1].x, stroke[c+1].y,
                 (void*)&lineData,
                 (AlgoPixel)&addPointsWithoutDuplicatingLastOne);
      }

      // Don't draw the first point in freehand tools (this is to
      // avoid painting above the last pixel of a freehand stroke,
      // when we use Shift+click in the Pencil tool to continue the
      // old stroke).
      // TODO useful only in the case when brush size = 1px
      const int start =
        (loop->getController()->isFreehand() &&
         (m_retainedTracePolicyLast || !m_firstStroke) ? 1: 0);

      for (int c=start; c<pts.size(); ++c)
        doPointshapeStrokePt(pts[c], loop);

      // Closed shape (polygon outline)
      // Note: Contour tool was getting into the condition with no need, so
      // we add the && !isFreehand to detect this circunstance.
      // When this is missing, we have problems previewing the stroke of
      // contour tool, with brush type = kImageBrush with alpha content and
      // with not Pixel Perfect pencil mode.
      if (loop->getFilled() && !loop->getController()->isFreehand()) {
        doPointshapeLine(stroke[stroke.size()-1], stroke[0], loop);
      }
    }
    m_firstStroke = false;
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
#if 0
    // We prefer to use doc::algorithm::polygon() directly instead of
    // joinStroke() for the simplest cases (i.e. stroke.size() < 3 is
    // one point, or a line with two points), because if we use
    // joinStroke(), we'll get some undesirable behaviors of the
    // Shift+click considerations. E.g. not drawing the first pixel
    // (or nothing at all) because it can been seen as the
    // continuation of the previous last point. An specific example is
    // when stroke[0] == stroke[1], joinStroke() assumes that it has
    // to draw a stroke with 2 pixels, but when the stroke is
    // converted to "pts", "pts" has just one point, then if the first
    // one has to be discarded no pixel is drawn.
    if (stroke.size() < 3) {
      joinStroke(loop, stroke);
      return;
    }
#endif

    // Fill content
    auto v = stroke.toXYInts();
    doc::algorithm::polygon(
      v.size()/2, &v[0],
      loop, (AlgoHLine)doPointshapeHline);
  }

};

class IntertwineAsRectangles : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() == 0)
      return;

    if (stroke.size() == 1) {
      doPointshapePoint(stroke[0].x, stroke[0].y, loop);
    }
    else if (stroke.size() >= 2) {
      for (int c=0; c+1<stroke.size(); ++c) {
        // TODO fix this with strokes and dynamics
        int x1 = stroke[c].x;
        int y1 = stroke[c].y;
        int x2 = stroke[c+1].x;
        int y2 = stroke[c+1].y;
        int y;

        if (x1 > x2) std::swap(x1, x2);
        if (y1 > y2) std::swap(y1, y2);

        const double angle = loop->getController()->getShapeAngle();
        if (ABS(angle) < 0.001) {
          doPointshapeLineWithoutDynamics(x1, y1, x2, y1, loop);
          doPointshapeLineWithoutDynamics(x1, y2, x2, y2, loop);

          for (y=y1; y<=y2; y++) {
            doPointshapePoint(x1, y, loop);
            doPointshapePoint(x2, y, loop);
          }
        }
        else {
          Stroke p = rotateRectangle(x1, y1, x2, y2, angle);
          int n = p.size();
          for (int i=0; i+1<n; ++i) {
            doPointshapeLine(p[i], p[i+1], loop);
          }
          doPointshapeLine(p[n-1], p[0], loop);
        }
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() < 2) {
      joinStroke(loop, stroke);
      return;
    }

    for (int c=0; c+1<stroke.size(); ++c) {
      int x1 = stroke[c].x;
      int y1 = stroke[c].y;
      int x2 = stroke[c+1].x;
      int y2 = stroke[c+1].y;
      int y;

      if (x1 > x2) std::swap(x1, x2);
      if (y1 > y2) std::swap(y1, y2);

      const double angle = loop->getController()->getShapeAngle();
      if (ABS(angle) < 0.001) {
        for (y=y1; y<=y2; y++)
          doPointshapeLineWithoutDynamics(x1, y, x2, y, loop);
      }
      else {
        Stroke p = rotateRectangle(x1, y1, x2, y2, angle);
        auto v = p.toXYInts();
        doc::algorithm::polygon(
          v.size()/2, &v[0],
          loop, (AlgoHLine)doPointshapeHline);
      }
    }
  }

  gfx::Rect getStrokeBounds(ToolLoop* loop, const Stroke& stroke) override {
    gfx::Rect bounds = stroke.bounds();
    const double angle = loop->getController()->getShapeAngle();

    if (ABS(angle) > 0.001) {
      bounds = gfx::Rect();
      if (stroke.size() >= 2) {
        for (int c=0; c+1<stroke.size(); ++c) {
          int x1 = stroke[c].x;
          int y1 = stroke[c].y;
          int x2 = stroke[c+1].x;
          int y2 = stroke[c+1].y;
          bounds |= rotateRectangle(x1, y1, x2, y2, angle).bounds();
        }
      }
    }

    return bounds;
  }

private:
  static Stroke rotateRectangle(int x1, int y1, int x2, int y2, double angle) {
    int cx = (x1+x2)/2;
    int cy = (y1+y2)/2;
    int a = (x2-x1)/2;
    int b = (y2-y1)/2;
    double s = -std::sin(angle);
    double c = std::cos(angle);

    Stroke stroke;
    stroke.addPoint(Point(cx-a*c-b*s, cy+a*s-b*c));
    stroke.addPoint(Point(cx+a*c-b*s, cy-a*s-b*c));
    stroke.addPoint(Point(cx+a*c+b*s, cy-a*s+b*c));
    stroke.addPoint(Point(cx-a*c+b*s, cy+a*s+b*c));
    return stroke;
  }

};

class IntertwineAsEllipses : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() == 0)
      return;

    if (stroke.size() == 1) {
      doPointshapePoint(stroke[0].x, stroke[0].y, loop);
    }
    else if (stroke.size() >= 2) {
      for (int c=0; c+1<stroke.size(); ++c) {
        int x1 = stroke[c].x;
        int y1 = stroke[c].y;
        int x2 = stroke[c+1].x;
        int y2 = stroke[c+1].y;

        if (x1 > x2) std::swap(x1, x2);
        if (y1 > y2) std::swap(y1, y2);

        const double angle = loop->getController()->getShapeAngle();
        if (ABS(angle) < 0.001) {
          algo_ellipse(x1, y1, x2, y2, loop, (AlgoPixel)doPointshapePoint);
        }
        else {
          draw_rotated_ellipse((x1+x2)/2, (y1+y2)/2,
                               ABS(x2-x1)/2,
                               ABS(y2-y1)/2,
                               angle,
                               loop, (AlgoPixel)doPointshapePoint);
        }
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() < 2) {
      joinStroke(loop, stroke);
      return;
    }

    for (int c=0; c+1<stroke.size(); ++c) {
      int x1 = stroke[c].x;
      int y1 = stroke[c].y;
      int x2 = stroke[c+1].x;
      int y2 = stroke[c+1].y;

      if (x1 > x2) std::swap(x1, x2);
      if (y1 > y2) std::swap(y1, y2);

      const double angle = loop->getController()->getShapeAngle();
      if (ABS(angle) < 0.001) {
        algo_ellipsefill(x1, y1, x2, y2, loop, (AlgoHLine)doPointshapeHline);
      }
      else {
        fill_rotated_ellipse((x1+x2)/2, (y1+y2)/2,
                             ABS(x2-x1)/2,
                             ABS(y2-y1)/2,
                             angle,
                             loop, (AlgoHLine)doPointshapeHline);
      }
    }
  }

  gfx::Rect getStrokeBounds(ToolLoop* loop, const Stroke& stroke) override {
    gfx::Rect bounds = stroke.bounds();
    const double angle = loop->getController()->getShapeAngle();

    if (ABS(angle) > 0.001) {
      Point center = bounds.center();
      int a = bounds.w/2.0 + 0.5;
      int b = bounds.h/2.0 + 0.5;
      double xd = a*a;
      double yd = b*b;
      double s = std::sin(angle);
      double zd = (xd-yd)*s;

      a = std::sqrt(xd-zd*s) + 0.5;
      b = std::sqrt(yd+zd*s) + 0.5;

      bounds.x = center.x-a-1;
      bounds.y = center.y-b-1;
      bounds.w = 2*a+3;
      bounds.h = 2*b+3;
    }
    else {
      ++bounds.w;
      ++bounds.h;
    }

    return bounds;
  }

};

class IntertwineAsBezier : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() == 0)
      return;

    for (int c=0; c<stroke.size(); c += 4) {
      if (stroke.size()-c == 1) {
        doPointshapeStrokePt(stroke[c], loop);
      }
      else if (stroke.size()-c == 2) {
        doPointshapeLine(stroke[c], stroke[c+1], loop);
      }
      else if (stroke.size()-c == 3) {
        algo_spline(stroke[c  ].x, stroke[c  ].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+2].x, stroke[c+2].y, loop,
                    (AlgoLine)doPointshapeLineWithoutDynamics);
      }
      else {
        algo_spline(stroke[c  ].x, stroke[c  ].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+2].x, stroke[c+2].y,
                    stroke[c+3].x, stroke[c+3].y, loop,
                    (AlgoLine)doPointshapeLineWithoutDynamics);
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    joinStroke(loop, stroke);
  }
};

class IntertwineAsPixelPerfect : public Intertwine {
  // It was introduced to know if joinStroke function
  // was executed inmediatelly after a "Last" trace policy (i.e. after the
  // user confirms a line draw while he is holding down the SHIFT key), so
  // we have to ignore printing the first pixel of the line.
  bool m_retainedTracePolicyLast = false;
  Stroke m_pts;

public:
  // Useful for Shift+Ctrl+pencil to draw straight lines and snap
  // angle when "pixel perfect" is selected.
  bool snapByAngle() override { return true; }

  void prepareIntertwine() override {
    m_pts.reset();
    m_retainedTracePolicyLast = false;
  }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    // Required for LineFreehand controller in the first stage, when
    // we are drawing the line and the trace policy is "Last". Each
    // new joinStroke() is like a fresh start.  Without this fix, the
    // first stage on LineFreehand will draw a "star" like pattern
    // with lines from the first point to the last point.
    if (loop->getTracePolicy() == TracePolicy::Last) {
      m_retainedTracePolicyLast = true;
      m_pts.reset();
    }

    if (stroke.size() == 0)
      return;
    else if (stroke.size() == 1) {
      if (m_pts.empty())
        m_pts = stroke;
      doPointshapeStrokePt(stroke[0], loop);
      return;
    }
    else {
      for (int c=0; c+1<stroke.size(); ++c) {
        auto lineAlgo = getLineAlgo(loop, stroke[c], stroke[c+1]);
        LineData2 lineData(loop, stroke[c], stroke[c+1], m_pts);
        lineAlgo(
          stroke[c].x,
          stroke[c].y,
          stroke[c+1].x,
          stroke[c+1].y,
          (void*)&lineData,
          (AlgoPixel)&addPointsWithoutDuplicatingLastOne);
      }
    }

    // For line brush type, the pixel-perfect will create gaps so we
    // avoid removing points
    if (loop->getBrush()->type() != kLineBrushType ||
        (loop->getDynamics().angle == tools::DynamicSensor::Static &&
         (loop->getBrush()->angle() == 0.0f ||
          loop->getBrush()->angle() == 90.0f ||
          loop->getBrush()->angle() == 180.0f))) {
      for (int c=0; c<m_pts.size(); ++c) {
        // We ignore a pixel that is between other two pixels in the
        // corner of a L-like shape.
        if (c > 0 && c+1 < m_pts.size()
            && (m_pts[c-1].x == m_pts[c].x || m_pts[c-1].y == m_pts[c].y)
            && (m_pts[c+1].x == m_pts[c].x || m_pts[c+1].y == m_pts[c].y)
            && m_pts[c-1].x != m_pts[c+1].x
            && m_pts[c-1].y != m_pts[c+1].y) {
          m_pts.erase(c);
        }
      }
    }

    for (int c=0; c<m_pts.size(); ++c) {
      // We must ignore to print the first point of the line after
      // a joinStroke pass with a retained "Last" trace policy
      // (i.e. the user confirms draw a line while he is holding
      // the SHIFT key))
      if (c == 0 && m_retainedTracePolicyLast)
        continue;
      doPointshapeStrokePt(m_pts[c], loop);
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() < 3) {
      joinStroke(loop, stroke);
      return;
    }

    // Fill content
    auto v = m_pts.toXYInts();
    doc::algorithm::polygon(
      v.size()/2, &v[0],
      loop, (AlgoHLine)doPointshapeHline);
  }
};

} // namespace tools
} // namespace app
