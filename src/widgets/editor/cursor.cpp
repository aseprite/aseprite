/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2010  David Capello
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

#include "config.h"

#include <allegro.h>

#include "jinete/jbase.h"
#include "jinete/jlist.h"
#include "jinete/jrect.h"
#include "jinete/jregion.h"
#include "jinete/jsystem.h"
#include "jinete/jwidget.h"

#include "app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "modules/editors.h"
#include "raster/image.h"
#include "raster/layer.h"
#include "raster/pen.h"
#include "raster/sprite.h"
#include "tools/tool.h"
#include "ui_context.h"
#include "util/boundary.h"
#include "widgets/editor.h"

/**********************************************************************/
/* drawing-cursor routines */
/**********************************************************************/

/**
 * Returns true if the cursor of the editor needs subpixel movement.
 */
#define IS_SUBPIXEL(editor)	((editor)->m_zoom >= 2)

/**
 * Maximum quantity of colors to save pixels overlapped by the cursor.
 */
#define MAX_SAVED   4096

static struct {
  int pen_type;
  int pen_size;
  int pen_angle;
  int nseg;
  BoundSeg *seg;
} cursor_bound = { 0, 0, 0, 0, NULL };

enum {
  CURSOR_PENCIL      = 1,	// New cursor style (with preview)
  CURSOR_CROSS_ONE   = 2,	// Old cursor style (deprecated)
  CURSOR_BOUNDS      = 4	// Old cursor boundaries (deprecated)
};

static int cursor_type = CURSOR_PENCIL;
static int cursor_negative;

static int saved_pixel[MAX_SAVED];
static int saved_pixel_n;

// These clipping regions are shared between all editors, so we cannot
// make assumptions about their old state
static JRegion clipping_region = NULL;
static JRegion old_clipping_region = NULL;

static void generate_cursor_boundaries();

static void editor_cursor_pencil(Editor *editor, int x, int y, int color, int thickness, void (*pixel)(BITMAP *bmp, int x, int y, int color));
static void editor_cursor_cross(Editor *editor, int x, int y, int color, int thickness, void (*pixel)(BITMAP *bmp, int x, int y, int color));
static void editor_cursor_bounds(Editor *editor, int x, int y, int color, void (*pixel)(BITMAP *bmp, int x, int y, int color));

static void savepixel(BITMAP *bmp, int x, int y, int color);
static void drawpixel(BITMAP *bmp, int x, int y, int color);
static void cleanpixel(BITMAP *bmp, int x, int y, int color);

static int point_inside_region(int x, int y, JRegion region);

static int get_pen_color(Sprite *sprite);

//////////////////////////////////////////////////////////////////////
// CURSOR COLOR
//////////////////////////////////////////////////////////////////////

static color_t cursor_color;
static int _cursor_color;
static bool _cursor_mask;

static void update_cursor_color()
{
  if (ji_screen)
    _cursor_color = get_color_for_allegro(bitmap_color_depth(ji_screen),
					  cursor_color);
  else
    _cursor_color = 0;

  _cursor_mask = (color_type(cursor_color) == COLOR_TYPE_MASK);
}

int Editor::get_raw_cursor_color()
{
  return _cursor_color;
}

bool Editor::is_cursor_mask()
{
  return _cursor_mask;
}

color_t Editor::get_cursor_color()
{
  return cursor_color;
}

void Editor::set_cursor_color(color_t color)
{
  cursor_color = color;
  update_cursor_color();
}

//////////////////////////////////////////////////////////////////////
// Slots for App signals
//////////////////////////////////////////////////////////////////////

static int pen_size_thick = 0;
static Pen* current_pen = NULL;

static void on_palette_change_update_cursor_color()
{
  update_cursor_color();
}

static void on_pen_size_before_change()
{
  ASSERT(current_editor != NULL);

  pen_size_thick = current_editor->editor_get_cursor_thick();
  if (pen_size_thick)
    current_editor->hide_drawing_cursor();
}

