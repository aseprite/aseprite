// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

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
#include "app/ui/context_bar.h"
#include "app/ui/editor/editor.h"
#include "app/ui/editor/tool_loop_impl.h"
#include "app/ui_context.h"
#include "app/util/wrap_value.h"
#include "doc/algo.h"
#include "doc/blend_internals.h"
#include "doc/brush.h"
#include "doc/cel.h"
#include "doc/image_impl.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "render/render.h"
#include "os/display.h"
#include "ui/manager.h"
#include "ui/system.h"

namespace app {

using namespace doc;

BrushPreview::BrushPreview(Editor* editor)
  : m_editor(editor)
  , m_type(CROSSHAIR)
  , m_onScreen(false)
  , m_withModifiedPixels(false)
  , m_withRealPreview(false)
  , m_screenPosition(0, 0)
  , m_editorPosition(0, 0)
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

  const bool isFloodfill = m_editor->getCurrentEditorTool()->getPointShape(0)->isFloodFill();

  // Setup the cursor type depending on several factors (current tool,
  // foreground color, layer transparency, brush size, etc.).
  BrushRef brush = getCurrentBrush();
  color_t brush_color = getBrushColor(sprite, layer);
  color_t mask_index = sprite->transparentColor();

  if (ink->isSelection() || ink->isSlice()) {
    m_type = SELECTION_CROSSHAIR;
  }
  else if (
    (brush->type() == kImageBrushType ||
     ((isFloodfill ? 1: brush->size()) > (1.0 / m_editor->zoom().scale()))) &&
    (// Use cursor bounds for inks that are effects (eraser, blur, etc.)
     (ink->isEffect()) ||
     // or when the brush color is transparent and we are not in the background layer
     (!ink->isShading() &&
      (layer && !layer->isBackground()) &&
      ((sprite->pixelFormat() == IMAGE_INDEXED && brush_color == mask_index) ||
       (sprite->pixelFormat() == IMAGE_RGB && rgba_geta(brush_color) == 0) ||
       (sprite->pixelFormat() == IMAGE_GRAYSCALE && graya_geta(brush_color) == 0))))) {
    m_type = BRUSH_BOUNDARIES;
  }
  else {
    m_type = CROSSHAIR;
  }

