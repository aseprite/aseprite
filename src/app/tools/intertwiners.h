// Aseprite
// Copyright (C) 2018  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "base/pi.h"

namespace app {
namespace tools {

class IntertwineNone : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    for (int c=0; c<stroke.size(); ++c)
      doPointshapePoint(stroke[c].x, stroke[c].y, loop);
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
    if (!stroke.empty())
      doPointshapePoint(stroke[0].x, stroke[0].y, loop);
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    joinStroke(loop, stroke);
  }
};

class IntertwineAsLines : public Intertwine {
public:
  bool snapByAngle() override { return true; }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
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

        doPointshapeLine(x1, y1, x2, y2, loop);
      }
    }

    // Closed shape (polygon outline)
    if (loop->getFilled()) {
      doPointshapeLine(stroke[0].x, stroke[0].y,
                       stroke[stroke.size()-1].x,
                       stroke[stroke.size()-1].y, loop);
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.size() < 3) {
      joinStroke(loop, stroke);
      return;
    }

    // Contour
    joinStroke(loop, stroke);

    // Fill content
    doc::algorithm::polygon(stroke.size(), (const int*)&stroke[0], loop, (AlgoHLine)doPointshapeHline);
  }

};

class IntertwineAsRectangles : public Intertwine {
public:

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
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
        int y;

        if (x1 > x2) std::swap(x1, x2);
        if (y1 > y2) std::swap(y1, y2);

        const double angle = loop->getController()->getShapeAngle();
        if (ABS(angle) < 0.001) {
          doPointshapeLine(x1, y1, x2, y1, loop);
          doPointshapeLine(x1, y2, x2, y2, loop);

          for (y=y1; y<=y2; y++) {
            doPointshapePoint(x1, y, loop);
            doPointshapePoint(x2, y, loop);
          }
        }
        else {
          Stroke p = rotateRectangle(x1, y1, x2, y2, angle);
          int n = p.size();
          for (int i=0; i+1<n; ++i) {
            doPointshapeLine(p[i].x, p[i].y,
                             p[i+1].x, p[i+1].y, loop);
          }
          doPointshapeLine(p[n-1].x, p[n-1].y,
                           p[0].x, p[0].y, loop);
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
          doPointshapeLine(x1, y, x2, y, loop);
      }
      else {
        Stroke p = rotateRectangle(x1, y1, x2, y2, angle);
        doc::algorithm::polygon(
          p.size(), (const int*)&p[0],
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

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.size() == 0)
      return;

    for (int c=0; c<stroke.size(); c += 4) {
      if (stroke.size()-c == 1) {
        doPointshapePoint(stroke[c].x, stroke[c].y, loop);
      }
      else if (stroke.size()-c == 2) {
        doPointshapeLine(stroke[c].x, stroke[c].y,
                         stroke[c+1].x, stroke[c+1].y, loop);
      }
      else if (stroke.size()-c == 3) {
        algo_spline(stroke[c  ].x, stroke[c  ].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+2].x, stroke[c+2].y, loop,
                    (AlgoLine)doPointshapeLine);
      }
      else {
        algo_spline(stroke[c  ].x, stroke[c  ].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+2].x, stroke[c+2].y,
                    stroke[c+3].x, stroke[c+3].y, loop,
                    (AlgoLine)doPointshapeLine);
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    joinStroke(loop, stroke);
  }
};

class IntertwineAsPixelPerfect : public Intertwine {
  static void pixelPerfectLine(int x, int y, Stroke* stroke) {
    gfx::Point newPoint(x, y);
    if (stroke->empty() ||
        stroke->lastPoint() != newPoint) {
      stroke->addPoint(newPoint);
    }
  }

  Stroke m_pts;

public:
  // Useful for Shift+Ctrl+pencil to draw straight lines and snap
  // angle when "pixel perfect" is selected.
  bool snapByAngle() override { return true; }

  void prepareIntertwine() override {
    m_pts.reset();
  }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    // Required for LineFreehand controller in the first stage, when
    // we are drawing the line and the trace policy is "Last". Each
    // new joinStroke() is like a fresh start.  Without this fix, the
    // first stage on LineFreehand will draw a "star" like pattern
    // with lines from the first point to the last point.
    if (loop->getTracePolicy() == TracePolicy::Last)
      m_pts.reset();

    if (stroke.size() == 0)
      return;
    else if (m_pts.empty() && stroke.size() == 1) {
      m_pts = stroke;
    }
    else {
      for (int c=0; c+1<stroke.size(); ++c) {
        algo_line_continuous(
          stroke[c].x,
          stroke[c].y,
          stroke[c+1].x,
          stroke[c+1].y,
          (void*)&m_pts,
          (AlgoPixel)&IntertwineAsPixelPerfect::pixelPerfectLine);
      }
    }

    for (int c=0; c<m_pts.size(); ++c) {
      // We ignore a pixel that is between other two pixels in the
      // corner of a L-like shape.
      if (c > 0 && c+1 < m_pts.size()
        && (m_pts[c-1].x == m_pts[c].x || m_pts[c-1].y == m_pts[c].y)
        && (m_pts[c+1].x == m_pts[c].x || m_pts[c+1].y == m_pts[c].y)
        && m_pts[c-1].x != m_pts[c+1].x
        && m_pts[c-1].y != m_pts[c+1].y) {
        ++c;
      }

      doPointshapePoint(m_pts[c].x, m_pts[c].y, loop);
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() < 3) {
      joinStroke(loop, stroke);
      return;
    }

    // Contour
    joinStroke(loop, stroke);

    // Fill content
    doc::algorithm::polygon(stroke.size(), (const int*)&stroke[0], loop, (AlgoHLine)doPointshapeHline);
  }
};

} // namespace tools
} // namespace app