static void on_pen_size_after_change()
{
  ASSERT(current_editor != NULL);

  // Show drawing cursor
  if (current_editor->getSprite() && pen_size_thick > 0)
    current_editor->show_drawing_cursor();
}

static Pen* editor_get_current_pen()
{
  // Create the current pen from settings
  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  IPenSettings* pen_settings = UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool)
    ->getPen();

  ASSERT(pen_settings != NULL);

  if (!current_pen ||
      current_pen->get_type() != pen_settings->getType() ||
      current_pen->get_size() != pen_settings->getSize() ||
      current_pen->get_angle() != pen_settings->getAngle()) {
    delete current_pen;
    current_pen = new Pen(pen_settings->getType(),
			  pen_settings->getSize(),
			  pen_settings->getAngle());
  }

  return current_pen;
}

//////////////////////////////////////////////////////////////////////
// CURSOR
//////////////////////////////////////////////////////////////////////

void Editor::editor_cursor_init()
{
  /* cursor color */
  set_cursor_color(get_config_color("Tools", "CursorColor", color_mask()));

  App::instance()->PaletteChange.connect(&on_palette_change_update_cursor_color);
  App::instance()->PenSizeBeforeChange.connect(&on_pen_size_before_change);
  App::instance()->PenSizeAfterChange.connect(&on_pen_size_after_change);
}

void Editor::editor_cursor_exit()
{
  set_config_color("Tools", "CursorColor", cursor_color);

  if (cursor_bound.seg != NULL)
    jfree(cursor_bound.seg);

  delete current_pen;
  current_pen = NULL;
}

/**
 * Draws the pen cursor inside the specified editor.
 *
 * @warning You should clean the cursor before to use
 * this routine with other editor.
 *
 * @param widget The editor widget
 * @param x Absolute position in X axis of the mouse.
 * @param y Absolute position in Y axis of the mouse.
 *
 * @see editor_clean_cursor
 */
void Editor::editor_draw_cursor(int x, int y, bool refresh)
{
  ASSERT(m_cursor_thick == 0);
  ASSERT(m_sprite != NULL);

  /* get drawable region */
  clipping_region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);

  /* get cursor color */
  cursor_negative = is_cursor_mask();
  int color = get_raw_cursor_color();

  /* cursor in the screen (view) */
  m_cursor_screen_x = x;
  m_cursor_screen_y = y;

  /* get cursor position in the editor */
  screen_to_editor(x, y, &x, &y);

  // Get the current tool
  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  // Setup the cursor type depending the current tool
  if (current_tool->getInk(0)->isSelection()) {
    cursor_type = CURSOR_CROSS_ONE;
  }
  else if (// Use cursor bounds for inks that are effects (eraser, blur, etc.)
	   current_tool->getInk(0)->isEffect() ||
	   // or when the FG color is mask and we are not in the background layer
	   (color_type(UIContext::instance()->getSettings()->getFgColor()) == COLOR_TYPE_MASK &&
	    (m_sprite->getCurrentLayer() != NULL &&
	     !m_sprite->getCurrentLayer()->is_background()))) {
    cursor_type = CURSOR_BOUNDS;
  }
  else {
    cursor_type = CURSOR_PENCIL;
  }

  // For cursor type 'bounds' we have to generate cursor boundaries
  if (cursor_type & CURSOR_BOUNDS)
    generate_cursor_boundaries();

  // draw pixel/pen preview
  if (cursor_type & CURSOR_PENCIL &&
      m_state != EDITOR_STATE_DRAWING) {
    IToolSettings* tool_settings = UIContext::instance()
      ->getSettings()
      ->getToolSettings(current_tool);

    int pen_color = get_pen_color(m_sprite);
    ase_uint32 new_mask_color;
    Pen* pen = editor_get_current_pen();

    // Create the extra cel to show the pen preview
    m_sprite->prepareExtraCel(x-pen->get_size()/2,
			      y-pen->get_size()/2,
			      pen->get_size(), pen->get_size(),
			      tool_settings->getOpacity());

    // In 'indexed' images, if the current color is 0, we have to use
    // a different mask color (different from 0) to draw the extra layer
    if (m_sprite->getImgType() == IMAGE_INDEXED && pen_color == 0) {
      new_mask_color = 1;
    }
    else {
      new_mask_color = 0;
    }

    Image* extraImage = m_sprite->getExtraCelImage();
    if (extraImage->mask_color != new_mask_color)
      image_clear(extraImage, extraImage->mask_color = new_mask_color);
    image_putpen(extraImage, pen, pen->get_size()/2, pen->get_size()/2, pen_color, extraImage->mask_color);

    if (refresh) {
      editors_draw_sprite(m_sprite,
			  x-pen->get_size()/2,
			  y-pen->get_size()/2,
			  x+pen->get_size()/2,
			  y+pen->get_size()/2);
    }
  }

  /* save area and draw the cursor */
  if (refresh) {
    acquire_bitmap(ji_screen);
    ji_screen->clip = false;
    for_each_pixel_of_pen(m_cursor_screen_x, m_cursor_screen_y, x, y, color, savepixel);
    for_each_pixel_of_pen(m_cursor_screen_x, m_cursor_screen_y, x, y, color, drawpixel);
    ji_screen->clip = true;
    release_bitmap(ji_screen);
  }

  // cursor thickness
  m_cursor_thick = 1; // get_thickness_for_cursor();

  /* cursor in the editor (model) */
  m_cursor_editor_x = x;
  m_cursor_editor_y = y;

  /* save the clipping-region to know where to clean the pixels */
  if (old_clipping_region)
    jregion_free(old_clipping_region);
  old_clipping_region = clipping_region;
}

