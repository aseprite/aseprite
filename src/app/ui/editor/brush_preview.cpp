// Aseprite
// Copyright (C) 2019-2021  Igara Studio S.A.
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
#include "app/site.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/intertwine.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool.h"
#include "app/tools/tool_loop.h"
#include "app/ui/color_bar.h"
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

namespace app {

using namespace doc;

BrushPreview::BrushPreview(Editor* editor)
  : m_editor(editor)
{
}

BrushPreview::~BrushPreview()
{
  m_cursor.reset();
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

  // Cursor in the screen (view)
  m_screenPosition = screenPos;

  // Get cursor position in the editor
  gfx::Point spritePos = m_editor->screenToEditor(screenPos);

  // Get the current tool
  tools::Ink* ink = m_editor->getCurrentEditorInk();

  // Get current tilemap mode
  TilemapMode tilemapMode = ColorBar::instance()->tilemapMode();

  const bool isFloodfill = m_editor->getCurrentEditorTool()->getPointShape(0)->isFloodFill();
  const auto& dynamics = App::instance()->contextBar()->getDynamics();

  // Setup the cursor type depending on several factors (current tool,
  // foreground color, layer transparency, brush size, etc.).
  BrushRef brush = getCurrentBrush();
  color_t brush_color = getBrushColor(sprite, layer);
  color_t mask_index = sprite->transparentColor();

  if (brush->type() != doc::kImageBrushType &&
      (dynamics.size != tools::DynamicSensor::Static ||
       dynamics.angle != tools::DynamicSensor::Static)) {
    brush.reset(
      new Brush(
        brush->type(),
        (dynamics.size != tools::DynamicSensor::Static ? dynamics.minSize: brush->size()),
        (dynamics.angle != tools::DynamicSensor::Static ? dynamics.minAngle: brush->angle())));
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
          break;
      }
      break;
  }

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
    generateBoundaries();

