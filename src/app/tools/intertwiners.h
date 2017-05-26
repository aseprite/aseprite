// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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

        algo_line(x1, y1, x2, y2, loop, (AlgoPixel)doPointshapePoint);
      }
    }

    // Closed shape (polygon outline)
    if (loop->getFilled()) {
      algo_line(stroke[0].x, stroke[0].y,
                stroke[stroke.size()-1].x,
                stroke[stroke.size()-1].y, loop, (AlgoPixel)doPointshapePoint);
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

        doPointshapeLine(x1, y1, x2, y1, loop);
        doPointshapeLine(x1, y2, x2, y2, loop);

        for (y=y1; y<=y2; y++) {
          doPointshapePoint(x1, y, loop);
          doPointshapePoint(x2, y, loop);
        }
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
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

      for (y=y1; y<=y2; y++)
        doPointshapeLine(x1, y, x2, y, loop);
    }
  }
};

class IntertwineAsEllipses : public Intertwine {
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

        if (x1 > x2) std::swap(x1, x2);
        if (y1 > y2) std::swap(y1, y2);

        algo_ellipse(x1, y1, x2, y2, loop, (AlgoPixel)doPointshapePoint);
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
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

      algo_ellipsefill(x1, y1, x2, y2, loop, (AlgoHLine)doPointshapeHline);
    }
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
        algo_line(stroke[c].x, stroke[c].y,
                  stroke[c+1].x, stroke[c+1].y, loop, (AlgoPixel)doPointshapePoint);
      }
      else if (stroke.size()-c == 3) {
        algo_spline(stroke[c  ].x, stroke[c  ].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+2].x, stroke[c+2].y, loop, (AlgoLine)doPointshapeLine);
      }
      else {
        algo_spline(stroke[c  ].x, stroke[c  ].y,
                    stroke[c+1].x, stroke[c+1].y,
                    stroke[c+2].x, stroke[c+2].y,
                    stroke[c+3].x, stroke[c+3].y, loop, (AlgoLine)doPointshapeLine);
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
  void prepareIntertwine() override {
    m_pts.reset();
  }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override {
    if (stroke.size() == 0)
      return;
    else if (m_pts.empty() && stroke.size() == 1) {
      m_pts = stroke;
    }
    else {
      for (int c=0; c+1<stroke.size(); ++c) {
        algo_line(
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
