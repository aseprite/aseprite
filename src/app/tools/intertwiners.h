// Aseprite
// Copyright (C) 2018-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "base/pi.h"
#include "doc/layer_tilemap.h"

namespace app { namespace tools {

struct LineData2 {
  Intertwine::LineData head;
  Stroke& output;
  LineData2(ToolLoop* loop, const Stroke::Pt& a, const Stroke::Pt& b, Stroke& output)
    : head(loop, a, b)
    , output(output)
  {
  }
};

static void addPointsWithoutDuplicatingLastOne(int x, int y, LineData2* data)
{
  data->head.doStep(x, y);
  if (data->output.empty() || data->output.lastPoint() != data->head.pt) {
    data->output.addPoint(data->head.pt);
  }
}

class IntertwineNone : public Intertwine {
public:
  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    for (int c = 0; c < stroke.size(); ++c)
      doPointshapeStrokePt(stroke[c], loop);
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override { joinStroke(loop, stroke); }
};

class IntertwineFirstPoint : public Intertwine {
public:
  // Snap angle because the angle between the first point and the last
  // point might be useful for the ink (e.g. the gradient ink)
  bool snapByAngle() override { return true; }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
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

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override { joinStroke(loop, stroke); }
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

  void prepareIntertwine(ToolLoop* loop) override
  {
    m_retainedTracePolicyLast = false;
    m_firstStroke = true;
  }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
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
      for (int c = 0; c + 1 < stroke.size(); ++c) {
        auto lineAlgo = getLineAlgo(loop, stroke[c], stroke[c + 1]);
        LineData2 lineData(loop, stroke[c], stroke[c + 1], pts);
        lineAlgo(stroke[c].x,
                 stroke[c].y,
                 stroke[c + 1].x,
                 stroke[c + 1].y,
                 (void*)&lineData,
                 (AlgoPixel)&addPointsWithoutDuplicatingLastOne);
      }

      // Don't draw the first point in freehand tools (this is to
      // avoid painting above the last pixel of a freehand stroke,
      // when we use Shift+click in the Pencil tool to continue the
      // old stroke).
      // TODO useful only in the case when brush size = 1px
      const int start =
        (loop->getController()->isFreehand() && (m_retainedTracePolicyLast || !m_firstStroke) ? 1 :
                                                                                                0);

      for (int c = start; c < pts.size(); ++c)
        doPointshapeStrokePt(pts[c], loop);

