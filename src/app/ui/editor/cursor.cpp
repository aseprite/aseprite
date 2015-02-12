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
#include "app/settings/settings.h"
#include "app/tools/ink.h"
#include "app/tools/tool.h"
#include "app/ui_context.h"
#include "app/util/boundary.h"
#include "base/memory.h"
#include "doc/algo.h"
#include "doc/brush.h"
#include "doc/image.h"
#include "doc/layer.h"
#include "doc/primitives.h"
#include "doc/sprite.h"
#include "ui/base.h"
#include "ui/system.h"
#include "ui/widget.h"

#include <algorithm>

#ifdef WIN32
#undef max
#undef min
#endif

namespace app {

using namespace ui;

// Returns true if the cursor of the editor needs subpixel movement.
#define IS_SUBPIXEL(editor)     ((editor)->m_zoom.scale() >= 4.0)

// Maximum quantity of colors to save pixels overlapped by the cursor.
#define MAX_SAVED   4096

static struct {
  int brush_type;
  int brush_size;
  int brush_angle;
  int nseg;
  BoundSeg* seg;
} cursor_bound = { 0, 0, 0, 0, NULL };

enum {
  CURSOR_THINCROSS   = 1,
  CURSOR_THICKCROSS  = 2,
  CURSOR_BRUSHBOUNDS = 4
};

static int cursor_type = CURSOR_THINCROSS;
static int cursor_negative;

static gfx::Color saved_pixel[MAX_SAVED];
static int saved_pixel_n;

// These clipping regions are shared between all editors, so we cannot
// make assumptions about their old state
static gfx::Region clipping_region;
static gfx::Region old_clipping_region;

static void generate_cursor_boundaries(Editor* editor);

static void trace_thincross_pixels(ui::Graphics* g, Editor* editor, const gfx::Point& pt, gfx::Color color, Editor::PixelDelegate pixel);
static void trace_thickcross_pixels(ui::Graphics* g, Editor* editor, const gfx::Point& pt, gfx::Color color, int thickness, Editor::PixelDelegate pixel);
static void trace_brush_bounds(ui::Graphics* g, Editor* editor, const gfx::Point& pt, gfx::Color color, Editor::PixelDelegate pixel);

static void savepixel(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);
static void drawpixel(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);
static void clearpixel(ui::Graphics* g, const gfx::Point& pt, gfx::Color color);

static color_t get_brush_color(Sprite* sprite, Layer* layer);

//////////////////////////////////////////////////////////////////////
// CURSOR COLOR
//////////////////////////////////////////////////////////////////////

static app::Color cursor_color;
static gfx::Color ui_cursor_color;
static bool is_cursor_mask;

static void update_cursor_color()
{
  ui_cursor_color = color_utils::color_for_ui(cursor_color);
  is_cursor_mask = (cursor_color.getType() == app::Color::MaskType);
}

app::Color Editor::get_cursor_color()
{
  return cursor_color;
}

void Editor::set_cursor_color(const app::Color& color)
{
  cursor_color = color;
  update_cursor_color();
}

//////////////////////////////////////////////////////////////////////
// Slots for App signals
//////////////////////////////////////////////////////////////////////

static int brush_size_thick = 0;
static Brush* current_brush = NULL;

static void on_palette_change_update_cursor_color()
{
  update_cursor_color();
}

static void on_brush_before_change()
{
  if (current_editor != NULL) {
    brush_size_thick = current_editor->cursorThick();
    if (brush_size_thick)
      current_editor->hideDrawingCursor();
  }
}

static void on_brush_after_change()
{
  if (current_editor != NULL) {
    // Show drawing cursor
    if (current_editor->sprite() && brush_size_thick > 0)
      current_editor->showDrawingCursor();
  }
}

static Brush* editor_get_current_brush(Editor* editor)
{
  // Create the current brush from settings
  tools::Tool* tool = editor->getCurrentEditorTool();
  IBrushSettings* brush_settings = UIContext::instance()
    ->settings()
    ->getToolSettings(tool)
    ->getBrush();

  ASSERT(brush_settings != NULL);

  if (!current_brush ||
      current_brush->type() != brush_settings->getType() ||
      current_brush->size() != brush_settings->getSize() ||
      current_brush->angle() != brush_settings->getAngle()) {
    delete current_brush;
    current_brush = new Brush(
      brush_settings->getType(),
      brush_settings->getSize(),
      brush_settings->getAngle());
  }

  return current_brush;
}

//////////////////////////////////////////////////////////////////////
// CURSOR
//////////////////////////////////////////////////////////////////////

void Editor::editor_cursor_init()
{
  // Cursor color
  set_cursor_color(get_config_color("Tools", "CursorColor", app::Color::fromMask()));

  App::instance()->PaletteChange.connect(&on_palette_change_update_cursor_color);
  App::instance()->BrushSizeBeforeChange.connect(&on_brush_before_change);
  App::instance()->BrushSizeAfterChange.connect(&on_brush_after_change);
  App::instance()->BrushAngleBeforeChange.connect(&on_brush_before_change);
  App::instance()->BrushAngleAfterChange.connect(&on_brush_after_change);
}

void Editor::editor_cursor_exit()
{
  set_config_color("Tools", "CursorColor", cursor_color);

  if (cursor_bound.seg != NULL)
    base_free(cursor_bound.seg);

  delete current_brush;
  current_brush = NULL;
}

// Draws the brush cursor inside the specified editor.
// Warning: You should clean the cursor before to use
// this routine with other editor.
//
// Note: x and y params are absolute positions of the mouse.
void Editor::drawBrushPreview(const gfx::Point& pos, bool refresh)
{
  ASSERT(m_cursorThick == 0);
  ASSERT(m_sprite != NULL);

  // Get drawable region
  getDrawableRegion(clipping_region, kCutTopWindows);

  // Get cursor color
  cursor_negative = is_cursor_mask;

  // Cursor in the screen (view)
  m_cursorScreen = pos;

  // Get cursor position in the editor
  gfx::Point spritePos = screenToEditor(pos);

  // Get the current tool
  tools::Tool* tool = getCurrentEditorTool();
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
    IToolSettings* tool_settings = UIContext::instance()
      ->settings()
      ->getToolSettings(tool);

    Brush* brush = editor_get_current_brush(this);
    gfx::Rect brushBounds = brush->bounds();

    // Create the extra cel to show the brush preview
    m_document->prepareExtraCel(
      gfx::Rect(brushBounds).offset(spritePos),
      tool_settings->getOpacity());

    // In 'indexed' images, if the current color is 0, we have to use
    // a different mask color (different from 0) to draw the extra layer
    if (brush_color == mask_color)
      mask_color = (mask_color == 0 ? 1: 0);

    Image* extraImage = m_document->getExtraCelImage();
    extraImage->setMaskColor(mask_color);
    draw_brush(extraImage, brush, -brushBounds.x, -brushBounds.y,
      brush_color, extraImage->maskColor());

    if (refresh) {
      m_document->notifySpritePixelsModified
        (m_sprite,
         gfx::Region(gfx::Rect(
             spritePos.x+brushBounds.x,
             spritePos.y+brushBounds.y,
             brushBounds.w, brushBounds.h)));
    }
  }

