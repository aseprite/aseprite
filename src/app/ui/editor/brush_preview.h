// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
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

  class BrushPreview {
  public:
    // Brush type
    enum {
      CROSSHAIR           = 1,
      SELECTION_CROSSHAIR = 2,
      BRUSH_BOUNDARIES    = 4,
      NATIVE_CROSSHAIR    = 8,
    };

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

    void generateBoundaries();
    void forEachBrushPixel(
      ui::Graphics* g,
      const gfx::Point& screenPos,
      const gfx::Point& spritePos,
      gfx::Color color,
      PixelDelegate pixelDelegate);

    void traceCrossPixels(ui::Graphics* g, const gfx::Point& pt, gfx::Color color, PixelDelegate pixel);
    void traceSelectionCrossPixels(ui::Graphics* g, const gfx::Point& pt, gfx::Color color, int thickness, PixelDelegate pixel);
    void traceBrushBoundaries(ui::Graphics* g, gfx::Point pos, gfx::Color color, PixelDelegate pixel);

    void savePixelDelegate(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);
    void drawPixelDelegate(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);
    void clearPixelDelegate(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);

    Editor* m_editor;
    int m_type;

    // The brush preview shows the cross or brush boundaries as black
    // & white negative.
    bool m_blackAndWhiteNegative;

    // The brush preview is on the screen.
    bool m_onScreen;
    bool m_withModifiedPixels;
    bool m_withRealPreview;
    gfx::Point m_screenPosition; // Position in the screen (view)
    gfx::Point m_editorPosition; // Position in the editor (model)

    // Information about current brush
    doc::MaskBoundaries m_brushBoundaries;
    int m_brushGen;
    int m_brushWidth;
    int m_brushHeight;

    std::vector<gfx::Color> m_savedPixels;
    int m_savedPixelsIterator;
    int m_savedPixelsLimit;

    gfx::Region m_clippingRegion;
    gfx::Region m_oldClippingRegion;

    // Information stored in show() and used in hide() to clear the
    // brush preview in the exact same place.
    gfx::Rect m_lastBounds;
    doc::frame_t m_lastFrame;

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
