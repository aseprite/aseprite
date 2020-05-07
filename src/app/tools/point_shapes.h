// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/wrap_point.h"

#include "app/tools/ink.h"
#include "render/gradient.h"

namespace app {
namespace tools {

class NonePointShape : public PointShape {
public:
  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override {
    // Do nothing
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override {
    // Do nothing
  }
};

class PixelPointShape : public PointShape {
public:
  bool isPixel() override { return true; }

  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override {
    loop->getInk()->prepareForPointShape(loop, true, pt.x, pt.y);
    doInkHline(pt.x, pt.y, pt.x, loop);
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override {
    area = Rect(x, y, 1, 1);
  }
};

class BrushPointShape : public PointShape {
  bool m_firstPoint;
  Brush* m_lastBrush;
  BrushType m_origBrushType;
  std::shared_ptr<CompressedImage> m_compressedImage;
  // For dynamics
  DynamicsOptions m_dynamics;
  bool m_useDynamics;
  bool m_hasDynamicGradient;
  color_t m_primaryColor;
  color_t m_secondaryColor;
  float m_lastGradientValue;

public:

  void preparePointShape(ToolLoop* loop) override {
    m_firstPoint = true;
    m_lastBrush = nullptr;
    m_origBrushType = loop->getBrush()->type();

    m_dynamics = loop->getDynamics();
    m_useDynamics = (m_dynamics.isDynamic() &&
                     // TODO support custom brushes in future versions
                     m_origBrushType != kImageBrushType);

    // For dynamic gradient
    m_hasDynamicGradient = (m_dynamics.gradient != DynamicSensor::Static);
    if (m_hasDynamicGradient &&
        m_dynamics.colorFromTo == ColorFromTo::FgToBg) {
      m_primaryColor = loop->getSecondaryColor();
      m_secondaryColor = loop->getPrimaryColor();
    }
    else {
      m_primaryColor = loop->getPrimaryColor();
      m_secondaryColor = loop->getSecondaryColor();
    }
    m_lastGradientValue = -1;
  }

  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override {
    int x = pt.x;
    int y = pt.y;

    Ink* ink = loop->getInk();
    Brush* brush = loop->getBrush();

    // Dynamics
    if (m_useDynamics) {
      // Dynamic gradient info
      if (m_hasDynamicGradient &&
          m_dynamics.ditheringMatrix.rows() == 1 &&
          m_dynamics.ditheringMatrix.cols() == 1) {
        color_t a = m_secondaryColor;
        color_t b = m_primaryColor;
        const float t = pt.gradient;
        const float ti = 1.0f - pt.gradient;
        switch (loop->sprite()->pixelFormat()) {
          case IMAGE_RGB:
            if (rgba_geta(a) == 0) a = b;
            else if (rgba_geta(b) == 0) b = a;
            a = doc::rgba(int(ti*rgba_getr(a) + t*rgba_getr(b)),
                          int(ti*rgba_getg(a) + t*rgba_getg(b)),
                          int(ti*rgba_getb(a) + t*rgba_getb(b)),
                          int(ti*rgba_geta(a) + t*rgba_geta(b)));
            break;
          case IMAGE_GRAYSCALE:
            if (graya_geta(a) == 0) a = b;
            else if (graya_geta(b) == 0) b = a;
            a = doc::graya(int(ti*graya_getv(a) + t*graya_getv(b)),
                           int(ti*graya_geta(a) + t*graya_geta(b)));
            break;
          case IMAGE_INDEXED: {
            int maskIndex = (loop->getLayer()->isBackground() ? -1: loop->sprite()->transparentColor());
            // Convert index to RGBA
            if (a == maskIndex) a = 0;
            else a = get_current_palette()->getEntry(a);
            if (b == maskIndex) b = 0;
            else b = get_current_palette()->getEntry(b);
            // Same as in RGBA gradient
            if (rgba_geta(a) == 0) a = b;
            else if (rgba_geta(b) == 0) b = a;
            a = doc::rgba(int(ti*rgba_getr(a) + t*rgba_getr(b)),
                          int(ti*rgba_getg(a) + t*rgba_getg(b)),
                          int(ti*rgba_getb(a) + t*rgba_getb(b)),
                          int(ti*rgba_geta(a) + t*rgba_geta(b)));
            // Convert RGBA to index
            a = loop->getRgbMap()->mapColor(rgba_getr(a),
                                            rgba_getg(a),
                                            rgba_getb(a),
                                            rgba_geta(a));
            break;
          }
        }
        loop->setPrimaryColor(a);
      }

      // Dynamic size and angle
      int size = base::clamp(int(pt.size), int(Brush::kMinBrushSize), int(Brush::kMaxBrushSize));
      int angle = base::clamp(int(pt.angle), -180, 180);
      if ((brush->size() != size) ||
          (brush->angle() != angle && m_origBrushType != kCircleBrushType) ||
          (m_hasDynamicGradient && pt.gradient != m_lastGradientValue)) {
        // TODO cache brushes
        BrushRef newBrush = std::make_shared<Brush>(
          m_origBrushType, size, angle);

        // Dynamic gradient with dithering
        bool prepareInk = false;
        if (m_hasDynamicGradient && !ink->isEraser() &&
            (m_dynamics.ditheringMatrix.rows() > 1 ||
             m_dynamics.ditheringMatrix.cols() > 1)) {
          convert_bitmap_brush_to_dithering_brush(
            newBrush.get(),
            loop->sprite()->pixelFormat(),
            m_dynamics.ditheringMatrix,
            pt.gradient,
            m_secondaryColor,
            m_primaryColor);
          prepareInk = true;
        }
        m_lastGradientValue = pt.gradient;

        loop->setBrush(newBrush);
        brush = loop->getBrush();

        if (prepareInk) {
          // Prepare ink for the new brush
          ink->prepareInk(loop);
        }
      }
    }

    // TODO cache compressed images (or remove them completelly)
    if (m_lastBrush != brush) {
      m_lastBrush = brush;
      m_compressedImage.reset(new CompressedImage(brush->image(),
                                                  brush->maskBitmap(),
                                                  false));
    }

    x += brush->bounds().x;
    y += brush->bounds().y;

    if (m_firstPoint) {
      if ((brush->type() == kImageBrushType) &&
          (brush->pattern() == BrushPattern::ALIGNED_TO_DST ||
           brush->pattern() == BrushPattern::PAINT_BRUSH)) {
        brush->setPatternOrigin(gfx::Point(x, y));
      }
    }
    else {
      if (brush->type() == kImageBrushType &&
          brush->pattern() == BrushPattern::PAINT_BRUSH) {
        brush->setPatternOrigin(gfx::Point(x, y));
      }
    }

    if (int(loop->getTiledMode()) & int(TiledMode::X_AXIS)) {
      int wrappedPatternOriginX = wrap_value(brush->patternOrigin().x, loop->sprite()->width()) % brush->bounds().w;
      brush->setPatternOrigin(gfx::Point(wrappedPatternOriginX, brush->patternOrigin().y));
      x = wrap_value(x, loop->sprite()->width());
    }
    if (int(loop->getTiledMode()) & int(TiledMode::Y_AXIS)) {
      int wrappedPatternOriginY = wrap_value(brush->patternOrigin().y, loop->sprite()->height()) % brush->bounds().h;
      brush->setPatternOrigin(gfx::Point(brush->patternOrigin().x, wrappedPatternOriginY));
      y = wrap_value(y, loop->sprite()->height());
    }

    ink->prepareForPointShape(loop, m_firstPoint, x, y);

    for (auto scanline : *m_compressedImage) {
      int u = x+scanline.x;
      ink->prepareVForPointShape(loop, y+scanline.y);
      doInkHline(u, y+scanline.y, u+scanline.w-1, loop);
    }
    m_firstPoint = false;
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override {
    area = loop->getBrush()->bounds();
    area.x += x;
    area.y += y;
  }

};

class FloodFillPointShape : public PointShape {
public:
  bool isFloodFill() override { return true; }

  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override {
    const doc::Image* srcImage = loop->getFloodFillSrcImage();
    gfx::Point wpt = wrap_point(loop->getTiledMode(),
                                gfx::Size(srcImage->width(),
                                          srcImage->height()),
                                pt.toPoint(), true);

    loop->getInk()->prepareForPointShape(loop, true, wpt.x, wpt.y);

    doc::algorithm::floodfill(
      srcImage,
      (loop->useMask() ? loop->getMask(): nullptr),
      wpt.x, wpt.y,
      floodfillBounds(loop, wpt.x, wpt.y),
      get_pixel(srcImage, wpt.x, wpt.y),
      loop->getTolerance(),
      loop->getContiguous(),
      loop->isPixelConnectivityEightConnected(),
      loop, (AlgoHLine)doInkHline);
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override {
    area = floodfillBounds(loop, x, y);
  }

private:
  gfx::Rect floodfillBounds(ToolLoop* loop, int x, int y) const {
    gfx::Rect bounds = loop->sprite()->bounds();
    bounds &= loop->getFloodFillSrcImage()->bounds();

    // Limit the flood-fill to the current tile if the grid is visible.
    if (loop->getStopAtGrid()) {
      gfx::Rect grid = loop->getGridBounds();
      if (!grid.isEmpty()) {
        div_t d, dx, dy;

        dx = div(grid.x, grid.w);
        dy = div(grid.y, grid.h);

        if (dx.rem > 0) dx.rem -= grid.w;
        if (dy.rem > 0) dy.rem -= grid.h;

        d = div(x-dx.rem, grid.w);
        x = dx.rem + d.quot*grid.w;

        d = div(y-dy.rem, grid.h);
        y = dy.rem + d.quot*grid.h;

        bounds = bounds.createIntersection(gfx::Rect(x, y, grid.w, grid.h));
      }
    }

    return bounds;
  }
};

class SprayPointShape : public PointShape {
  BrushPointShape m_subPointShape;
  float m_pointRemainder = 0;

public:

