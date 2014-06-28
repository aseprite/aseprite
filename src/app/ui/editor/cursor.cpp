/* Aseprite
 * Copyright (C) 2001-2014  David Capello
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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
#include "raster/algo.h"
#include "raster/brush.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/primitives.h"
#include "raster/sprite.h"
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
#define IS_SUBPIXEL(editor)     ((editor)->m_zoom >= 2)

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

static ui::Color saved_pixel[MAX_SAVED];
static int saved_pixel_n;

// These clipping regions are shared between all editors, so we cannot
// make assumptions about their old state
static gfx::Region clipping_region;
static gfx::Region old_clipping_region;

static void generate_cursor_boundaries();

static void trace_thincross_pixels(ui::Graphics* g, Editor* editor, int x, int y, ui::Color color, Editor::PixelDelegate pixel);
static void trace_thickcross_pixels(ui::Graphics* g, Editor* editor, int x, int y, ui::Color color, int thickness, Editor::PixelDelegate pixel);
static void trace_brush_bounds(ui::Graphics* g, Editor* editor, int x, int y, ui::Color color, Editor::PixelDelegate pixel);

static void savepixel(ui::Graphics* g, int x, int y, ui::Color color);
static void drawpixel(ui::Graphics* g, int x, int y, ui::Color color);
static void clearpixel(ui::Graphics* g, int x, int y, ui::Color color);

static color_t get_brush_color(Sprite* sprite, Layer* layer);

//////////////////////////////////////////////////////////////////////
// CURSOR COLOR
//////////////////////////////////////////////////////////////////////

static app::Color cursor_color;
static ui::Color ui_cursor_color;
static bool is_cursor_mask;

static void update_cursor_color()
{
  if (ji_screen)
    ui_cursor_color = color_utils::color_for_ui(cursor_color);
  else
    ui_cursor_color = 0;

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
    brush_size_thick = current_editor->getCursorThick();
    if (brush_size_thick)
      current_editor->hideDrawingCursor();
  }
}

static void on_brush_after_change()
{
  if (current_editor != NULL) {
    // Show drawing cursor
    if (current_editor->getSprite() && brush_size_thick > 0)
      current_editor->showDrawingCursor();
  }
}

static Brush* editor_get_current_brush()
{
  // Create the current brush from settings
  tools::Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  IBrushSettings* brush_settings = UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool)
    ->getBrush();

  ASSERT(brush_settings != NULL);

  if (!current_brush ||
      current_brush->get_type() != brush_settings->getType() ||
      current_brush->get_size() != brush_settings->getSize() ||
      current_brush->get_angle() != brush_settings->getAngle()) {
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
void Editor::drawBrushPreview(int x, int y, bool refresh)
{
  ASSERT(m_cursor_thick == 0);
  ASSERT(m_sprite != NULL);

  // Get drawable region
  getDrawableRegion(clipping_region, kCutTopWindows);

  // Get cursor color
  cursor_negative = is_cursor_mask;

  // Cursor in the screen (view)
  m_cursor_screen_x = x;
  m_cursor_screen_y = y;

  // Get cursor position in the editor
  screenToEditor(x, y, &x, &y);

  // Get the current tool
  tools::Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  // Setup the cursor type debrushding of several factors (current tool,
  // foreground color, and layer transparency).
  color_t brush_color = get_brush_color(m_sprite, m_layer);
  color_t mask_color = m_sprite->getTransparentColor();

  if (current_tool->getInk(0)->isSelection() ||
      current_tool->getInk(0)->isSlice()) {
    cursor_type = CURSOR_THICKCROSS;
  }
  else if (
    // Use cursor bounds for inks that are effects (eraser, blur, etc.)
    (current_tool->getInk(0)->isEffect()) ||
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
    generate_cursor_boundaries();

  // Draw pixel/brush preview
  if (cursor_type & CURSOR_THINCROSS && m_state->requireBrushPreview()) {
    IToolSettings* tool_settings = UIContext::instance()
      ->getSettings()
      ->getToolSettings(current_tool);

    Brush* brush = editor_get_current_brush();
    gfx::Rect brushBounds = brush->getBounds();

    // Create the extra cel to show the brush preview
    m_document->prepareExtraCel(x+brushBounds.x, y+brushBounds.y,
                                brushBounds.w, brushBounds.h,
                                tool_settings->getOpacity());

    // In 'indexed' images, if the current color is 0, we have to use
    // a different mask color (different from 0) to draw the extra layer
    if (brush_color == mask_color)
      mask_color = (mask_color == 0 ? 1: 0);

    Image* extraImage = m_document->getExtraCelImage();
    extraImage->setMaskColor(mask_color);
    draw_brush(extraImage, brush, -brushBounds.x, -brushBounds.y,
      brush_color, extraImage->getMaskColor());

    if (refresh) {
      m_document->notifySpritePixelsModified
        (m_sprite,
         gfx::Region(gfx::Rect(x+brushBounds.x,
                               y+brushBounds.y,
                               brushBounds.w, brushBounds.h)));
    }
  }

  // Save area and draw the cursor
  if (refresh) {
    ScreenGraphics g;
    SetClip clip(&g, gfx::Rect(0, 0, g.width(), g.height()));

    forEachBrushPixel(&g, m_cursor_screen_x, m_cursor_screen_y, x, y, ui_cursor_color, savepixel);
    forEachBrushPixel(&g, m_cursor_screen_x, m_cursor_screen_y, x, y, ui_cursor_color, drawpixel);
  }

  // Cursor thickness
  m_cursor_thick = 1;

  // Cursor in the editor (model)
  m_cursor_editor_x = x;
  m_cursor_editor_y = y;

  // Save the clipping-region to know where to clean the pixels
  old_clipping_region = clipping_region;
}

void Editor::moveBrushPreview(int x, int y, bool refresh)
{
  ASSERT(m_sprite != NULL);

  int old_screen_x = m_cursor_screen_x;
  int old_screen_y = m_cursor_screen_y;
  int old_x = m_cursor_editor_x;
  int old_y = m_cursor_editor_y;

  clearBrushPreview(false);
  drawBrushPreview(x, y, false);

  int new_x = m_cursor_editor_x;
  int new_y = m_cursor_editor_y;

  if (refresh) {
    // Restore pixels
    {
      ScreenGraphics g;
      SetClip clip(&g, gfx::Rect(0, 0, g.width(), g.height()));

      forEachBrushPixel(&g, old_screen_x, old_screen_y, old_x, old_y, ui::ColorNone, clearpixel);
    }

    if (cursor_type & CURSOR_THINCROSS && m_state->requireBrushPreview()) {
      Brush* brush = editor_get_current_brush();
      gfx::Rect brushBounds = brush->getBounds();
      gfx::Rect rc1(old_x+brushBounds.x, old_y+brushBounds.y, brushBounds.w, brushBounds.h);
      gfx::Rect rc2(new_x+brushBounds.x, new_y+brushBounds.y, brushBounds.w, brushBounds.h);
      m_document->notifySpritePixelsModified
        (m_sprite, gfx::Region(rc1.createUnion(rc2)));
    }

    // Save area and draw the cursor
    ScreenGraphics g;
    forEachBrushPixel(&g, m_cursor_screen_x, m_cursor_screen_y, new_x, new_y, ui_cursor_color, savepixel);
    forEachBrushPixel(&g, m_cursor_screen_x, m_cursor_screen_y, new_x, new_y, ui_cursor_color, drawpixel);
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
  int x, y;

  ASSERT(m_cursor_thick != 0);
  ASSERT(m_sprite != NULL);

  getDrawableRegion(clipping_region, kCutTopWindows);

  x = m_cursor_editor_x;
  y = m_cursor_editor_y;

  if (refresh) {
    // Restore pixels
    ScreenGraphics g;
    SetClip clip(&g, gfx::Rect(0, 0, g.width(), g.height()));

    forEachBrushPixel(&g, m_cursor_screen_x, m_cursor_screen_y, x, y, ui::ColorNone, clearpixel);
  }

  // Clean pixel/brush preview
  if (cursor_type & CURSOR_THINCROSS && m_state->requireBrushPreview()) {
    Brush* brush = editor_get_current_brush();
    gfx::Rect brushBounds = brush->getBounds();

    m_document->prepareExtraCel(x+brushBounds.x, y+brushBounds.y,
                                brushBounds.w, brushBounds.h,
                                0); // Opacity = 0

    if (refresh) {
      m_document->notifySpritePixelsModified
        (m_sprite,
         gfx::Region(gfx::Rect(x+brushBounds.x,
                               y+brushBounds.y,
                               brushBounds.w, brushBounds.h)));
    }
  }

  m_cursor_thick = 0;

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

static void generate_cursor_boundaries()
{
  tools::Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  IBrushSettings* brush_settings = NULL;
  if (current_tool)
    brush_settings = UIContext::instance()
      ->getSettings()
      ->getToolSettings(current_tool)
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

    cursor_bound.seg = find_mask_boundary(brush->get_image(),
                                          &cursor_bound.nseg,
                                          IgnoreBounds, 0, 0, 0, 0);
    delete brush;
  }
}

void Editor::forEachBrushPixel(
  ui::Graphics* g,
  int screen_x, int screen_y,
  int sprite_x, int sprite_y,
  ui::Color color,
  Editor::PixelDelegate pixelDelegate)
{
  saved_pixel_n = 0;

  if (cursor_type & CURSOR_THINCROSS)
    trace_thincross_pixels(g, this, screen_x, screen_y, color, pixelDelegate);

  if (cursor_type & CURSOR_THICKCROSS)
    trace_thickcross_pixels(g, this, sprite_x, sprite_y, color, 1, pixelDelegate);

  if (cursor_type & CURSOR_BRUSHBOUNDS)
    trace_brush_bounds(g, this, sprite_x, sprite_y, color, pixelDelegate);

  if (IS_SUBPIXEL(this)) {
    pixelDelegate(g, screen_x, screen_y, color);
  }
}

//////////////////////////////////////////////////////////////////////
// New Thin Cross

static void trace_thincross_pixels(ui::Graphics* g, Editor* editor,
  int x, int y, ui::Color color, Editor::PixelDelegate pixelDelegate)
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
  int u, v, xout, yout;

  for (v=0; v<7; v++) {
    for (u=0; u<7; u++) {
      if (cursor_cross[v*7+u]) {
        xout = x-3+u;
        yout = y-3+v;
        pixelDelegate(g, xout, yout, color);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Old Thick Cross

static void trace_thickcross_pixels(ui::Graphics* g, Editor* editor,
  int x, int y, ui::Color color, int thickness, Editor::PixelDelegate pixelDelegate)
{
  static int cursor_cross[6*6] = {
    0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0,
    1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1,
    0, 0, 1, 1, 0, 0,
    0, 0, 1, 1, 0, 0,
  };
  int u, v, xout, yout;
  int zoom = editor->getZoom();

  for (v=0; v<6; v++) {
    for (u=0; u<6; u++) {
      if (cursor_cross[v*6+u]) {
        editor->editorToScreen(x, y, &xout, &yout);

        xout += ((u<3) ?
                 u-((thickness>>1)<<zoom)-3:
                 u-((thickness>>1)<<zoom)-3+(thickness<<zoom));

        yout += ((v<3)?
                 v-((thickness>>1)<<zoom)-3:
                 v-((thickness>>1)<<zoom)-3+(thickness<<zoom));

        pixelDelegate(g, xout, yout, color);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Current Brush Bounds

struct Data {
  ui::Graphics* g;
  ui::Color color;
  Editor::PixelDelegate pixelDelegate;
};

static void algo_line_proxy(int x, int y, void* _data)
{
  Data* data = (Data*)_data;
  data->pixelDelegate(data->g, x, y, data->color);
}

static void trace_brush_bounds(ui::Graphics* g, Editor* editor,
  int x, int y, ui::Color color, Editor::PixelDelegate pixelDelegate)
{
  Data data = { g, color, pixelDelegate };
  int c, x1, y1, x2, y2;
  BoundSeg* seg;

  for (c=0; c<cursor_bound.nseg; c++) {
    seg = cursor_bound.seg+c;

    x1 = seg->x1 - cursor_bound.brush_size/2;
    y1 = seg->y1 - cursor_bound.brush_size/2;
    x2 = seg->x2 - cursor_bound.brush_size/2;
    y2 = seg->y2 - cursor_bound.brush_size/2;

    editor->editorToScreen(x+x1, y+y1, &x1, &y1);
    editor->editorToScreen(x+x2, y+y2, &x2, &y2);

    if (seg->open) {            /* outside */
      if (x1 == x2) {
        x1--;
        x2--;
        y2--;
      }
      else {
        y1--;
        y2--;
        x2--;
      }
    }
    else {
      if (x1 == x2) {
        y2--;
      }
      else {
        x2--;
      }
    }

    raster::algo_line(x1, y1, x2, y2, (void*)&data, algo_line_proxy);
  }
}