void Editor::editor_move_cursor(int x, int y, bool refresh)
{
  int old_screen_x = m_cursor_screen_x;
  int old_screen_y = m_cursor_screen_y;
  int old_x = m_cursor_editor_x;
  int old_y = m_cursor_editor_y;

  editor_clean_cursor(false);
  editor_draw_cursor(x, y, false);

  int new_x = m_cursor_editor_x;
  int new_y = m_cursor_editor_y;

  if (refresh) {
    /* restore points */
    acquire_bitmap(ji_screen);
    ji_screen->clip = FALSE;
    for_each_pixel_of_pen(old_screen_x, old_screen_y, old_x, old_y, 0, cleanpixel);
    ji_screen->clip = TRUE;
    release_bitmap(ji_screen);
    
    if (cursor_type & CURSOR_PENCIL &&
	m_state != EDITOR_STATE_DRAWING) {
      Pen* pen = editor_get_current_pen();
      editors_draw_sprite(m_sprite,
			  Vaca::min_value(new_x, old_x)-pen->get_size()/2,
			  Vaca::min_value(new_y, old_y)-pen->get_size()/2,
			  Vaca::max_value(new_x, old_x)+pen->get_size()/2,
			  Vaca::max_value(new_y, old_y)+pen->get_size()/2);
    }

    /* save area and draw the cursor */
    int color = get_raw_cursor_color();
    acquire_bitmap(ji_screen);
    ji_screen->clip = false;
    for_each_pixel_of_pen(m_cursor_screen_x, m_cursor_screen_y, new_x, new_y, color, savepixel);
    for_each_pixel_of_pen(m_cursor_screen_x, m_cursor_screen_y, new_x, new_y, color, drawpixel);
    ji_screen->clip = true;
    release_bitmap(ji_screen);
  }
}

/**
 * Cleans the pen cursor from the specified editor.
 *
 * The mouse position is got from the last
 * call to @c editor_draw_cursor. So you must
 * to use this routine only if you called
 * @c editor_draw_cursor before with the specified
 * editor @a widget.
 *
 * @param widget The editor widget
 *
 * @see editor_draw_cursor
 */