  bool showPreview = false;
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
      showPreview = m_editor->getState()->requireBrushPreview();
      break;
  }

  if (m_type & SELECTION_CROSSHAIR)
    showPreview = false;

  // When the extra cel is locked (e.g. we are flashing the active
  // layer) we don't show the brush preview temporally.
  if (showPreview && m_editor->isExtraCelLocked()) {
    showPreview = false;
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
      showPreview = false;
    generateBoundaries();
  }

  // Draw pixel/brush preview
  if (showPreview) {
    gfx::Rect origBrushBounds = (isFloodfill ? gfx::Rect(0, 0, 1, 1): brush->bounds());
    gfx::Rect brushBounds = origBrushBounds;
    brushBounds.offset(spritePos);
    gfx::Rect extraCelBounds = brushBounds;

    // Tiled mode might require a bigger extra cel (to show the tiled)
    if (int(m_editor->docPref().tiled.mode()) & int(filters::TiledMode::X_AXIS)) {
      brushBounds.x = wrap_value(brushBounds.x, sprite->width());
      extraCelBounds.x = brushBounds.x;
      if ((extraCelBounds.x < 0 && extraCelBounds.x2() > 0) ||
          (extraCelBounds.x < sprite->width() && extraCelBounds.x2() > sprite->width())) {
        extraCelBounds.x = 0;
        extraCelBounds.w = sprite->width();
      }
    }
    if (int(m_editor->docPref().tiled.mode()) & int(filters::TiledMode::Y_AXIS)) {
      brushBounds.y = wrap_value(brushBounds.y, sprite->height());
      extraCelBounds.y = brushBounds.y;
      if ((extraCelBounds.y < 0 && extraCelBounds.y2() > 0) ||
          (extraCelBounds.y < sprite->height() && extraCelBounds.y2() > sprite->height())) {
        extraCelBounds.y = 0;
        extraCelBounds.h = sprite->height();
      }
    }

    // Create the extra cel to show the brush preview
    Site site = m_editor->getSite();
    Cel* cel = site.cel();

    int t, opacity = 255;
    if (cel) opacity = MUL_UN8(opacity, cel->opacity(), t);
    if (layer) opacity = MUL_UN8(opacity, static_cast<LayerImage*>(layer)->opacity(), t);

    if (!m_extraCel)
      m_extraCel.reset(new ExtraCel);
    m_extraCel->create(document->sprite(), extraCelBounds, site.frame(), opacity);
    m_extraCel->setType(render::ExtraType::NONE);
    m_extraCel->setBlendMode(
      (layer ? static_cast<LayerImage*>(layer)->blendMode():
               BlendMode::NORMAL));

    document->setExtraCel(m_extraCel);

    Image* extraImage = m_extraCel->image();
    extraImage->setMaskColor(mask_index);
    clear_image(extraImage,
                (extraImage->pixelFormat() == IMAGE_INDEXED ? mask_index: 0));

    if (layer) {
      render::Render().renderLayer(
        extraImage, layer, site.frame(),
        gfx::Clip(0, 0, extraCelBounds),
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
      sprite, gfx::Region(m_lastBounds = extraCelBounds),
      m_lastFrame = site.frame());

    m_withRealPreview = true;
  }

  // Save area and draw the cursor
  if (!(m_type & NATIVE_CROSSHAIR) ||
      (m_type & BRUSH_BOUNDARIES)) {
    ui::ScreenGraphics g;
    ui::SetClip clip(&g);
    gfx::Color uiCursorColor = color_utils::color_for_ui(appCursorColor);
    forEachBrushPixel(&g, m_screenPosition, spritePos, uiCursorColor, &BrushPreview::savePixelDelegate);
    forEachBrushPixel(&g, m_screenPosition, spritePos, uiCursorColor, &BrushPreview::drawPixelDelegate);
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

  // Get drawable region
  m_editor->getDrawableRegion(m_clippingRegion, ui::Widget::kCutTopWindows);

  // Remove the invalidated region in the editor.
  m_clippingRegion.createSubtraction(m_clippingRegion,
                                     m_editor->getUpdateRegion());

  if (m_withModifiedPixels) {
    // Restore pixels
    ui::ScreenGraphics g;
    ui::SetClip clip(&g);
    forEachBrushPixel(&g, m_screenPosition, m_editorPosition, gfx::ColorNone,
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
  int w = (isOnePixel ? 1: brushImage->width());
  int h = (isOnePixel ? 1: brushImage->height());

  m_brushGen = brush->gen();
  m_brushWidth = w;
  m_brushHeight = h;

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

void BrushPreview::forEachBrushPixel(
  ui::Graphics* g,
  const gfx::Point& screenPos,
  const gfx::Point& spritePos,
  gfx::Color color,
  PixelDelegate pixelDelegate)
{
  m_savedPixelsIterator = 0;

  if (m_type & CROSSHAIR)
    traceCrossPixels(g, screenPos, color, pixelDelegate);

  if (m_type & SELECTION_CROSSHAIR)
    traceSelectionCrossPixels(g, spritePos, color, 1, pixelDelegate);

  if (m_type & BRUSH_BOUNDARIES)
    traceBrushBoundaries(g, spritePos, color, pixelDelegate);

  // Depending on the editor zoom, maybe we need subpixel movement (a
  // little dot inside the active pixel)
  if (!(m_type & NATIVE_CROSSHAIR) &&
      m_editor->zoom().scale() >= 4.0) {
    (this->*pixelDelegate)(g, screenPos, color);
  }

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

//////////////////////////////////////////////////////////////////////
// Old Thick Cross

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

//////////////////////////////////////////////////////////////////////
// Current Brush Bounds

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