  // Save area and draw the cursor
  if (refresh) {
    ScreenGraphics g;
    SetClip clip(&g, gfx::Rect(0, 0, g.width(), g.height()));

    forEachBrushPixel(&g, m_cursorScreen, spritePos, ui_cursor_color, savepixel);
    forEachBrushPixel(&g, m_cursorScreen, spritePos, ui_cursor_color, drawpixel);
  }

  // Cursor thickness
  m_cursorThick = 1;

  // Cursor in the editor (model)
  m_cursorEditor = spritePos;

  // Save the clipping-region to know where to clean the pixels
  old_clipping_region = clipping_region;
}

void Editor::moveBrushPreview(const gfx::Point& pos, bool refresh)
{
  ASSERT(m_sprite != NULL);

  gfx::Point oldScreenPos = m_cursorScreen;
  gfx::Point oldEditorPos = m_cursorEditor;

  clearBrushPreview(false);
  drawBrushPreview(pos, false);

  gfx::Point newEditorPos = m_cursorEditor;

  if (refresh) {
    // Restore pixels
    {
      ScreenGraphics g;
      SetClip clip(&g, gfx::Rect(0, 0, g.width(), g.height()));

      forEachBrushPixel(&g, oldScreenPos, oldEditorPos, gfx::ColorNone, clearpixel);
    }

    if (cursor_type & CURSOR_THINCROSS && m_state->requireBrushPreview()) {
      Brush* brush = editor_get_current_brush(this);
      gfx::Rect brushBounds = brush->bounds();
      gfx::Rect rc1(oldEditorPos.x+brushBounds.x, oldEditorPos.y+brushBounds.y, brushBounds.w, brushBounds.h);
      gfx::Rect rc2(newEditorPos.x+brushBounds.x, newEditorPos.y+brushBounds.y, brushBounds.w, brushBounds.h);
      m_document->notifySpritePixelsModified
        (m_sprite, gfx::Region(rc1.createUnion(rc2)));
    }

    // Save area and draw the cursor
    ScreenGraphics g;
    forEachBrushPixel(&g, m_cursorScreen, newEditorPos, ui_cursor_color, savepixel);
    forEachBrushPixel(&g, m_cursorScreen, newEditorPos, ui_cursor_color, drawpixel);
  }
}