  // Draw pixel/brush preview
  if (showPreview) {
    Site site = m_editor->getSite();

    // TODO add support for "tile-brushes"
    gfx::Rect origBrushBounds =
      (isFloodfill || site.tilemapMode() == TilemapMode::Tiles ? gfx::Rect(0, 0, 1, 1):
                                                                 brush->bounds());
    gfx::Rect brushBounds = origBrushBounds;
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
    if (site.tilemapMode() == TilemapMode::Tiles) {
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
      site.tilemapMode(),
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
        loop->getIntertwine()->prepareIntertwine();
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

    createNativeCursor();
    if (m_cursor)
      forEachLittleCrossPixel(&g, m_screenPosition, uiCursorColor, &BrushPreview::putPixelInCursorDelegate);

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
  //  m_editor->m_editor->display()->nativeWindow()->setNativeMouseCursor(os::NativeCursor::Hidden);

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

void BrushPreview::generateBoundaries()
{
  BrushRef brush = getCurrentBrush();

  if (!m_brushBoundaries.isEmpty() &&
      m_brushGen == brush->gen())
    return;

  const bool isOnePixel =
    (m_editor->getCurrentEditorTool()->getPointShape(0)->isPixel() ||
     m_editor->getCurrentEditorTool()->getPointShape(0)->isFloodFill());
  Image* brushImage = brush->image();
  m_brushGen = brush->gen();

  Image* mask = nullptr;
  bool deleteMask = true;
  if (isOnePixel) {
    mask = Image::create(IMAGE_BITMAP, 1, 1);
    mask->putPixel(0, 0, (color_t)1);
  }
  else if (brushImage->pixelFormat() != IMAGE_BITMAP) {
    ASSERT(brush->maskBitmap());
    deleteMask = false;
    mask = brush->maskBitmap();
  }

  m_brushBoundaries.regen(mask ? mask: brushImage);
  if (!isOnePixel)
    m_brushBoundaries.offset(-brush->center().x,
                             -brush->center().y);

  if (deleteMask)
    delete mask;
}

void BrushPreview::createNativeCursor()
{
  gfx::Rect cursorBounds;

  if (m_type & CROSSHAIR) {
    cursorBounds |= gfx::Rect(-3, -3, 7, 7);
    m_cursorCenter = -cursorBounds.origin();
  }
  // Special case of a cursor for one pixel
  else if (!(m_type & NATIVE_CROSSHAIR) &&
           m_editor->zoom().scale() >= 4.0) {
    cursorBounds = gfx::Rect(0, 0, 1, 1);
    m_cursorCenter = gfx::Point(0, 0);
  }

  if (m_cursor) {
    if (m_cursor->width() != cursorBounds.w ||
        m_cursor->height() != cursorBounds.h) {
      m_cursor.reset();
    }
  }

  if (cursorBounds.isEmpty()) {
    ASSERT(!m_cursor);
    m_editor->display()->nativeWindow()->setNativeMouseCursor(os::NativeCursor::Hidden);
    return;
  }

  if (!m_cursor) {
    m_cursor = os::instance()->makeRgbaSurface(cursorBounds.w, cursorBounds.h);

    // Cannot clear the cursor on each iteration because it can
    // generate a flicker effect when zooming in the same mouse
    // position. That's strange.
    m_cursor->clear();
  }
}

void BrushPreview::forEachLittleCrossPixel(
  ui::Graphics* g,
  const gfx::Point& screenPos,
  gfx::Color color,
  PixelDelegate pixelDelegate)
{
  if (m_type & CROSSHAIR)
    traceCrossPixels(g, screenPos, color, pixelDelegate);

  // Depending on the editor zoom, maybe we need subpixel movement (a
  // little dot inside the active pixel)
  if (!(m_type & NATIVE_CROSSHAIR) &&
      m_editor->zoom().scale() >= 4.0) {
    (this->*pixelDelegate)(g, screenPos, color);
  }
  else {
    // We'll remove the pixel (as we didn't called Surface::clear() to
    // avoid a flickering issue when zooming in the same mouse
    // position).
    base::ScopedValue<bool> restore(m_blackAndWhiteNegative, false,
                                    m_blackAndWhiteNegative);
    (this->*pixelDelegate)(g, screenPos, gfx::ColorNone);
  }

  if (m_cursor) {
    m_editor->display()->nativeWindow()->setNativeMouseCursor(
      m_cursor.get(),
      m_cursorCenter,
      m_editor->display()->nativeWindow()->scale());
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

void BrushPreview::traceCrossPixels(
  ui::Graphics* g,
  const gfx::Point& pt, gfx::Color color,
  PixelDelegate pixelDelegate)
{
  static int cross[7*7] = {
    0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0,
  };
  gfx::Point out;
  int u, v;

  for (v=0; v<7; v++) {
    for (u=0; u<7; u++) {
      if (cross[v*7+u]) {
        out.x = pt.x-3+u;
        out.y = pt.y-3+v;
        (this->*pixelDelegate)(g, out, color);
      }
    }
  }
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

void BrushPreview::putPixelInCursorDelegate(ui::Graphics* g, const gfx::Point& pt, gfx::Color color)
{
  ASSERT(m_cursor);

  if (!m_clippingRegion.contains(pt))
    return;

  if (m_blackAndWhiteNegative) {
    color_t c = g->getPixel(pt.x, pt.y);
    int r = gfx::getr(c);
    int g = gfx::getg(c);
    int b = gfx::getb(c);

    m_cursor->putPixel(color_utils::blackandwhite_neg(gfx::rgba(r, g, b)),
                       pt.x - m_screenPosition.x + m_cursorCenter.x,
                       pt.y - m_screenPosition.y + m_cursorCenter.y);
  }
  else {
    m_cursor->putPixel(color,
                       pt.x - m_screenPosition.x + m_cursorCenter.x,
                       pt.y - m_screenPosition.y + m_cursorCenter.y);
  }
}

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
