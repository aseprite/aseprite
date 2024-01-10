// Aseprite
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#define BP_TRACE(...) // TRACEARGS(__VA_ARGS__)

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/brush_preview.h"

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/doc.h"
#include "app/snap_to_grid.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/intertwine.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool.h"
#include "app/tools/tool_loop.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/tool_loop_impl.h"
#include "app/ui_context.h"
#include "app/util/wrap_value.h"
#include "base/debug.h"
#include "base/scoped_value.h"
#include "doc/algo.h"
#include "doc/blend_internals.h"
#include "doc/brush.h"
#include "doc/cel.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "os/surface.h"
#include "os/system.h"
#include "os/window.h"
#include "render/render.h"
#include "ui/manager.h"
#include "ui/system.h"

#include <array>

namespace app {

using namespace doc;

static int g_crosshair_pattern[7*7] = {
  0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0,
  1, 1, 0, 0, 0, 1, 1,
  0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0,
  0, 0, 0, 1, 0, 0, 0,
};

// We're going to keep a cache of native mouse cursor for each
// possibility of the following crosshair:
//
//      2
//      2
//   22 3 22
//      2
//      2
//
// Number of crosshair cursors 2^8 * 3 + 2 = 770
// Here the center can be black, white or hidden. When the center is
// black or white, the crosshair can be empty.
//
// The index/key of this array is calculated in
// BrushPreview::createCrosshairCursor().
//
// Win32: This is needed to avoid converting from a os::Surface ->
// HCURSOR calling CreateIconIndirect() as many times as possible for
// each mouse movement (because it's a slow function).
//
static std::array<os::CursorRef, 770> g_bwCursors;
static int g_cacheCursorScale = 0;

// 3 cached cursors when we use a solid cursor (1 dot, crosshair
// without dot at the center, crosshair with dot in the center)
static std::array<os::CursorRef, 3> g_solidCursors;
static gfx::Color g_solidCursorColor = gfx::ColorNone;

// static
void BrushPreview::destroyInternals()
{
  g_bwCursors.fill(nullptr);
  g_solidCursors.fill(nullptr);
}

BrushPreview::BrushPreview(Editor* editor)
  : m_editor(editor)
{
}

BrushPreview::~BrushPreview()
{
}

BrushRef BrushPreview::getCurrentBrush()
{
  return App::instance()
    ->contextBar()
    ->activeBrush(m_editor->getCurrentEditorTool());
}

// static
color_t BrushPreview::getBrushColor(Sprite* sprite, Layer* layer)
{
  app::Color c = Preferences::instance().colorBar.fgColor();
  ASSERT(sprite != NULL);

  // Avoid using invalid colors
  if (!c.isValid())
    return 0;

  if (layer != NULL)
    return color_utils::color_for_layer(c, layer);
  else
    return color_utils::color_for_image(c, sprite->pixelFormat());
}

// Draws the brush cursor in the specified absolute mouse position
// given in 'pos' param.  Warning: You should clean the cursor before
// to use this routine with other editor.
//
// TODO the logic in this function is really complex, we should think
//      a way to simplify all possibilities
void BrushPreview::show(const gfx::Point& screenPos)
{
  if (m_onScreen)
    hide();

  Doc* document = m_editor->document();
  Sprite* sprite = m_editor->sprite();
  Site site = m_editor->getSite();
  Layer* layer = (m_editor->layer() &&
                  m_editor->layer()->isImage() ? m_editor->layer():
                                                 nullptr);
  ASSERT(sprite);

  // Get drawable region
  m_editor->getDrawableRegion(m_clippingRegion, ui::Widget::kCutTopWindows);

  // Remove the invalidated region in the editor.
  m_clippingRegion.createSubtraction(m_clippingRegion,
                                     m_editor->getUpdateRegion());

  // Get cursor color
  const auto& pref = Preferences::instance();
  app::Color appCursorColor = pref.cursor.cursorColor();
  m_blackAndWhiteNegative = (appCursorColor.getType() == app::Color::MaskType);

  BrushRef brush = getCurrentBrush();

  tools::Tool* tool = m_editor->getCurrentEditorTool();
  const bool isFloodfill = tool->getPointShape(0)->isFloodFill();

  // Get current tilemap mode
  const TilemapMode tilemapMode = site.tilemapMode();

  gfx::Rect origBrushBounds =
    ((isFloodfill && brush->type() != BrushType::kImageBrushType) ||
     tilemapMode == TilemapMode::Tiles ? gfx::Rect(0, 0, 1, 1)
                                       : brush->bounds());
  gfx::Rect brushBounds = origBrushBounds;

  // Cursor in the screen (view)
  m_screenPosition = screenPos;

  // Get cursor position in the editor
  gfx::Point spritePos = m_editor->screenToEditor(screenPos);
  if (pref.cursor.snapToGrid() &&
      m_editor->docPref().grid.snap()) {
    spritePos = snap_to_grid(m_editor->docPref().grid.bounds(),
                             spritePos,
                             PreferSnapTo::ClosestGridVertex) +
                gfx::Point(brushBounds.w / 2, brushBounds.h / 2);
  }

  // Get the current tool
  tools::Ink* ink = m_editor->getCurrentEditorInk();

  // Setup the cursor type depending on several factors (current tool,
  // foreground color, layer transparency, brush size, etc.).
  color_t brush_color = getBrushColor(sprite, layer);
  color_t mask_index = sprite->transparentColor();

  // Check dynamics option for freehand tools
  if (tool &&
      tool->getController(0)->isFreehand() &&
      // TODO add support for dynamics to contour tool
      tool->getFill(0) == tools::FillNone) {
    const auto& dynamics = App::instance()->contextBar()->getDynamics();
    if (brush->type() != doc::kImageBrushType &&
        (dynamics.size != tools::DynamicSensor::Static ||
         dynamics.angle != tools::DynamicSensor::Static)) {
      brush.reset(
        new Brush(
          brush->type(),
          (dynamics.size != tools::DynamicSensor::Static ? dynamics.minSize: brush->size()),
          (dynamics.angle != tools::DynamicSensor::Static ? dynamics.minAngle: brush->angle())));
    }
  }

  if (ink->isSelection() || ink->isSlice()) {
    m_type = SELECTION_CROSSHAIR;
  }
  else if (
    (tilemapMode == TilemapMode::Pixels) &&
    (brush->type() == kImageBrushType ||
     ((isFloodfill ? 1: brush->size()) > (1.0 / m_editor->zoom().scale()))) &&
    (// Use cursor bounds for inks that are effects (eraser, blur, etc.)
     (ink->isEffect()) ||
     // or when the brush color is transparent and we are not in the background layer
     (!ink->isShading() &&
      (layer && layer->isTransparent()) &&
      ((sprite->pixelFormat() == IMAGE_INDEXED && brush_color == mask_index) ||
       (sprite->pixelFormat() == IMAGE_RGB && rgba_geta(brush_color) == 0) ||
       (sprite->pixelFormat() == IMAGE_GRAYSCALE && graya_geta(brush_color) == 0))))) {
    m_type = BRUSH_BOUNDARIES;
  }
  else {
    m_type = CROSSHAIR;
  }

  bool showPreview = false;
  bool showPreviewWithEdges = false;
  bool cancelEdges = false;
  auto brushPreview = pref.cursor.brushPreview();
  if (!m_editor->docPref().show.brushPreview())
    brushPreview = app::gen::BrushPreview::NONE;

  switch (brushPreview) {
    case app::gen::BrushPreview::NONE:
      m_type = CROSSHAIR;
      break;
    case app::gen::BrushPreview::EDGES:
      m_type = BRUSH_BOUNDARIES;
      break;
    case app::gen::BrushPreview::FULL:
    case app::gen::BrushPreview::FULLALL:
    case app::gen::BrushPreview::FULLNEDGES:
      showPreview = m_editor->getState()->requireBrushPreview();
      switch (brushPreview) {
        case app::gen::BrushPreview::FULLALL:
          if (showPreview)
            m_type = CROSSHAIR;
          cancelEdges = true;
          break;
        case app::gen::BrushPreview::FULLNEDGES:
          if (showPreview)
            showPreviewWithEdges = true;
          m_type = BRUSH_BOUNDARIES;
          break;
      }
      break;
  }

  // For tiles as we show the edges of the tile, we add the crosshair
  // in the mouse position as a helper.
  if (m_type & BRUSH_BOUNDARIES &&
      tilemapMode == TilemapMode::Tiles)
    m_type |= CROSSHAIR;

  if (m_type & SELECTION_CROSSHAIR)
    showPreview = false;

  // When the extra cel is locked (e.g. we are flashing the active
  // layer) we don't show the brush preview temporally.
  if (showPreview && m_editor->isExtraCelLocked()) {
    showPreview = false;
    showPreviewWithEdges = false;
    cancelEdges = false;
    m_type |= BRUSH_BOUNDARIES;
  }

  // Use a simple cross
  if (pref.cursor.paintingCursorType() == gen::PaintingCursorType::SIMPLE_CROSSHAIR) {
    m_type &= ~(CROSSHAIR | SELECTION_CROSSHAIR);
    m_type |= NATIVE_CROSSHAIR;
  }

  // For cursor type 'bounds' we have to generate cursor boundaries
  if (m_type & BRUSH_BOUNDARIES) {
    if (brush->type() != kImageBrushType)
      showPreview = showPreviewWithEdges;
    if (cancelEdges)
      m_type &= ~BRUSH_BOUNDARIES;
  }
  if (m_type & BRUSH_BOUNDARIES)
    generateBoundaries(site, spritePos);

  // Draw pixel/brush preview
  if (showPreview) {
    brushBounds.offset(spritePos);
    gfx::Rect extraCelBoundsInCanvas = brushBounds;

    // Tiled mode might require a bigger extra cel (to show the tiled)
    if (int(m_editor->docPref().tiled.mode()) & int(filters::TiledMode::X_AXIS)) {
      brushBounds.x = wrap_value(brushBounds.x, sprite->width());
      extraCelBoundsInCanvas.x = brushBounds.x;
      if ((extraCelBoundsInCanvas.x < 0 && extraCelBoundsInCanvas.x2() > 0) ||
          (extraCelBoundsInCanvas.x < sprite->width() && extraCelBoundsInCanvas.x2() > sprite->width())) {
        extraCelBoundsInCanvas.x = 0;
        extraCelBoundsInCanvas.w = sprite->width();
      }
    }
    if (int(m_editor->docPref().tiled.mode()) & int(filters::TiledMode::Y_AXIS)) {
      brushBounds.y = wrap_value(brushBounds.y, sprite->height());
      extraCelBoundsInCanvas.y = brushBounds.y;
      if ((extraCelBoundsInCanvas.y < 0 && extraCelBoundsInCanvas.y2() > 0) ||
          (extraCelBoundsInCanvas.y < sprite->height() && extraCelBoundsInCanvas.y2() > sprite->height())) {
        extraCelBoundsInCanvas.y = 0;
        extraCelBoundsInCanvas.h = sprite->height();
      }
    }

    gfx::Rect extraCelBounds;
    if (tilemapMode == TilemapMode::Tiles) {
      ASSERT(layer->isTilemap());
      doc::Grid grid = site.grid();
      extraCelBounds = grid.canvasToTile(extraCelBoundsInCanvas);
      extraCelBoundsInCanvas = grid.tileToCanvas(extraCelBounds);
    }
    else {
      extraCelBounds = extraCelBoundsInCanvas;
    }

    BP_TRACE("BrushPreview:",
             "brushBounds", brushBounds,
             "extraCelBounds", extraCelBounds,
             "extraCelBoundsInCanvas", extraCelBoundsInCanvas);

    // Create the extra cel to show the brush preview
    Cel* cel = site.cel();

    int t, opacity = 255;
    if (cel) opacity = MUL_UN8(opacity, cel->opacity(), t);
    if (layer) opacity = MUL_UN8(opacity, static_cast<LayerImage*>(layer)->opacity(), t);

    if (!m_extraCel)
      m_extraCel.reset(new ExtraCel);

    m_extraCel->create(
      tilemapMode,
      document->sprite(),
      extraCelBoundsInCanvas,
      extraCelBounds.size(),
      site.frame(),
      opacity);
    m_extraCel->setType(render::ExtraType::NONE);
    m_extraCel->setBlendMode(
      (layer ? static_cast<LayerImage*>(layer)->blendMode():
               BlendMode::NORMAL));

    document->setExtraCel(m_extraCel);

    Image* extraImage = m_extraCel->image();
    if (extraImage->pixelFormat() == IMAGE_TILEMAP) {
      extraImage->setMaskColor(notile);
      clear_image(extraImage, notile);
    }
    else {
      extraImage->setMaskColor(mask_index);
      clear_image(extraImage,
                  (extraImage->pixelFormat() == IMAGE_INDEXED ? mask_index: 0));
    }

    if (layer) {
      render::Render().renderLayer(
        extraImage, layer, site.frame(),
        gfx::Clip(0, 0, extraCelBoundsInCanvas),
        BlendMode::SRC);

      // This extra cel is a patch for the current layer/frame
      m_extraCel->setType(render::ExtraType::PATCH);
    }

    {
      std::unique_ptr<tools::ToolLoop> loop(
        create_tool_loop_preview(
          m_editor, brush, extraImage,
          extraCelBounds.origin()));
      if (loop) {
        loop->getInk()->prepareInk(loop.get());
        loop->getController()->prepareController(loop.get());
        loop->getIntertwine()->prepareIntertwine(loop.get());
        loop->getPointShape()->preparePointShape(loop.get());

        tools::Stroke::Pt pt(brushBounds.x-origBrushBounds.x,
                             brushBounds.y-origBrushBounds.y);
        pt.size = brush->size();
        pt.angle = brush->angle();
        loop->getPointShape()->transformPoint(loop.get(), pt);
      }
    }

    document->notifySpritePixelsModified(
      sprite, gfx::Region(m_lastBounds = extraCelBoundsInCanvas),
      m_lastFrame = site.frame());

    m_withRealPreview = true;
  }

  // Save area and draw the cursor
  if (!(m_type & NATIVE_CROSSHAIR) ||
      (m_type & BRUSH_BOUNDARIES)) {
    ui::ScreenGraphics g(m_editor->display());
    ui::SetClip clip(&g);
    gfx::Color uiCursorColor = color_utils::color_for_ui(appCursorColor);

    if (!(m_type & NATIVE_CROSSHAIR)) {
      createCrosshairCursor(&g, uiCursorColor);
    }

    forEachBrushPixel(&g, spritePos, uiCursorColor, &BrushPreview::savePixelDelegate);
    forEachBrushPixel(&g, spritePos, uiCursorColor, &BrushPreview::drawPixelDelegate);
    m_withModifiedPixels = true;
  }

  // Cursor in the editor (model)
  m_onScreen = true;
  m_editorPosition = spritePos;

  // Save the clipping-region to know where to clean the pixels
  m_oldClippingRegion = m_clippingRegion;

  if (m_type & NATIVE_CROSSHAIR)
    ui::set_mouse_cursor(ui::kCrosshairCursor);

  m_lastLayer = site.layer();
  m_lastTilemapMode = tilemapMode;
}

// Cleans the brush cursor from the specified editor.
//
// The mouse position is got from the last call to showBrushPreview()
// (m_cursorEditor). So you must to use this routine only if you
// called showBrushPreview() before.
void BrushPreview::hide()
{
  if (!m_onScreen)
    return;

  // Don't hide the cursor to avoid flickering, the native mouse
  // cursor will be changed anyway after the hide() by the caller.
  //
  //if (m_cursor)
  //  m_editor->display()->nativeWindow()->setCursor(os::NativeCursor::Hidden);

  // Get drawable region
  m_editor->getDrawableRegion(m_clippingRegion, ui::Widget::kCutTopWindows);

  // Remove the invalidated region in the editor.
  m_clippingRegion.createSubtraction(m_clippingRegion,
                                     m_editor->getUpdateRegion());

  if (m_withModifiedPixels) {
    // Restore pixels
    ui::ScreenGraphics g(m_editor->display());
    ui::SetClip clip(&g);
    forEachBrushPixel(&g, m_editorPosition, gfx::ColorNone,
                      &BrushPreview::clearPixelDelegate);
  }

  // Clean pixel/brush preview
  if (m_withRealPreview) {
    Doc* document = m_editor->document();
    doc::Sprite* sprite = m_editor->sprite();

    ASSERT(document);
    ASSERT(sprite);

    if (document && sprite) {
      document->setExtraCel(ExtraCelRef(nullptr));
      document->notifySpritePixelsModified(
        sprite, gfx::Region(m_lastBounds), m_lastFrame);
    }

    m_withRealPreview = false;
  }

  m_onScreen = false;

  m_clippingRegion.clear();
  m_oldClippingRegion.clear();
}

void BrushPreview::discardBrushPreview()
{
  Doc* document = m_editor->document();
  ASSERT(document);

  if (document && m_onScreen && m_withRealPreview) {
    document->setExtraCel(ExtraCelRef(nullptr));
  }
}

void BrushPreview::redraw()
{
  if (m_onScreen) {
    gfx::Point screenPos = m_screenPosition;
    hide();
    show(screenPos);
  }
}

void BrushPreview::invalidateRegion(const gfx::Region& region)
{
  m_clippingRegion.createSubtraction(m_clippingRegion, region);
}

void BrushPreview::calculateTileBoundariesOrigin(const doc::Grid& grid,
                                                 const gfx::Point& spritePos)
{
  m_brushBoundaries.offset(-m_brushBoundaries.begin()[0].bounds().x,
                           -m_brushBoundaries.begin()[0].bounds().y);
  gfx::Point canvasPos = grid.tileToCanvas(grid.canvasToTile(spritePos));
  m_brushBoundaries.offset(canvasPos.x - spritePos.x,
                           canvasPos.y - spritePos.y);
}

void BrushPreview::generateBoundaries(const Site& site,
                                      const gfx::Point& spritePos)
{
  BrushRef brush = getCurrentBrush();
  Layer* currentLayer = site.layer();
  TilemapMode tilemapMode = site.tilemapMode();

  if (tilemapMode == TilemapMode::Pixels &&
      tilemapMode == m_lastTilemapMode &&
      !m_brushBoundaries.isEmpty() &&
      m_brushGen == brush->gen()) {
    return;
  }
  else if (tilemapMode == TilemapMode::Tiles &&
           tilemapMode == m_lastTilemapMode &&
           m_lastLayer == currentLayer) {
    // When tilemapMode is 'Tiles' is needed an offset
    // re-calculation of the brush boundaries, even
    // if it's no need to update the mask.
    calculateTileBoundariesOrigin(site.grid(), spritePos);
    return;
  }

  const bool isOnePixel =
    (m_editor->getCurrentEditorTool()->getPointShape(0)->isPixel() ||
     m_editor->getCurrentEditorTool()->getPointShape(0)->isFloodFill());
  Image* brushImage = brush->image();
  m_brushGen = brush->gen();

  Image* mask = nullptr;
  bool deleteMask = true;
  if (tilemapMode == TilemapMode::Tiles) {
    mask = Image::create(IMAGE_BITMAP,
                         site.grid().tileSize().w,
                         site.grid().tileSize().h);
    mask->clear((color_t)1);
  }
  else if (isOnePixel) {
    mask = Image::create(IMAGE_BITMAP, 1, 1);
    mask->putPixel(0, 0, (color_t)1);
  }
  else if (brushImage->pixelFormat() != IMAGE_BITMAP) {
    ASSERT(brush->maskBitmap());
    deleteMask = false;
    mask = brush->maskBitmap();
  }

  m_brushBoundaries.regen(mask ? mask: brushImage);
  if (tilemapMode == TilemapMode::Pixels) {
    if (!isOnePixel)
      m_brushBoundaries.offset(-brush->center().x,
                               -brush->center().y);
  }
  else
    calculateTileBoundariesOrigin(site.grid(), spritePos);

  if (deleteMask)
    delete mask;
}

void BrushPreview::createCrosshairCursor(ui::Graphics* g,
                                         const gfx::Color cursorColor)
{
  ASSERT(!(m_type & NATIVE_CROSSHAIR));

  gfx::Rect cursorBounds;
  gfx::Point cursorCenter;

  // Depending on the editor zoom, maybe we need subpixel movement (a
  // little dot inside the active pixel)
  const bool requireLittleCenterDot = (m_editor->zoom().scale() >= 4.0);

  if (m_type & CROSSHAIR) {
    // Regular crosshair of 7x7
    cursorBounds |= gfx::Rect(-3, -3, 7, 7);
    cursorCenter = -cursorBounds.origin();
  }
  else if (requireLittleCenterDot) {
    // Special case of a cursor for one pixel
    cursorBounds = gfx::Rect(0, 0, 1, 1);
    cursorCenter = gfx::Point(0, 0);
  }
  else {
    // TODO should we use ui::set_mouse_cursor()?
    ui::set_mouse_cursor_reset_info();
    m_editor->display()->nativeWindow()->setCursor(os::NativeCursor::Hidden);
    return;
  }

  os::Window* window = m_editor->display()->nativeWindow();
  const int scale = window->scale();
  os::CursorRef cursor = nullptr;

  // Invalidate the entire cache if the scale has changed
  if (g_cacheCursorScale != scale) {
    g_cacheCursorScale = scale;
    g_bwCursors.fill(nullptr);
    g_solidCursors.fill(nullptr);
  }

  // Cursor with black/white colors (we create a key/index for
  // g_cachedCursors depending on the colors on the screen)
  if (m_blackAndWhiteNegative) {
    int k = 0;
    if (m_type & CROSSHAIR) {
      int bit = 0;
      for (int v=0; v<7; v++) {
        for (int u=0; u<7; u++) {
          if (g_crosshair_pattern[v*7+u]) {
            color_t c = g->getPixel(m_screenPosition.x-3+u,
                                    m_screenPosition.y-3+v);
            c = color_utils::blackandwhite_neg(c);
            if (rgba_getr(c) == 255) { // White
              k |= (1 << bit);
            }
            ++bit;
          }
        }
      }
    }
    if (requireLittleCenterDot) {
      color_t c = g->getPixel(m_screenPosition.x,
                              m_screenPosition.y);
      c = color_utils::blackandwhite_neg(c);
      if (rgba_getr(c) == 255) {       // White
        k |= (m_type & CROSSHAIR ? 0x200: 0x301);
      }
      else {                           // Black
        k |= (m_type & CROSSHAIR ? 0x100: 0x300);
      }
    }

    ASSERT(k < int(g_bwCursors.size()));
    if (k >= int(g_bwCursors.size())) // Unexpected key value in release mode
      return;

    // Use cached cursor
    if (g_bwCursors[k]) {
      cursor = g_bwCursors[k];
    }
    else {
      const gfx::Color black = gfx::rgba(0, 0, 0);
      const gfx::Color white = gfx::rgba(255, 255, 255);
      os::SurfaceRef cursorSurface =
        os::instance()->makeRgbaSurface(cursorBounds.w,
                                        cursorBounds.h);
      cursorSurface->clear();
      int bit = 0;
      if (m_type & CROSSHAIR) {
        for (int v=0; v<7; v++) {
          for (int u=0; u<7; u++) {
            if (g_crosshair_pattern[v*7+u]) {
              cursorSurface->putPixel(
                (k & (1 << bit) ? white: black), u, v);
              ++bit;
            }
          }
        }
      }
      if (requireLittleCenterDot) {
        if (m_type & CROSSHAIR) {
          if (k & (0x100 | 0x200)) { // 0x100 or 0x200
            cursorSurface->putPixel(
              (k & 0x100 ? black: white),
              cursorBounds.w/2, cursorBounds.h/2);
          }
        }
        else {                       // 0x300 or 0x301
          cursorSurface->putPixel(
            (k == 0x300 ? black: white),
            cursorBounds.w/2, cursorBounds.h/2);
        }
      }

      cursor = g_bwCursors[k] =
        os::instance()->makeCursor(
          cursorSurface.get(),
          cursorCenter,
          scale);
    }
  }
  // Cursor with solid color (easiest case, we don't have to check the
  // colors in the screen to create the crosshair)
  else {
    // We have to recreate all cursors if the color has changed.
    if (g_solidCursorColor != cursorColor) {
      g_solidCursors.fill(nullptr);
      g_solidCursorColor = cursorColor;
    }

    int k = 0;
    if (m_type & CROSSHAIR) {
      if (requireLittleCenterDot)
        k = 2;
      else
        k = 1;
    }

    // Use cached cursor
    if (g_solidCursors[k]) {
      cursor = g_solidCursors[k];
    }
    else {
      os::SurfaceRef cursorSurface =
        os::instance()->makeRgbaSurface(cursorBounds.w,
                                        cursorBounds.h);
      cursorSurface->clear();
      if (m_type & CROSSHAIR) {
        for (int v=0; v<7; v++)
          for (int u=0; u<7; u++)
            if (g_crosshair_pattern[v*7+u])
              cursorSurface->putPixel(cursorColor, u, v);
      }
      if (requireLittleCenterDot)
        cursorSurface->putPixel(cursorColor, cursorBounds.w/2, cursorBounds.h/2);

      cursor = g_solidCursors[k] =
        os::instance()->makeCursor(
          cursorSurface.get(),
          cursorCenter,
          scale);
    }
  }

  if (cursor) {
    // TODO should we use ui::set_mouse_cursor()?
    ui::set_mouse_cursor_reset_info();
    window->setCursor(cursor);
  }
}

void BrushPreview::forEachBrushPixel(
  ui::Graphics* g,
  const gfx::Point& spritePos,
  gfx::Color color,
  PixelDelegate pixelDelegate)
{
  m_savedPixelsIterator = 0;

  if (m_type & SELECTION_CROSSHAIR)
    traceSelectionCrossPixels(g, spritePos, color, 1, pixelDelegate);

  if (m_type & BRUSH_BOUNDARIES)
    traceBrushBoundaries(g, spritePos, color, pixelDelegate);

  m_savedPixelsLimit = m_savedPixelsIterator;
}

// Old thick cross (used for selection tools)
void BrushPreview::traceSelectionCrossPixels(
  ui::Graphics* g,
  const gfx::Point& pt, gfx::Color color,
  int thickness, PixelDelegate pixelDelegate)
{
  static int cross[6*6] = {
    0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0,
  };
  gfx::Point out, outpt = m_editor->editorToScreen(pt);
  const render::Projection& proj = m_editor->projection();
  gfx::Size size(proj.applyX(thickness/2),
                 proj.applyY(thickness/2));
  gfx::Size size2(proj.applyX(thickness),
                  proj.applyY(thickness));
  if (size2.w == 0) size2.w = 1;
  if (size2.h == 0) size2.h = 1;

  for (int v=0; v<6; v++) {
    for (int u=0; u<6; u++) {
      if (!cross[v*6+u])
        continue;

      out = outpt;
      out.x += ((u<3) ? u-size.w-3: u-size.w-3+size2.w);
      out.y += ((v<3) ? v-size.h-3: v-size.h-3+size2.h);

      (this->*pixelDelegate)(g, out, color);
    }
  }
}

// Current brush edges
void BrushPreview::traceBrushBoundaries(ui::Graphics* g,
                                        gfx::Point pos,
                                        gfx::Color color,
                                        PixelDelegate pixelDelegate)
{
  for (const auto& seg : m_brushBoundaries) {
    gfx::Rect bounds = seg.bounds();
    bounds.offset(pos);
    bounds = m_editor->editorToScreen(bounds);

    if (seg.open()) {
      if (seg.vertical()) --bounds.x;
      else --bounds.y;
    }

    gfx::Point pt(bounds.x, bounds.y);
    if (seg.vertical()) {
      for (; pt.y<bounds.y+bounds.h; ++pt.y)
        (this->*pixelDelegate)(g, pt, color);
    }
    else {
      for (; pt.x<bounds.x+bounds.w; ++pt.x)
        (this->*pixelDelegate)(g, pt, color);
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Pixel delegates

void BrushPreview::savePixelDelegate(ui::Graphics* g, const gfx::Point& pt, gfx::Color color)
{
  if (m_clippingRegion.contains(pt)) {
    color_t c = g->getPixel(pt.x, pt.y);

    if (m_savedPixelsIterator < (int)m_savedPixels.size())
      m_savedPixels[m_savedPixelsIterator] = c;
    else
      m_savedPixels.push_back(c);

    ++m_savedPixelsIterator;
  }
}

void BrushPreview::drawPixelDelegate(ui::Graphics* gfx, const gfx::Point& pt, gfx::Color color)
{
  if (m_savedPixelsIterator < (int)m_savedPixels.size() &&
      m_clippingRegion.contains(pt)) {
    if (m_blackAndWhiteNegative) {
      int c = m_savedPixels[m_savedPixelsIterator];
      int r = gfx::getr(c);
      int g = gfx::getg(c);
      int b = gfx::getb(c);

      gfx->putPixel(color_utils::blackandwhite_neg(gfx::rgba(r, g, b)), pt.x, pt.y);
    }
    else {
      gfx->putPixel(color, pt.x, pt.y);
    }
    ++m_savedPixelsIterator;
  }
}

void BrushPreview::clearPixelDelegate(ui::Graphics* g, const gfx::Point& pt, gfx::Color color)
{
  if (m_savedPixelsIterator < (int)m_savedPixels.size()) {
    if (m_oldClippingRegion.contains(pt)) {
      if (m_clippingRegion.contains(pt))
        g->putPixel(m_savedPixels[m_savedPixelsIterator], pt.x, pt.y);
      ++m_savedPixelsIterator;
    }
  }

#if _DEBUG
  if (!(m_savedPixelsIterator <= m_savedPixelsLimit)) {
    TRACE("m_savedPixelsIterator <= m_savedPixelsLimit: %d <= %d failed\n",
          m_savedPixelsIterator, m_savedPixelsLimit);
  }
#endif
}

} // namespace app