/**
 * Cleans the brush cursor from the specified editor.
 *
 * The mouse position is got from the last
 * call to @c drawBrushPreview. So you must
 * to use this routine only if you called
 * @c drawBrushPreview before with the specified
 * editor @a widget.
 *
 * @param widget The editor widget
 *
 * @see drawBrushPreview
 */
void Editor::clearBrushPreview(bool refresh)
{
  ASSERT(m_cursorThick != 0);
  ASSERT(m_sprite != NULL);

  getDrawableRegion(clipping_region, kCutTopWindows);

  gfx::Point pos = m_cursorEditor;

  if (refresh) {
    // Restore pixels
    ScreenGraphics g;
    SetClip clip(&g, gfx::Rect(0, 0, g.width(), g.height()));

    forEachBrushPixel(&g, m_cursorScreen, pos, gfx::ColorNone, clearpixel);
  }

  // Clean pixel/brush preview
  if (cursor_type & CURSOR_THINCROSS && m_state->requireBrushPreview()) {
    Brush* brush = editor_get_current_brush(this);
    gfx::Rect brushBounds = brush->bounds();

    m_document->prepareExtraCel(
      gfx::Rect(brushBounds).offset(pos),
      0); // Opacity = 0

    if (refresh) {
      m_document->notifySpritePixelsModified
        (m_sprite,
         gfx::Region(gfx::Rect(
             pos.x+brushBounds.x,
             pos.y+brushBounds.y,
             brushBounds.w, brushBounds.h)));
    }
  }

  m_cursorThick = 0;

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
  tools::Tool* tool = editor->getCurrentEditorTool();

  IBrushSettings* brush_settings = NULL;
  if (tool)
    brush_settings = UIContext::instance()
      ->settings()
      ->getToolSettings(tool)
      ->getBrush();

  if (cursor_bound.seg == NULL ||
      cursor_bound.brush_type != brush_settings->getType() ||
      cursor_bound.brush_size != brush_settings->getSize() ||
      cursor_bound.brush_angle != brush_settings->getAngle()) {
    cursor_bound.brush_type = brush_settings->getType();
    cursor_bound.brush_size = brush_settings->getSize();
    cursor_bound.brush_angle = brush_settings->getAngle();

    if (cursor_bound.seg != NULL)
      base_free(cursor_bound.seg);

    Brush* brush;

    if (brush_settings) {
      brush = new Brush(brush_settings->getType(),
                    brush_settings->getSize(),
                    brush_settings->getAngle());
    }
    else
      brush = new Brush();

    cursor_bound.seg = find_mask_boundary(brush->image(),
                                          &cursor_bound.nseg,
                                          IgnoreBounds, 0, 0, 0, 0);
    delete brush;
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
  const gfx::Point& pos, gfx::Color color, Editor::PixelDelegate pixelDelegate)
{
  Data data = { g, color, pixelDelegate };
  gfx::Point pt1, pt2;
  BoundSeg* seg;
  int c;

  for (c=0; c<cursor_bound.nseg; c++) {
    seg = cursor_bound.seg+c;

    pt1.x = pos.x + seg->x1 - cursor_bound.brush_size/2;
    pt1.y = pos.y + seg->y1 - cursor_bound.brush_size/2;
    pt2.x = pos.x + seg->x2 - cursor_bound.brush_size/2;
    pt2.y = pos.y + seg->y2 - cursor_bound.brush_size/2;

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
  if (saved_pixel_n < MAX_SAVED && clipping_region.contains(pt))
    saved_pixel[saved_pixel_n++] = g->getPixel(pt.x, pt.y);
}

static void drawpixel(ui::Graphics* graphics, const gfx::Point& pt, gfx::Color color)
{
  if (saved_pixel_n < MAX_SAVED && clipping_region.contains(pt)) {
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
  if (saved_pixel_n < MAX_SAVED) {
    if (clipping_region.contains(pt))
      g->putPixel(saved_pixel[saved_pixel_n++], pt.x, pt.y);
    else if (!old_clipping_region.isEmpty() &&
             old_clipping_region.contains(pt))
      saved_pixel_n++;
  }
}

static color_t get_brush_color(Sprite* sprite, Layer* layer)
{
  app::Color c = UIContext::instance()->settings()->getFgColor();
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
