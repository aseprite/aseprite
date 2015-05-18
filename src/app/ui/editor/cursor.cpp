// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/editor/editor.h"

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/ini_file.h"
#include "app/modules/editors.h"
#include "app/tools/controller.h"
#include "app/tools/ink.h"
#include "app/tools/intertwine.h"
#include "app/tools/point_shape.h"
#include "app/tools/tool.h"
#include "app/tools/tool_loop.h"
#include "app/ui/context_bar.h"
#include "app/ui/editor/tool_loop_impl.h"
#include "app/ui/main_window.h"
#include "app/ui_context.h"
#include "app/util/boundary.h"
#include "base/bind.h"
#include "base/memory.h"
#include "doc/algo.h"
#include "doc/brush.h"
#include "doc/cel.h"
#include "doc/image.h"
#include "doc/image_bits.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "render/render.h"
#include "ui/base.h"
#include "ui/system.h"
#include "ui/widget.h"

#include <algorithm>

namespace app {

using namespace ui;

// Returns true if the cursor of the editor needs subpixel movement.
#define IS_SUBPIXEL(editor)     ((editor)->m_zoom.scale() >= 4.0)

static struct {
  int nseg;
  BoundSeg* seg;
  Image* brush_image;
  int brush_width;
  int brush_height;
} cursor_bound = { 0, nullptr, nullptr, 0, 0 };

enum {
  CURSOR_THINCROSS   = 1,
  CURSOR_THICKCROSS  = 2,
  CURSOR_BRUSHBOUNDS = 4
};

static int cursor_type = CURSOR_THINCROSS;
static bool cursor_negative;

static std::vector<gfx::Color> saved_pixel;
static int saved_pixel_n;

// These clipping regions are shared between all editors, so we cannot
// make assumptions about their old state
static gfx::Region clipping_region;
static gfx::Region old_clipping_region;

static gfx::Rect lastBrushBounds;

static void generate_cursor_boundaries(Editor* editor);

static void trace_thincross_pixels(ui::Graphics* g, Editor* editor, const gfx::Point& pt, gfx::Color color, Editor::PixelDelegate pixel);
static void trace_thickcross_pixels(ui::Graphics* g, Editor* editor, const gfx::Point& pt, gfx::Color color, int thickness, Editor::PixelDelegate pixel);
static void trace_brush_bounds(ui::Graphics* g, Editor* editor, gfx::Point pos, gfx::Color color, Editor::PixelDelegate pixel);

static void savepixel(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);
static void drawpixel(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);
static void clearpixel(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);

static color_t get_brush_color(Sprite* sprite, Layer* layer);

//////////////////////////////////////////////////////////////////////
// CURSOR COLOR
//////////////////////////////////////////////////////////////////////

static gfx::Color ui_cursor_color;
static bool is_cursor_mask;

static void update_cursor_color()
{
  app::Color color = Preferences::instance().editor.cursorColor();

  ui_cursor_color = color_utils::color_for_ui(color);
  is_cursor_mask = (color.getType() == app::Color::MaskType);
}

static Brush* get_current_brush()
{
  return App::instance()->getMainWindow()->getContextBar()->activeBrush().get();
}

void Editor::initEditorCursor()
{
  update_cursor_color();

  Preferences::instance().editor.cursorColor.AfterChange.connect(
    Bind<void>(&update_cursor_color));

  App::instance()->PaletteChange.connect(&update_cursor_color);
}

void Editor::exitEditorCursor()
{
  if (cursor_bound.seg)
    base_free(cursor_bound.seg);
}

// Draws the brush cursor in the specified absolute mouse position
// given in 'pos' param.  Warning: You should clean the cursor before
// to use this routine with other editor.
void Editor::drawBrushPreview(const gfx::Point& pos)
{
  ASSERT(!m_cursorOnScreen);
  ASSERT(m_sprite);

  // Get drawable region
  getDrawableRegion(clipping_region, kCutTopWindows);

  // Get cursor color
  cursor_negative = is_cursor_mask;

  // Cursor in the screen (view)
  m_cursorScreen = pos;

  // Get cursor position in the editor
  gfx::Point spritePos = screenToEditor(pos);

  // Get the current tool
  tools::Ink* ink = getCurrentEditorInk();

  // Setup the cursor type debrushding of several factors (current tool,
  // foreground color, and layer transparency).
  color_t brush_color = get_brush_color(m_sprite, m_layer);
  color_t mask_color = m_sprite->transparentColor();

  if (ink->isSelection() || ink->isSlice()) {
    cursor_type = CURSOR_THICKCROSS;
  }
  else if (
    // Use cursor bounds for inks that are effects (eraser, blur, etc.)
    (ink->isEffect()) ||
    // or when the brush color is transparent and we are not in the background layer
    (m_layer && !m_layer->isBackground() &&
     brush_color == mask_color)) {
    cursor_type = CURSOR_BRUSHBOUNDS;
  }
  else {
    cursor_type = CURSOR_THINCROSS;
  }

  // For cursor type 'bounds' we have to generate cursor boundaries
  if (cursor_type & CURSOR_BRUSHBOUNDS)
    generate_cursor_boundaries(this);

  // Draw pixel/brush preview
  if (cursor_type & CURSOR_THINCROSS && m_state->requireBrushPreview()) {
    Brush* brush = get_current_brush();
    gfx::Rect brushBounds = brush->bounds();
    brushBounds.offset(spritePos);

    // Create the extra cel to show the brush preview
    Site site = getSite();
    Cel* cel = site.cel();

    m_document->prepareExtraCel(
      brushBounds,
      cel ? cel->opacity(): 255);
    m_document->setExtraCelType(render::ExtraType::NONE);
    m_document->setExtraCelBlendMode(
      m_layer ? static_cast<LayerImage*>(m_layer)->getBlendMode(): BLEND_MODE_NORMAL);

    // In 'indexed' images, if the current color is 0, we have to use
    // a different mask color (different from 0) to draw the extra layer
    if (brush_color == mask_color)
      mask_color = (mask_color == 0 ? 1: 0);

    Image* extraImage = m_document->getExtraCelImage();
    extraImage->setMaskColor(mask_color);
    clear_image(extraImage, mask_color);

    if (m_layer) {
      render::Render().renderLayer(
        extraImage, m_layer, m_frame,
        gfx::Clip(0, 0, brushBounds),
        BLEND_MODE_COPY);

      // This extra cel is a patch for the current layer/frame
      m_document->setExtraCelType(render::ExtraType::PATCH);
    }

    tools::ToolLoop* loop = create_tool_loop_preview(
      this, UIContext::instance(), extraImage,
      -gfx::Point(brushBounds.x,
                  brushBounds.y));

    if (loop) {
      loop->getInk()->prepareInk(loop);
      loop->getIntertwine()->prepareIntertwine();
      loop->getController()->prepareController();
      loop->getPointShape()->preparePointShape(loop);
      loop->getPointShape()->transformPoint(
        loop, -brush->bounds().x, -brush->bounds().y);
      delete loop;
    }

    m_document->notifySpritePixelsModified(
      m_sprite,
      gfx::Region(lastBrushBounds = brushBounds));
  }

  // Save area and draw the cursor
  {
    ScreenGraphics g;
    SetClip clip(&g, gfx::Rect(0, 0, g.width(), g.height()));

    forEachBrushPixel(&g, m_cursorScreen, spritePos, ui_cursor_color, savepixel);
    forEachBrushPixel(&g, m_cursorScreen, spritePos, ui_cursor_color, drawpixel);
  }

  // Cursor in the editor (model)
  m_cursorOnScreen = true;
  m_cursorEditor = spritePos;

  // Save the clipping-region to know where to clean the pixels
  old_clipping_region = clipping_region;
}

// Cleans the brush cursor from the specified editor.
//
// The mouse position is got from the last call to drawBrushPreview()
// (m_cursorEditor). So you must to use this routine only if you
// called drawBrushPreview() before.
void Editor::clearBrushPreview()
{
  ASSERT(m_cursorOnScreen);
  ASSERT(m_sprite);

  getDrawableRegion(clipping_region, kCutTopWindows);

  gfx::Point pos = m_cursorEditor;

  {
    // Restore pixels
    ScreenGraphics g;
    SetClip clip(&g, gfx::Rect(0, 0, g.width(), g.height()));

    forEachBrushPixel(&g, m_cursorScreen, pos, gfx::ColorNone, clearpixel);
  }

  // Clean pixel/brush preview
  if (cursor_type & CURSOR_THINCROSS && m_state->requireBrushPreview()) {
    m_document->destroyExtraCel();
    m_document->notifySpritePixelsModified(
      m_sprite, gfx::Region(lastBrushBounds));
  }

  m_cursorOnScreen = false;

  clipping_region.clear();
  old_clipping_region.clear();
}

// Returns true if the cursor to draw in the editor has subpixel
// movement (a little pixel of the screen that indicates where is the
// mouse inside the pixel of the sprite).
bool Editor::doesBrushPreviewNeedSubpixel()
{
  return IS_SUBPIXEL(this);
}

//////////////////////////////////////////////////////////////////////

static void generate_cursor_boundaries(Editor* editor)
{
  Brush* brush = get_current_brush();

  if (!cursor_bound.seg || cursor_bound.brush_image != brush->image()) {
    Image* brush_image = brush->image();
    int w = brush_image->width();
    int h = brush_image->height();

    cursor_bound.brush_image = brush_image;
    cursor_bound.brush_width = w;
    cursor_bound.brush_height = h;

    if (cursor_bound.seg)
      base_free(cursor_bound.seg);

    ImageRef mask;
    if (brush_image->pixelFormat() != IMAGE_BITMAP) {
      mask.reset(Image::create(IMAGE_BITMAP, w, h));

      LockImageBits<BitmapTraits> bits(mask.get());
      auto pos = bits.begin();
      for (int v=0; v<h; ++v) {
        for (int u=0; u<w; ++u) {
          *pos = get_pixel(brush_image, u, v);
          ++pos;
        }
      }
    }

    cursor_bound.seg = find_mask_boundary(
      (mask ? mask.get(): brush_image),
      &cursor_bound.nseg,
      IgnoreBounds, 0, 0, 0, 0);
  }
}

void Editor::forEachBrushPixel(
  ui::Graphics* g,
  const gfx::Point& screenPos,
  const gfx::Point& spritePos,
  gfx::Color color,
  Editor::PixelDelegate pixelDelegate)
{
  saved_pixel_n = 0;

  if (cursor_type & CURSOR_THINCROSS)
    trace_thincross_pixels(g, this, screenPos, color, pixelDelegate);

  if (cursor_type & CURSOR_THICKCROSS)
    trace_thickcross_pixels(g, this, spritePos, color, 1, pixelDelegate);

  if (cursor_type & CURSOR_BRUSHBOUNDS)
    trace_brush_bounds(g, this, spritePos, color, pixelDelegate);

  if (IS_SUBPIXEL(this)) {
    pixelDelegate(g, screenPos, color);
  }
}

//////////////////////////////////////////////////////////////////////
// New Thin Cross

static void trace_thincross_pixels(ui::Graphics* g, Editor* editor,
  const gfx::Point& pt, gfx::Color color, Editor::PixelDelegate pixelDelegate)
{
  static int cursor_cross[7*7] = {
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
      if (cursor_cross[v*7+u]) {
        out.x = pt.x-3+u;
        out.y = pt.y-3+v;
        pixelDelegate(g, out, color);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Old Thick Cross

static void trace_thickcross_pixels(ui::Graphics* g, Editor* editor,
  const gfx::Point& pt, gfx::Color color, int thickness, Editor::PixelDelegate pixelDelegate)
{
  static int cursor_cross[6*6] = {
    0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0,
  };
  gfx::Point out, outpt = editor->editorToScreen(pt);
  int u, v;
  int size = editor->zoom().apply(thickness/2);
  int size2 = editor->zoom().apply(thickness);
  if (size2 == 0) size2 = 1;

  for (v=0; v<6; v++) {
    for (u=0; u<6; u++) {
      if (!cursor_cross[v*6+u])
        continue;

      out = outpt;
      out.x += ((u<3) ? u-size-3: u-size-3+size2);
      out.y += ((v<3) ? v-size-3: v-size-3+size2);

      pixelDelegate(g, out, color);
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Current Brush Bounds

struct Data {
  ui::Graphics* g;
  gfx::Color color;
  Editor::PixelDelegate pixelDelegate;
};

static void algo_line_proxy(int x, int y, void* _data)
{
  Data* data = (Data*)_data;
  data->pixelDelegate(data->g, gfx::Point(x, y), data->color);
}

static void trace_brush_bounds(ui::Graphics* g, Editor* editor,
  gfx::Point pos, gfx::Color color, Editor::PixelDelegate pixelDelegate)
{
  Data data = { g, color, pixelDelegate };
  gfx::Point pt1, pt2;
  BoundSeg* seg;
  int c;

  pos.x -= cursor_bound.brush_width/2;
  pos.y -= cursor_bound.brush_height/2;

  for (c=0; c<cursor_bound.nseg; c++) {
    seg = cursor_bound.seg+c;

    pt1.x = pos.x + seg->x1;
    pt1.y = pos.y + seg->y1;
    pt2.x = pos.x + seg->x2;
    pt2.y = pos.y + seg->y2;

    pt1 = editor->editorToScreen(pt1);
    pt2 = editor->editorToScreen(pt2);

    if (seg->open) {            // Outside
      if (pt1.x == pt2.x) {
        pt1.x--;
        pt2.x--;
        pt2.y--;
      }
      else {
        pt1.y--;
        pt2.y--;
        pt2.x--;
      }
    }
    else {
      if (pt1.x == pt2.x) {
        pt2.y--;
      }
      else {
        pt2.x--;
      }
    }

    doc::algo_line(pt1.x, pt1.y, pt2.x, pt2.y, (void*)&data, algo_line_proxy);
  }
}

//////////////////////////////////////////////////////////////////////
// Helpers

static void savepixel(ui::Graphics* g, const gfx::Point& pt, gfx::Color color)
{
  if (clipping_region.contains(pt)) {
    color_t c = g->getPixel(pt.x, pt.y);

    if (saved_pixel_n < (int)saved_pixel.size())
      saved_pixel[saved_pixel_n] = c;
    else
      saved_pixel.push_back(c);

    ++saved_pixel_n;
  }
}

static void drawpixel(ui::Graphics* graphics, const gfx::Point& pt, gfx::Color color)
{
  if (saved_pixel_n < (int)saved_pixel.size() && clipping_region.contains(pt)) {
    if (cursor_negative) {
      int c = saved_pixel[saved_pixel_n++];
      int r = gfx::getr(c);
      int g = gfx::getg(c);
      int b = gfx::getb(c);

      graphics->putPixel(color_utils::blackandwhite_neg(gfx::rgba(r, g, b)), pt.x, pt.y);
    }
    else {
      graphics->putPixel(color, pt.x, pt.y);
    }
  }
}

static void clearpixel(ui::Graphics* g, const gfx::Point& pt, gfx::Color color)
{
  if (saved_pixel_n < (int)saved_pixel.size()) {
    if (clipping_region.contains(pt))
      g->putPixel(saved_pixel[saved_pixel_n++], pt.x, pt.y);
    else if (!old_clipping_region.isEmpty() &&
             old_clipping_region.contains(pt))
      saved_pixel_n++;
  }
}

static color_t get_brush_color(Sprite* sprite, Layer* layer)
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

} // namespace app