void Editor::editor_clean_cursor(bool refresh)
{
  int x, y;

  ASSERT(m_cursor_thick != 0);

  clipping_region = jwidget_get_drawable_region(this, JI_GDR_CUTTOPWINDOWS);

  x = m_cursor_editor_x;
  y = m_cursor_editor_y;

  if (refresh) {
    /* restore points */
    acquire_bitmap(ji_screen);
    ji_screen->clip = FALSE;
    for_each_pixel_of_pen(m_cursor_screen_x, m_cursor_screen_y, x, y, 0, cleanpixel);
    ji_screen->clip = TRUE;
    release_bitmap(ji_screen);
  }

  // clean pixel/pen preview
  if (cursor_type & CURSOR_PENCIL &&
      m_state != EDITOR_STATE_DRAWING) {
    Pen* pen = editor_get_current_pen();

    m_sprite->prepareExtraCel(x-pen->get_size()/2,
			      y-pen->get_size()/2,
			      pen->get_size(), pen->get_size(),
			      0); // Opacity = 0

    if (refresh) {
      editors_draw_sprite(m_sprite,
			  x-pen->get_size()/2,
			  y-pen->get_size()/2,
			  x+pen->get_size()/2,
			  y+pen->get_size()/2);
    }
  }

  m_cursor_thick = 0;

  jregion_free(clipping_region);
  if (old_clipping_region)
    jregion_free(old_clipping_region);

  clipping_region = NULL;
  old_clipping_region = NULL;
}

/**
 * Returns true if the cursor to draw in the editor has subpixel
 * movement (a little pixel of the screen that indicates where is the
 * mouse inside the pixel of the sprite).
 */
bool Editor::editor_cursor_is_subpixel()
{
  return IS_SUBPIXEL(this);
}

//////////////////////////////////////////////////////////////////////

static void generate_cursor_boundaries()
{
  Tool* current_tool = UIContext::instance()
    ->getSettings()
    ->getCurrentTool();

  IPenSettings* pen_settings = UIContext::instance()
    ->getSettings()
    ->getToolSettings(current_tool)
    ->getPen();

  if (cursor_bound.seg == NULL || 
      cursor_bound.pen_type != pen_settings->getType() ||
      cursor_bound.pen_size != pen_settings->getSize() ||
      cursor_bound.pen_angle != pen_settings->getAngle()) {
    cursor_bound.pen_type = pen_settings->getType();
    cursor_bound.pen_size = pen_settings->getSize();
    cursor_bound.pen_angle = pen_settings->getAngle();

    if (cursor_bound.seg != NULL)
      jfree(cursor_bound.seg);

    Pen* pen;

    UIContext* context = UIContext::instance();
    Tool* current_tool = context->getSettings()->getCurrentTool();
    if (current_tool) {
      IPenSettings* pen_settings = context->getSettings()
	->getToolSettings(current_tool)->getPen();
      ASSERT(pen_settings != NULL);
      pen = new Pen(pen_settings->getType(),
		    pen_settings->getSize(),
		    pen_settings->getAngle());
    }
    else
      pen = new Pen();

    cursor_bound.seg = find_mask_boundary(pen->get_image(),
    					  &cursor_bound.nseg,
    					  IgnoreBounds, 0, 0, 0, 0);
    delete pen;
  }
}

void Editor::for_each_pixel_of_pen(int screen_x, int screen_y,
				   int sprite_x, int sprite_y, int color,
				   void (*pixel)(BITMAP *bmp, int x, int y, int color))
{
  saved_pixel_n = 0;

  if (cursor_type & CURSOR_PENCIL) {
    editor_cursor_pencil(this, screen_x, screen_y, color, 1, pixel);
  }

  if (cursor_type & CURSOR_CROSS_ONE) {
    editor_cursor_cross(this, sprite_x, sprite_y, color, 1, pixel);
  }

  if (cursor_type & CURSOR_BOUNDS) {
    editor_cursor_bounds(this, sprite_x, sprite_y, color, pixel);
  }

  if (IS_SUBPIXEL(this)) {
    (*pixel)(ji_screen, screen_x, screen_y, color);
  }
}