  bool isSpray() override { return true; }

  void preparePointShape(ToolLoop* loop) override {
    m_subPointShape.preparePointShape(loop);
  }

  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override {
    loop->getInk()->prepareForPointShape(loop, true, pt.x, pt.y);

    int spray_width = loop->getSprayWidth();
    int spray_speed = loop->getSpraySpeed();

    // The number of points to spray is proportional to the spraying area, and
    // we calculate it as a float to handle very low spray rates properly.
    float points_to_spray = (spray_width * spray_width / 4.0f) * spray_speed / 100.0f;

    // We add the fractional points from last time to get
    // the total number of points to paint this time.
    points_to_spray += m_pointRemainder;
    int integral_points = (int)points_to_spray;

    // Save any leftover fraction of a point for next time.
    m_pointRemainder = points_to_spray - integral_points;
    ASSERT(m_pointRemainder >= 0 && m_pointRemainder < 1.0f);

    fixmath::fixed angle, radius;

    for (int c=0; c<integral_points; c++) {

#if RAND_MAX <= 0xffff
      // In Windows, rand() has a RAND_MAX too small
      angle = fixmath::itofix(rand() * 255 / RAND_MAX);
      radius = fixmath::itofix(rand() * spray_width / RAND_MAX);
#else
      angle = rand();
      radius = rand() % fixmath::itofix(spray_width);
#endif

      Stroke::Pt pt2(pt);
      pt2.x += fixmath::fixtoi(fixmath::fixmul(radius, fixmath::fixcos(angle)));
      pt2.y += fixmath::fixtoi(fixmath::fixmul(radius, fixmath::fixsin(angle)));
      m_subPointShape.transformPoint(loop, pt2);
    }
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override {
    int spray_width = loop->getSprayWidth();
    Point p1(x-spray_width, y-spray_width);
    Point p2(x+spray_width, y+spray_width);

    Rect area1;
    Rect area2;
    m_subPointShape.getModifiedArea(loop, p1.x, p1.y, area1);
    m_subPointShape.getModifiedArea(loop, p2.x, p2.y, area2);

    area = area1.createUnion(area2);
  }
};

} // namespace tools
} // namespace app