      // Closed shape (polygon outline)
      // Note: Contour tool was getting into the condition with no need, so
      // we add the && !isFreehand to detect this circunstance.
      // When this is missing, we have problems previewing the stroke of
      // contour tool, with brush type = kImageBrush with alpha content and
      // with not Pixel Perfect pencil mode.
      if (loop->getFilled() && !loop->getController()->isFreehand()) {
        doPointshapeLine(stroke[stroke.size() - 1], stroke[0], loop);
      }
    }
    m_firstStroke = false;
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
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
    doc::algorithm::polygon(v.size() / 2, &v[0], loop, (AlgoHLine)doPointshapeHline);
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
      for (int c = 0; c + 1 < stroke.size(); ++c) {
        // TODO fix this with strokes and dynamics
        int x1 = stroke[c].x;
        int y1 = stroke[c].y;
        int x2 = stroke[c + 1].x;
        int y2 = stroke[c + 1].y;
        int y;

        if (x1 > x2)
          std::swap(x1, x2);
        if (y1 > y2)
          std::swap(y1, y2);

        const double angle = loop->getController()->getShapeAngle();
        if (ABS(angle) < 0.001) {
          doPointshapeLineWithoutDynamics(x1, y1, x2, y1, loop);
          doPointshapeLineWithoutDynamics(x1, y2, x2, y2, loop);

          for (y = y1; y <= y2; y++) {
            doPointshapePoint(x1, y, loop);
            doPointshapePoint(x2, y, loop);
          }
        }
        else {
          Stroke p = rotateRectangle(x1, y1, x2, y2, angle);
          int n = p.size();
          for (int i = 0; i + 1 < n; ++i) {
            doPointshapeLine(p[i], p[i + 1], loop);
          }
          doPointshapeLine(p[n - 1], p[0], loop);
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

    for (int c = 0; c + 1 < stroke.size(); ++c) {
      int x1 = stroke[c].x;
      int y1 = stroke[c].y;
      int x2 = stroke[c + 1].x;
      int y2 = stroke[c + 1].y;
      int y;

      if (x1 > x2)
        std::swap(x1, x2);
      if (y1 > y2)
        std::swap(y1, y2);

      const double angle = loop->getController()->getShapeAngle();
      if (ABS(angle) < 0.001) {
        for (y = y1; y <= y2; y++)
          doPointshapeLineWithoutDynamics(x1, y, x2, y, loop);
      }
      else {
        Stroke p = rotateRectangle(x1, y1, x2, y2, angle);
        auto v = p.toXYInts();
        doc::algorithm::polygon(v.size() / 2, &v[0], loop, (AlgoHLine)doPointshapeHline);
      }
    }
  }

  gfx::Rect getStrokeBounds(ToolLoop* loop, const Stroke& stroke) override
  {
    gfx::Rect bounds = stroke.bounds();
    const double angle = loop->getController()->getShapeAngle();

    if (ABS(angle) > 0.001) {
      bounds = gfx::Rect();
      if (stroke.size() >= 2) {
        for (int c = 0; c + 1 < stroke.size(); ++c) {
          int x1 = stroke[c].x;
          int y1 = stroke[c].y;
          int x2 = stroke[c + 1].x;
          int y2 = stroke[c + 1].y;
          bounds |= rotateRectangle(x1, y1, x2, y2, angle).bounds();
        }
      }
    }

    return bounds;
  }

private:
  static Stroke rotateRectangle(int x1, int y1, int x2, int y2, double angle)
  {
    int cx = (x1 + x2) / 2;
    int cy = (y1 + y2) / 2;
    int a = (x2 - x1) / 2;
    int b = (y2 - y1) / 2;
    double s = -std::sin(angle);
    double c = std::cos(angle);

    Stroke stroke;
    stroke.addPoint(Point(cx - a * c - b * s, cy + a * s - b * c));
    stroke.addPoint(Point(cx + a * c - b * s, cy - a * s - b * c));
    stroke.addPoint(Point(cx + a * c + b * s, cy - a * s + b * c));
    stroke.addPoint(Point(cx - a * c + b * s, cy + a * s + b * c));
    return stroke;
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
      for (int c = 0; c + 1 < stroke.size(); ++c) {
        int x1 = stroke[c].x;
        int y1 = stroke[c].y;
        int x2 = stroke[c + 1].x;
        int y2 = stroke[c + 1].y;

        if (x1 > x2)
          std::swap(x1, x2);
        if (y1 > y2)
          std::swap(y1, y2);

        const double angle = loop->getController()->getShapeAngle();
        if (ABS(angle) < 0.001) {
          algo_ellipse(x1, y1, x2, y2, 0, 0, loop, (AlgoPixel)doPointshapePoint);
        }
        else {
          draw_rotated_ellipse((x1 + x2) / 2,
                               (y1 + y2) / 2,
                               ABS(x2 - x1) / 2,
                               ABS(y2 - y1) / 2,
                               angle,
                               loop,
                               (AlgoPixel)doPointshapePoint);
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

    for (int c = 0; c + 1 < stroke.size(); ++c) {
      int x1 = stroke[c].x;
      int y1 = stroke[c].y;
      int x2 = stroke[c + 1].x;
      int y2 = stroke[c + 1].y;

      if (x1 > x2)
        std::swap(x1, x2);
      if (y1 > y2)
        std::swap(y1, y2);

      const double angle = loop->getController()->getShapeAngle();
      if (ABS(angle) < 0.001) {
        algo_ellipsefill(x1, y1, x2, y2, 0, 0, loop, (AlgoHLine)doPointshapeHline);
      }
      else {
        fill_rotated_ellipse((x1 + x2) / 2,
                             (y1 + y2) / 2,
                             ABS(x2 - x1) / 2,
                             ABS(y2 - y1) / 2,
                             angle,
                             loop,
                             (AlgoHLine)doPointshapeHline);
      }
    }
  }

  gfx::Rect getStrokeBounds(ToolLoop* loop, const Stroke& stroke) override
  {
    gfx::Rect bounds = stroke.bounds();
    const double angle = loop->getController()->getShapeAngle();

    if (ABS(angle) > 0.001) {
      Point center = bounds.center();
      int a = bounds.w / 2.0 + 0.5;
      int b = bounds.h / 2.0 + 0.5;
      double xd = a * a;
      double yd = b * b;
      double s = std::sin(angle);
      double zd = (xd - yd) * s;

      a = std::sqrt(xd - zd * s) + 0.5;
      b = std::sqrt(yd + zd * s) + 0.5;

      bounds.x = center.x - a - 1;
      bounds.y = center.y - b - 1;
      bounds.w = 2 * a + 3;
      bounds.h = 2 * b + 3;
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

    for (int c = 0; c < stroke.size(); c += 4) {
      if (stroke.size() - c == 1) {
        doPointshapeStrokePt(stroke[c], loop);
      }
      else if (stroke.size() - c == 2) {
        doPointshapeLine(stroke[c], stroke[c + 1], loop);
      }
      else if (stroke.size() - c == 3) {
        algo_spline(stroke[c].x,
                    stroke[c].y,
                    stroke[c + 1].x,
                    stroke[c + 1].y,
                    stroke[c + 1].x,
                    stroke[c + 1].y,
                    stroke[c + 2].x,
                    stroke[c + 2].y,
                    loop,
                    (AlgoLine)doPointshapeLineWithoutDynamics);
      }
      else {
        algo_spline(stroke[c].x,
                    stroke[c].y,
                    stroke[c + 1].x,
                    stroke[c + 1].y,
                    stroke[c + 2].x,
                    stroke[c + 2].y,
                    stroke[c + 3].x,
                    stroke[c + 3].y,
                    loop,
                    (AlgoLine)doPointshapeLineWithoutDynamics);
      }
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override { joinStroke(loop, stroke); }
};

class IntertwineAsPixelPerfect : public Intertwine {
  // It was introduced to know if joinStroke function
  // was executed inmediatelly after a "Last" trace policy (i.e. after the
  // user confirms a line draw while he is holding down the SHIFT key), so
  // we have to ignore printing the first pixel of the line.
  bool m_retainedTracePolicyLast = false;
  Stroke m_pts;
  bool m_saveStrokeArea = false;

  // Helper struct to store an image's area that will be affected by the stroke
  // point at the specified position of the original image.
  struct SavedArea {
    doc::ImageRef img;
    // Original stroke point position.
    tools::Stroke::Pt pos;
    // Area of the original image that was saved into img.
    gfx::Rect r;
  };
  // Holds the areas saved by savePointshapeStrokePtArea method and restored by
  // restoreLastPts method.
  std::vector<SavedArea> m_savedAreas;
  // When a SavedArea is restored we add its Rect to this Region, then we use
  // this to expand the modified region when editing a tilemap manually.
  gfx::Region m_restoredRegion;
  // Last point index.
  int m_lastPti;

  // Temporal tileset with latest changes to be used by pixel perfect only when
  // modifying a tilemap in Manual mode.
  std::unique_ptr<Tileset> m_tempTileset;
  doc::Grid m_grid;
  doc::Grid m_dstGrid;
  doc::Grid m_celGrid;

public:
  // Useful for Shift+Ctrl+pencil to draw straight lines and snap
  // angle when "pixel perfect" is selected.
  bool snapByAngle() override { return true; }

  void prepareIntertwine(ToolLoop* loop) override
  {
    m_pts.reset();
    m_retainedTracePolicyLast = false;
    m_grid = m_dstGrid = m_celGrid = loop->getGrid();
    m_restoredRegion.clear();

    if (loop->getLayer()->isTilemap() && !loop->isTilemapMode() && loop->isManualTilesetMode()) {
      const Tileset* srcTileset = static_cast<LayerTilemap*>(loop->getLayer())->tileset();
      m_tempTileset.reset(Tileset::MakeCopyCopyingImages(srcTileset));

      // Grid to convert to dstImage coordinates
      m_dstGrid.origin(gfx::Point(0, 0));

      // Grid where the original cel is the origin
      m_celGrid.origin(loop->getCel()->position());
    }
    else {
      m_tempTileset.reset();
    }
  }

  void joinStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    // Required for LineFreehand controller in the first stage, when
    // we are drawing the line and the trace policy is "Last". Each
    // new joinStroke() is like a fresh start.  Without this fix, the
    // first stage on LineFreehand will draw a "star" like pattern
    // with lines from the first point to the last point.
    if (loop->getTracePolicy() == TracePolicy::Last) {
      m_retainedTracePolicyLast = true;
      m_pts.reset();
    }

    int thirdFromLastPt = 0, nextPt = 0;

    if (stroke.size() == 0)
      return;
    else if (stroke.size() == 1) {
      if (m_pts.empty())
        m_pts = stroke;

      m_saveStrokeArea = false;
      doPointshapeStrokePt(stroke[0], loop);

      return;
    }
    else {
      if (stroke.firstPoint() == stroke.lastPoint())
        return;

      nextPt = m_pts.size();
      thirdFromLastPt = (m_pts.size() > 2 ? m_pts.size() - 3 : m_pts.size() - 1);

      for (int c = 0; c + 1 < stroke.size(); ++c) {
        auto lineAlgo = getLineAlgo(loop, stroke[c], stroke[c + 1]);
        LineData2 lineData(loop, stroke[c], stroke[c + 1], m_pts);
        lineAlgo(stroke[c].x,
                 stroke[c].y,
                 stroke[c + 1].x,
                 stroke[c + 1].y,
                 (void*)&lineData,
                 (AlgoPixel)&addPointsWithoutDuplicatingLastOne);
      }
    }

    // For line brush type, the pixel-perfect will create gaps so we
    // avoid removing points
    if (loop->getBrush()->type() != kLineBrushType ||
        (loop->getDynamics().angle == tools::DynamicSensor::Static &&
         (loop->getBrush()->angle() == 0.0f || loop->getBrush()->angle() == 90.0f ||
          loop->getBrush()->angle() == 180.0f))) {
      for (int c = thirdFromLastPt; c < m_pts.size(); ++c) {
        // We ignore a pixel that is between other two pixels in the
        // corner of a L-like shape.
        if (c > 0 && c + 1 < m_pts.size() &&
            (m_pts[c - 1].x == m_pts[c].x || m_pts[c - 1].y == m_pts[c].y) &&
            (m_pts[c + 1].x == m_pts[c].x || m_pts[c + 1].y == m_pts[c].y) &&
            m_pts[c - 1].x != m_pts[c + 1].x && m_pts[c - 1].y != m_pts[c + 1].y) {
          restoreLastPts(loop, c, m_pts[c]);
          if (c == nextPt - 1)
            nextPt--;
          m_pts.erase(c);
        }
      }
    }

    for (int c = nextPt; c < m_pts.size(); ++c) {
      // We must ignore to print the first point of the line after
      // a joinStroke pass with a retained "Last" trace policy
      // (i.e. the user confirms draw a line while he is holding
      // the SHIFT key))
      if (c == 0 && m_retainedTracePolicyLast)
        continue;

      // For the last point we need to store the source image content at that
      // point so we can restore it when erasing a point because of
      // pixel-perfect. So we set the following flag to indicate this, and
      // use it in doTransformPoint.
      m_saveStrokeArea = (c == m_pts.size() - 1);
      if (m_saveStrokeArea) {
        clearPointshapeStrokePtAreas();
        setLastPtIndex(c);
      }
      doPointshapeStrokePt(m_pts[c], loop);
    }
  }

  void fillStroke(ToolLoop* loop, const Stroke& stroke) override
  {
    if (stroke.empty())
      return;

    // Fill content
    auto v = m_pts.toXYInts();
    doc::algorithm::polygon(v.size() / 2, &v[0], loop, (AlgoHLine)doPointshapeHline);
  }

  gfx::Region forceTilemapRegionToValidate() override { return m_restoredRegion; }

protected:
  void doTransformPoint(const Stroke::Pt& pt, ToolLoop* loop) override
  {
    if (m_saveStrokeArea)
      savePointshapeStrokePtArea(loop, pt);

    Intertwine::doTransformPoint(pt, loop);

    if (loop->getLayer()->isTilemap() && m_tempTileset)
      updateTempTileset(loop, pt);
  }

private:
  void clearPointshapeStrokePtAreas() { m_savedAreas.clear(); }

  void setLastPtIndex(const int pti) { m_lastPti = pti; }

  // Saves the destination image's area that will be updated by the point
  // passed. The idea is to have the state of the image (only the
  // portion modified by the stroke's point shape) before drawing the last
  // point of the stroke, then if that point has to be deleted by the
  // pixel-perfect algorithm, we can use this image to restore the image to the
  // state previous to the deletion. This method is used by
  // IntertwineAsPixelPerfect.joinStroke() method.
  void savePointshapeStrokePtArea(ToolLoop* loop, const tools::Stroke::Pt& pt)
  {
    gfx::Rect r;
    loop->getPointShape()->getModifiedArea(loop, pt.x, pt.y, r);

    gfx::Region rgn(r);
    // By wrapping the modified area's position when tiled mode is active, the
    // user can draw outside the canvas and still get the pixel-perfect
    // effect.
    loop->getTiledModeHelper().wrapPosition(rgn);
    loop->getTiledModeHelper().collapseRegionByTiledMode(rgn);

    for (auto a : rgn) {
      a.offset(-loop->getCelOrigin());

      if (m_tempTileset) {
        forEachTilePos(loop,
                       m_dstGrid.tilesInCanvasRegion(gfx::Region(a)),
                       [loop](const doc::ImageRef existentTileImage, const gfx::Point tilePos) {
                         loop->getDstImage()->copy(existentTileImage.get(),
                                                   gfx::Clip(tilePos.x,
                                                             tilePos.y,
                                                             0,
                                                             0,
                                                             existentTileImage->width(),
                                                             existentTileImage->height()));
                       });
      }

      ImageRef i(crop_image(loop->getDstImage(), a, loop->getDstImage()->maskColor()));
      m_savedAreas.push_back(SavedArea{ i, pt, a });
    }
  }

  // Takes the images saved by savePointshapeStrokePtArea and copies them to
  // the destination image. It restores the destination image because the
  // images in m_savedAreas are from previous states of the destination
  // image. This method is used by IntertwineAsPixelPerfect.joinStroke()
  // method.
  void restoreLastPts(ToolLoop* loop, const int pti, const tools::Stroke::Pt& pt)
  {
    if (m_savedAreas.empty() || pti != m_lastPti || m_savedAreas[0].pos != pt)
      return;

    m_restoredRegion.clear();

    tools::Stroke::Pt pos;
    for (int i = 0; i < m_savedAreas.size(); ++i) {
      loop->getDstImage()->copy(
        m_savedAreas[i].img.get(),
        gfx::Clip(m_savedAreas[i].r.origin(), m_savedAreas[i].img->bounds()));

      if (m_tempTileset) {
        auto r = m_savedAreas[i].r;
        forEachTilePos(
          loop,
          m_dstGrid.tilesInCanvasRegion(gfx::Region(r)),
          [this, i, r](const doc::ImageRef existentTileImage, const gfx::Point tilePos) {
            existentTileImage->copy(m_savedAreas[i].img.get(),
                                    gfx::Clip(r.x - tilePos.x, r.y - tilePos.y, 0, 0, r.w, r.h));
          });
      }

      m_restoredRegion |= gfx::Region(m_savedAreas[i].r);
    }

    m_restoredRegion.offset(loop->getCelOrigin());
  }

  void updateTempTileset(ToolLoop* loop, const tools::Stroke::Pt& pt)
  {
    ASSERT(m_tempTileset);

    gfx::Rect r;
    loop->getPointShape()->getModifiedArea(loop, pt.x, pt.y, r);

    r.offset(-loop->getCelOrigin());
    auto tilesPts = m_dstGrid.tilesInCanvasRegion(gfx::Region(r));
    forEachTilePos(loop,
                   tilesPts,
                   [loop, r](const doc::ImageRef existentTileImage, const gfx::Point tilePos) {
                     existentTileImage->copy(loop->getDstImage(),
                                             gfx::Clip(r.x - tilePos.x, r.y - tilePos.y, r));
                   });

    if (tilesPts.size() > 1) {
      forEachTilePos(loop,
                     tilesPts,
                     [loop](const doc::ImageRef existentTileImage, const gfx::Point tilePos) {
                       loop->getDstImage()->copy(existentTileImage.get(),
                                                 gfx::Clip(tilePos.x,
                                                           tilePos.y,
                                                           0,
                                                           0,
                                                           existentTileImage->width(),
                                                           existentTileImage->height()));
                     });
    }
  }

  // Loops over the points in tilesPts, and for each one calls the provided
  // processTempTileImage callback passing to it the corresponding temp tile
  // image and canvas position.
  void forEachTilePos(ToolLoop* loop,
                      const std::vector<gfx::Point>& tilesPts,
                      const std::function<void(const doc::ImageRef existentTileImage,
                                               const gfx::Point tilePos)>& processTempTileImage)
  {
    ASSERT(loop->getCel());
    if (!loop->getCel())
      return;

    const Image* tilemapImage = loop->getCel()->image();

    // Offset to convert a tile from dstImage coordinates to tilemap
    // image coordinates (to get the tile from the original tilemap)
    const gfx::Point tilePt0 = m_celGrid.canvasToTile(gfx::Point(0, 0));

    for (const gfx::Point& tilePt : tilesPts) {
      const gfx::Point tilePtInTilemap = tilePt0 + tilePt;

      // Ignore modifications outside the tilemap
      if (!tilemapImage->bounds().contains(tilePtInTilemap))
        continue;

      const doc::tile_t t = tilemapImage->getPixel(tilePtInTilemap.x, tilePtInTilemap.y);
      if (t == doc::notile)
        continue;

      const doc::tile_index ti = doc::tile_geti(t);
      const doc::ImageRef existentTileImage = m_tempTileset->get(ti);
      if (!existentTileImage)
        continue;

      const gfx::Point tilePosInDstImage = m_dstGrid.tileToCanvas(tilePt);
      processTempTileImage(existentTileImage, tilePosInDstImage);
    }
  }
};

}} // namespace app::tools