//////////////////////////////////////////////////////////////////////
// New cross

static void editor_cursor_pencil(Editor *editor, int x, int y, int color, int thickness, void (*pixel)(BITMAP *bmp, int x, int y, int color))
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
	(*pixel)(ji_screen, xout, yout, color);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Old cross

static void editor_cursor_cross(Editor* editor, int x, int y, int color, int thickness,
				void (*pixel)(BITMAP *bmp, int x, int y, int color))
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
  int zoom = editor->editor_get_zoom();

  for (v=0; v<6; v++) {
    for (u=0; u<6; u++) {
      if (cursor_cross[v*6+u]) {
	editor->editor_to_screen(x, y, &xout, &yout);

	xout += ((u<3) ?
		 u-((thickness>>1)<<zoom)-3:
		 u-((thickness>>1)<<zoom)-3+(thickness<<zoom));

	yout += ((v<3)?
		 v-((thickness>>1)<<zoom)-3:
		 v-((thickness>>1)<<zoom)-3+(thickness<<zoom));

	(*pixel)(ji_screen, xout, yout, color);
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////
// Cursor Bounds

static void editor_cursor_bounds(Editor *editor, int x, int y, int color, void (*pixel) (BITMAP *bmp, int x, int y, int color))
{
  int c, x1, y1, x2, y2;
  BoundSeg *seg;

  for (c=0; c<cursor_bound.nseg; c++) {
    seg = cursor_bound.seg+c;

    x1 = seg->x1 - cursor_bound.pen_size/2;
    y1 = seg->y1 - cursor_bound.pen_size/2;
    x2 = seg->x2 - cursor_bound.pen_size/2;
    y2 = seg->y2 - cursor_bound.pen_size/2;

    editor->editor_to_screen(x+x1, y+y1, &x1, &y1);
    editor->editor_to_screen(x+x2, y+y2, &x2, &y2);

    if (seg->open) {		/* outside */
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

    do_line(ji_screen, x1, y1, x2, y2, color, pixel);
  }
}

//////////////////////////////////////////////////////////////////////
// Helpers

static void savepixel(BITMAP *bmp, int x, int y, int color)
{
  if (saved_pixel_n < MAX_SAVED && point_inside_region(x, y, clipping_region))
    saved_pixel[saved_pixel_n++] = getpixel(bmp, x, y);
}

static void drawpixel(BITMAP *bmp, int x, int y, int color)
{
  if (saved_pixel_n < MAX_SAVED && point_inside_region(x, y, clipping_region)) {
    if (cursor_negative) {
      int r, g, b, c = saved_pixel[saved_pixel_n++];

      r = getr(c);
      g = getg(c);
      b = getb(c);

      putpixel(bmp, x, y, blackandwhite_neg(r, g, b));
    }
    else {
      putpixel(bmp, x, y, color);
    }
  }
}

static void cleanpixel(BITMAP *bmp, int x, int y, int color)
{
  if (saved_pixel_n < MAX_SAVED) {
    if (point_inside_region(x, y, clipping_region))
      putpixel(bmp, x, y, saved_pixel[saved_pixel_n++]);
    else if (old_clipping_region &&
	     point_inside_region(x, y, old_clipping_region))
      saved_pixel_n++;
  }
}

static int point_inside_region(int x, int y, JRegion region)
{
  struct jrect box;
  return jregion_point_in(region, x, y, &box);
}

static int get_pen_color(Sprite *sprite)
{
  color_t c = UIContext::instance()->getSettings()->getFgColor();
  ASSERT(sprite != NULL);

  // Avoid using invalid colors
  if (!color_is_valid(c))
    return 0;

  if (sprite->getCurrentLayer() != NULL)
    return get_color_for_layer(sprite->getCurrentLayer(), c);
  else
    return get_color_for_image(sprite->getImgType(), c);
}