//////////////////////////////////////////////////////////////////////
// Helpers

static void savepixel(ui::Graphics* g, int x, int y, ui::Color color)
{
  if (saved_pixel_n < MAX_SAVED && clipping_region.contains(gfx::Point(x, y)))
    saved_pixel[saved_pixel_n++] = g->getPixel(x, y);
}

static void drawpixel(ui::Graphics* graphics, int x, int y, ui::Color color)
{
  if (saved_pixel_n < MAX_SAVED && clipping_region.contains(gfx::Point(x, y))) {
    if (cursor_negative) {
      int c = saved_pixel[saved_pixel_n++];
      int r = getr(c);
      int g = getg(c);
      int b = getb(c);

      graphics->putPixel(color_utils::blackandwhite_neg(ui::rgba(r, g, b)), x, y);
    }
    else {
      graphics->putPixel(color, x, y);
    }
  }
}

static void clearpixel(ui::Graphics* g, int x, int y, ui::Color color)
{
  if (saved_pixel_n < MAX_SAVED) {
    if (clipping_region.contains(gfx::Point(x, y)))
      g->putPixel(saved_pixel[saved_pixel_n++], x, y);
    else if (!old_clipping_region.isEmpty() &&
             old_clipping_region.contains(gfx::Point(x, y)))
      saved_pixel_n++;
  }
}

static color_t get_brush_color(Sprite* sprite, Layer* layer)
{
  app::Color c = UIContext::instance()->getSettings()->getFgColor();
  ASSERT(sprite != NULL);

  // Avoid using invalid colors
  if (!c.isValid())
    return 0;

  if (layer != NULL)
    return color_utils::color_for_layer(c, layer);
  else
    return color_utils::color_for_image(c, sprite->getPixelFormat());
}

} // namespace app
