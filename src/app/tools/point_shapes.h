// Aseprite
// Copyright (C) 2019-2022  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#include "app/util/wrap_point.h"

#include "app/tools/ink.h"
#include "doc/algorithm/flip_image.h"
#include "render/gradient.h"

#include <array>
#include <memory>

namespace app { namespace tools {

class NonePointShape : public PointShape {
public:
  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override
  {
    // Do nothing
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override
  {
    // Do nothing
  }
};

class PixelPointShape : public PointShape {
public:
  bool isPixel() override { return true; }

  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override
  {
    loop->getInk()->prepareForPointShape(loop, true, pt.x, pt.y);
    doInkHline(pt.x, pt.y, pt.x, loop);
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override
  {
    area = Rect(x, y, 1, 1);
  }
};

class TilePointShape : public PointShape {
public:
  bool isPixel() override { return true; }
  bool isTile() override { return true; }

  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override
  {
    const doc::Grid& grid = loop->getGrid();
    gfx::Point newPos = grid.canvasToTile(pt.toPoint());

    loop->getInk()->prepareForPointShape(loop, true, newPos.x, newPos.y);
    doInkHline(newPos.x, newPos.y, newPos.x, loop);
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override
  {
    const doc::Grid& grid = loop->getGrid();
    area = grid.alignBounds(Rect(x, y, 1, 1));
  }
};

class BrushPointShape : public PointShape {
  bool m_firstPoint;
  Brush* m_lastBrush;
  BrushType m_origBrushType;
  std::array<std::shared_ptr<CompressedImage>, 4> m_compressedImages;
  // For dynamics
  DynamicsOptions m_dynamics;
  bool m_useDynamics;
  bool m_hasDynamicGradient;
  color_t m_primaryColor;
  color_t m_secondaryColor;
  float m_lastGradientValue;

public:
  void preparePointShape(ToolLoop* loop) override
  {
    m_firstPoint = true;
    m_lastBrush = nullptr;
    m_origBrushType = loop->getBrush()->type();

    m_dynamics = loop->getDynamics();
    m_useDynamics = (m_dynamics.isDynamic() &&
                     // TODO support custom brushes in future versions
                     m_origBrushType != kImageBrushType);

    // For dynamic gradient
    m_hasDynamicGradient = (m_dynamics.gradient != DynamicSensor::Static);
    if (m_hasDynamicGradient && m_dynamics.colorFromTo == ColorFromTo::FgToBg) {
      m_primaryColor = loop->getSecondaryColor();
      m_secondaryColor = loop->getPrimaryColor();
    }
    else {
      m_primaryColor = loop->getPrimaryColor();
      m_secondaryColor = loop->getSecondaryColor();
    }
    m_lastGradientValue = -1;
  }

  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override
  {
    int x = pt.x;
    int y = pt.y;

    Ink* ink = loop->getInk();
    Brush* brush = loop->getBrush();

    // Dynamics
    if (m_useDynamics) {
      // Dynamic gradient info
      if (m_hasDynamicGradient && m_dynamics.ditheringMatrix.rows() == 1 &&
          m_dynamics.ditheringMatrix.cols() == 1) {
        color_t a = m_secondaryColor;
        color_t b = m_primaryColor;
        const float t = pt.gradient;
        const float ti = 1.0f - pt.gradient;

        auto rgbaGradient = [t, ti](color_t a, color_t b) -> color_t {
          if (rgba_geta(a) == 0)
            return doc::rgba(rgba_getr(b), rgba_getg(b), rgba_getb(b), int(t * rgba_geta(b)));
          else if (rgba_geta(b) == 0)
            return doc::rgba(rgba_getr(a), rgba_getg(a), rgba_getb(a), int(ti * rgba_geta(a)));
          else
            return doc::rgba(int(ti * rgba_getr(a) + t * rgba_getr(b)),
                             int(ti * rgba_getg(a) + t * rgba_getg(b)),
                             int(ti * rgba_getb(a) + t * rgba_getb(b)),
                             int(ti * rgba_geta(a) + t * rgba_geta(b)));
        };

        switch (loop->sprite()->pixelFormat()) {
          case IMAGE_RGB: a = rgbaGradient(a, b); break;
          case IMAGE_GRAYSCALE:
            if (graya_geta(a) == 0)
              a = doc::graya(graya_getv(b), int(t * graya_geta(b)));
            else if (graya_geta(b) == 0)
              a = doc::graya(graya_getv(a), int(ti * graya_geta(a)));
            else
              a = doc::graya(int(ti * graya_getv(a) + t * graya_getv(b)),
                             int(ti * graya_geta(a) + t * graya_geta(b)));
            break;
          case IMAGE_INDEXED: {
            int maskIndex = (loop->getLayer()->isBackground() ? -1 :
                                                                loop->sprite()->transparentColor());
            // Convert index to RGBA
            if (a == maskIndex)
              a = 0;
            else
              a = loop->getPalette()->getEntry(a);
            if (b == maskIndex)
              b = 0;
            else
              b = loop->getPalette()->getEntry(b);
            // Same as in RGBA gradient
            a = rgbaGradient(a, b);
            // Convert RGBA to index
            a = loop->getRgbMap()->mapColor(rgba_getr(a), rgba_getg(a), rgba_getb(a), rgba_geta(a));
            break;
          }
        }
        loop->setPrimaryColor(a);
      }

      // Dynamic size and angle
      int size = std::clamp(int(pt.size), int(Brush::kMinBrushSize), int(Brush::kMaxBrushSize));
      int angle = std::clamp(int(pt.angle), -180, 180);
      if ((brush->size() != size) ||
          (brush->angle() != angle && m_origBrushType != kCircleBrushType) ||
          (m_hasDynamicGradient && pt.gradient != m_lastGradientValue)) {
        // TODO cache brushes
        BrushRef newBrush = std::make_shared<Brush>(m_origBrushType, size, angle);

        // Dynamic gradient with dithering
        bool prepareInk = false;
        if (m_hasDynamicGradient && !ink->isEraser() &&
            (m_dynamics.ditheringMatrix.rows() > 1 || m_dynamics.ditheringMatrix.cols() > 1)) {
          convert_bitmap_brush_to_dithering_brush(newBrush.get(),
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
      m_compressedImages.fill(nullptr);
    }

    x += brush->bounds().x;
    y += brush->bounds().y;

    if (m_firstPoint) {
      if ((brush->type() == kImageBrushType) && (brush->pattern() == BrushPattern::ALIGNED_TO_DST ||
                                                 brush->pattern() == BrushPattern::PAINT_BRUSH)) {
        brush->setPatternOrigin(gfx::Point(x, y));
      }
    }
    else {
      if (brush->type() == kImageBrushType && brush->pattern() == BrushPattern::PAINT_BRUSH) {
        brush->setPatternOrigin(gfx::Point(x, y));
      }
    }

    if (int(loop->getTiledMode()) & int(TiledMode::X_AXIS)) {
      int wrappedPatternOriginX = wrap_value(brush->patternOrigin().x, loop->sprite()->width()) %
                                  brush->bounds().w;
      brush->setPatternOrigin(gfx::Point(wrappedPatternOriginX, brush->patternOrigin().y));
      x = wrap_value(x, loop->sprite()->width());
    }
    if (int(loop->getTiledMode()) & int(TiledMode::Y_AXIS)) {
      int wrappedPatternOriginY = wrap_value(brush->patternOrigin().y, loop->sprite()->height()) %
                                  brush->bounds().h;
      brush->setPatternOrigin(gfx::Point(brush->patternOrigin().x, wrappedPatternOriginY));
      y = wrap_value(y, loop->sprite()->height());
    }

    ink->prepareForPointShape(loop, m_firstPoint, x, y);

    for (auto scanline : getCompressedImage(pt.symmetry)) {
      int u = x + scanline.x;
      ink->prepareVForPointShape(loop, y + scanline.y);
      doInkHline(u, y + scanline.y, u + scanline.w - 1, loop);
    }
    m_firstPoint = false;
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override
  {
    area = loop->getBrush()->bounds();
    area.x += x;
    area.y += y;
  }

private:
  CompressedImage& getCompressedImage(gen::SymmetryMode symmetryMode)
  {
    auto& compressPtr = m_compressedImages[int(symmetryMode)];
    if (!compressPtr) {
      switch (symmetryMode) {
        case gen::SymmetryMode::NONE: {
          compressPtr.reset(
            new CompressedImage(m_lastBrush->image(), m_lastBrush->maskBitmap(), false));
          break;
        }
        case gen::SymmetryMode::HORIZONTAL:
        case gen::SymmetryMode::VERTICAL:   {
          std::unique_ptr<Image> tempImage(Image::createCopy(m_lastBrush->image()));
          doc::algorithm::FlipType flip = (symmetryMode == gen::SymmetryMode::HORIZONTAL) ?
                                            doc::algorithm::FlipType::FlipHorizontal :
                                            doc::algorithm::FlipType::FlipVertical;
          doc::algorithm::flip_image(tempImage.get(), tempImage->bounds(), flip);
          compressPtr.reset(new CompressedImage(tempImage.get(), m_lastBrush->maskBitmap(), false));
          break;
        }
        case gen::SymmetryMode::BOTH: {
          std::unique_ptr<Image> tempImage(Image::createCopy(m_lastBrush->image()));
          doc::algorithm::flip_image(tempImage.get(),
                                     tempImage->bounds(),
                                     doc::algorithm::FlipType::FlipVertical);
          doc::algorithm::flip_image(tempImage.get(),
                                     tempImage->bounds(),
                                     doc::algorithm::FlipType::FlipHorizontal);
          compressPtr.reset(new CompressedImage(tempImage.get(), m_lastBrush->maskBitmap(), false));
          break;
        }
      }
    }
    return *compressPtr;
  }
};

class FloodFillPointShape : public PointShape {
public:
  bool isFloodFill() override { return true; }

  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override
  {
    const doc::Image* srcImage = loop->getFloodFillSrcImage();
    const bool tilesMode = (srcImage->pixelFormat() == IMAGE_TILEMAP);
    gfx::Point wpt = pt.toPoint();
    if (tilesMode) { // Tiles mode
      const doc::Grid& grid = loop->getGrid();
      wpt = grid.canvasToTile(wpt);
    }
    else {
      wpt = wrap_point(loop->getTiledMode(),
                       gfx::Size(srcImage->width(), srcImage->height()),
                       wpt,
                       true);
    }

    loop->getInk()->prepareForPointShape(loop, true, wpt.x, wpt.y);

    doc::algorithm::floodfill(
      srcImage,
      (loop->useMask() ? loop->getMask() : nullptr),
      wpt.x,
      wpt.y,
      (tilesMode ? srcImage->bounds() : floodfillBounds(loop, wpt.x, wpt.y)),
      get_pixel(srcImage, wpt.x, wpt.y),
      loop->getTolerance(),
      loop->getContiguous(),
      loop->isPixelConnectivityEightConnected(),
      loop,
      (AlgoHLine)doInkHline);
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override
  {
    area = floodfillBounds(loop, x, y);
  }

private:
  gfx::Rect floodfillBounds(ToolLoop* loop, int x, int y) const
  {
    const doc::Image* srcImage = loop->getFloodFillSrcImage();
    gfx::Rect bounds = loop->sprite()->bounds();
    bounds &= srcImage->bounds();

    if (srcImage->pixelFormat() == IMAGE_TILEMAP) { // Tiles mode
      const doc::Grid& grid = loop->getGrid();
      bounds = grid.tileToCanvas(bounds);
    }
    // Limit the flood-fill to the current tile if the grid is visible.
    else if (loop->getStopAtGrid()) {
      gfx::Rect grid = loop->getGridBounds();
      if (!grid.isEmpty()) {
        div_t d, dx, dy;

        dx = div(grid.x, grid.w);
        dy = div(grid.y, grid.h);

        if (dx.rem > 0)
          dx.rem -= grid.w;
        if (dy.rem > 0)
          dy.rem -= grid.h;

        d = div(x - dx.rem, grid.w);
        x = dx.rem + d.quot * grid.w;

        d = div(y - dy.rem, grid.h);
        y = dy.rem + d.quot * grid.h;

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

  void preparePointShape(ToolLoop* loop) override { m_subPointShape.preparePointShape(loop); }

  void transformPoint(ToolLoop* loop, const Stroke::Pt& pt) override
  {
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

    double angle, radius;

    for (int c = 0; c < integral_points; c++) {
      angle = 360.0 * rand() / RAND_MAX;
      radius = double(spray_width) * rand() / RAND_MAX;

      Stroke::Pt pt2(pt);
      pt2.x += double(radius * std::cos(angle));
      pt2.y += double(radius * std::sin(angle));
      m_subPointShape.transformPoint(loop, pt2);
    }
  }

  void getModifiedArea(ToolLoop* loop, int x, int y, Rect& area) override
  {
    int spray_width = loop->getSprayWidth();
    Point p1(x - spray_width, y - spray_width);
    Point p2(x + spray_width, y + spray_width);

    Rect area1;
    Rect area2;
    m_subPointShape.getModifiedArea(loop, p1.x, p1.y, area1);
    m_subPointShape.getModifiedArea(loop, p2.x, p2.y, area2);

    area = area1.createUnion(area2);
  }
};

}} // namespace app::tools
