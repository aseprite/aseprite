// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2016  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_EDITOR_BRUSH_PREVIEW_H_INCLUDED
#define APP_UI_EDITOR_BRUSH_PREVIEW_H_INCLUDED
#pragma once

#include "app/extra_cel.h"
#include "doc/brush.h"
#include "doc/color.h"
#include "doc/frame.h"
#include "doc/mask_boundaries.h"
#include "gfx/color.h"
#include "gfx/point.h"
#include "gfx/rect.h"
#include "gfx/region.h"
#include "os/surface.h"
#include "ui/cursor.h"

#include <vector>

namespace doc {
  class Layer;
  class Sprite;
}

namespace ui {
  class Graphics;
}

namespace app {
  class Editor;
  class Site;

  class BrushPreview {
  public:
    // Brush type
    enum {
      // A simple cursor in the mouse position for drawing tools. The
      // crosshair is painted in real-time with black and white
      // depending on the pixel of the screen.
      //
      //     |
      //     |
      // --- * ---
      //     |
      //     |
      CROSSHAIR = 1,

      // Crosshair used in the selection tools in the sprite position.
      //
      //   | |
      // --   --
      //    *
      // --   --
      //   | |
      SELECTION_CROSSHAIR = 2,

      // The boundaries of the brush used in the sprite position
      // (depends on the shape of the brush generated with
      // doc::MaskBoundaries).
      BRUSH_BOUNDARIES = 4,

      // Use a pre-defined native cursor that is a crosshair in the
      // mouse position.
      NATIVE_CROSSHAIR = 8,
    };

    static void destroyInternals();

    BrushPreview(Editor* editor);
    ~BrushPreview();

    bool onScreen() const { return m_onScreen; }
    const gfx::Point& screenPosition() const { return m_screenPosition; }

    void show(const gfx::Point& screenPos);
    void hide();
    void redraw();
    void discardBrushPreview();

    void invalidateRegion(const gfx::Region& region);

  private:
    typedef void (BrushPreview::*PixelDelegate)(ui::Graphics*, const gfx::Point&, gfx::Color);

    doc::BrushRef getCurrentBrush();
    static doc::color_t getBrushColor(doc::Sprite* sprite, doc::Layer* layer);

    // Offset re-calculation of brush boundaries only for TilemapMode::Tiles.
    // Used within 'generateBoundaries' function.
    void calculateTileBoundariesOrigin(const doc::Grid& grid,
                                       const gfx::Point& spritePos);

    void generateBoundaries(const Site& site,
                            const gfx::Point& spritePos);

    // Creates a little native cursor to draw the CROSSHAIR
    void createCrosshairCursor(ui::Graphics* g, const gfx::Color cursorColor);

    void forEachBrushPixel(
      ui::Graphics* g,
      const gfx::Point& spritePos,
      gfx::Color color,
      PixelDelegate pixelDelegate);

    void traceSelectionCrossPixels(ui::Graphics* g, const gfx::Point& pt, gfx::Color color, int thickness, PixelDelegate pixel);
    void traceBrushBoundaries(ui::Graphics* g, gfx::Point pos, gfx::Color color, PixelDelegate pixel);

    void savePixelDelegate(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);
    void drawPixelDelegate(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);
    void clearPixelDelegate(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);

    Editor* m_editor;
    int m_type = CROSSHAIR;

    // The brush preview shows the cross or brush boundaries as black
    // & white negative.
    bool m_blackAndWhiteNegative;

    // The brush preview is on the screen.
    bool m_onScreen = false;

    bool m_withRealPreview = false;
    gfx::Point m_screenPosition; // Position in the screen (view)
    gfx::Point m_editorPosition; // Position in the editor (model)

    // Information about current brush
    doc::MaskBoundaries m_brushBoundaries;
    int m_brushGen;

    // True if we've modified pixels in the display surface
    // (e.g. drawing the selection crosshair or the brush edges).
    bool m_withModifiedPixels = false;
    std::vector<gfx::Color> m_savedPixels;
    int m_savedPixelsIterator;
    int m_savedPixelsLimit;

    gfx::Region m_clippingRegion;
    gfx::Region m_oldClippingRegion;

    // Information stored in show() and used in hide() to clear the
    // brush preview in the exact same place.
    gfx::Rect m_lastBounds;
    doc::frame_t m_lastFrame;

    doc::Layer* m_lastLayer = nullptr;
    TilemapMode m_lastTilemapMode;

    ExtraCelRef m_extraCel;
  };

  class HideBrushPreview {
  public:
    HideBrushPreview(BrushPreview& brushPreview)
      : m_brushPreview(brushPreview)
      , m_oldScreenPosition(brushPreview.screenPosition())
      , m_onScreen(brushPreview.onScreen()) {
      if (m_onScreen)
        m_brushPreview.hide();
    }

    ~HideBrushPreview() {
      if (m_onScreen)
        m_brushPreview.show(m_oldScreenPosition);
    }

  private:
    BrushPreview& m_brushPreview;
    gfx::Point m_oldScreenPosition;
    bool m_onScreen;
  };

} // namespace app

#endif
