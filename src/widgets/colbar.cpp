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

#include <cassert>
#include <cstring>
#include <allegro.h>

#include "jinete/jinete.h"
#include "Vaca/Point.h"
#include "Vaca/Rect.h"

#include "commands/commands.h"
#include "app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/skinneable_theme.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "widgets/colbar.h"
#include "widgets/colsel.h"
#include "widgets/paledit.h"
#include "widgets/statebar.h"
#include "sprite_wrappers.h"
#include "ui_context.h"
#include "console.h"

using Vaca::Point;
using Vaca::Rect;

#define COLORBAR_MAX_COLORS	256

// Pixels
#define FGBUTTON_SIZE		(16*jguiscale())
#define BGBUTTON_SIZE	        (18*jguiscale())

typedef enum {
  HOTCOLOR_NONE = -3,
  HOTCOLOR_FGCOLOR = -2,
  HOTCOLOR_BGCOLOR = -1,
} hotcolor_t;

class ColorBar : public Widget
{
  size_t m_firstIndex;
  size_t m_columns;
  size_t m_colorsPerColum;
  int m_entrySize;
  color_t m_fgcolor;
  color_t m_bgcolor;
  hotcolor_t m_hot;
  hotcolor_t m_hot_editing;
  // Drag & drop colors
  hotcolor_t m_hot_drag;
  hotcolor_t m_hot_drop;

public:
  ColorBar(int align);
  ~ColorBar();

  color_t getFgColor() const { return m_fgcolor; }
  color_t getBgColor() const { return m_bgcolor; }
  void setFgColor(color_t color);
  void setBgColor(color_t color);

  void setColor(int index, color_t color);
  color_t getColorByPosition(int x, int y);

protected:
  virtual bool msg_proc(JMessage msg);

private:
  int getEntriesCount() const { return m_columns*m_colorsPerColum; }
  color_t getEntryColor(size_t i) const { return color_index(i+m_firstIndex); }

  color_t getHotColor(hotcolor_t hot);
  void setHotColor(hotcolor_t hot, color_t color);
  Rect getColumnBounds(size_t column) const;
  Rect getEntryBounds(size_t index) const;
  Rect getFgBounds() const;
  Rect getBgBounds() const;
  void updateStatusBar(color_t color, int msecs);
};

int colorbar_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

Widget* colorbar_new(int align)
{
  return new ColorBar(align);
}

void colorbar_set_size(JWidget widget, int size)
{
  // TODO remove this
}

color_t colorbar_get_fg_color(JWidget widget)
{
  return ((ColorBar*)widget)->getFgColor();
}

color_t colorbar_get_bg_color(JWidget widget)
{
  return ((ColorBar*)widget)->getBgColor();
}

void colorbar_set_fg_color(JWidget widget, color_t color)
{
  ((ColorBar*)widget)->setFgColor(color);
}

void colorbar_set_bg_color(JWidget widget, color_t color)
{
  ((ColorBar*)widget)->setBgColor(color);
}

void colorbar_set_color(JWidget widget, int index, color_t color)
{
  ((ColorBar*)widget)->setColor(index, color);
}

color_t colorbar_get_color_by_position(JWidget widget, int x, int y)
{
  return ((ColorBar*)widget)->getColorByPosition(x, y);
}

//////////////////////////////////////////////////////////////////////
// ColorBar class

ColorBar::ColorBar(int align)
  : Widget(colorbar_type())
{
  m_entrySize = 16;
  m_firstIndex = 0;
  m_columns = 2;
  m_colorsPerColum = 12;
  m_fgcolor = color_index(15);
  m_bgcolor = color_index(0);
  m_hot = HOTCOLOR_NONE;
  m_hot_editing = HOTCOLOR_NONE;
  m_hot_drag = HOTCOLOR_NONE;
  m_hot_drop = HOTCOLOR_NONE;

  jwidget_focusrest(this, true);
  this->setAlign(align);

  this->border_width.l = 2;
  this->border_width.t = 2;
  this->border_width.r = 2;
  this->border_width.b = 2;

  // Get selected colors
  m_fgcolor = get_config_color("ColorBar", "FG", m_fgcolor);
  m_bgcolor = get_config_color("ColorBar", "BG", m_bgcolor);

  // Get color-bar configuration
  m_columns = get_config_int("ColorBar", "Columns", m_columns);
  m_columns = MID(1, m_columns, 4);

  m_entrySize = get_config_int("ColorBar", "EntrySize", m_entrySize);
  m_entrySize = MID(12, m_entrySize, 256);
}

ColorBar::~ColorBar()
{
  set_config_color("ColorBar", "FG", m_fgcolor);
  set_config_color("ColorBar", "BG", m_bgcolor);
  set_config_int("ColorBar", "Columns", m_columns);
  set_config_int("ColorBar", "EntrySize", m_entrySize);
}

void ColorBar::setFgColor(color_t color)
{
  m_fgcolor = color;
  dirty();

  updateStatusBar(m_fgcolor, 100);
}

void ColorBar::setBgColor(color_t color)
{
  m_bgcolor = color;
  dirty();

  updateStatusBar(m_bgcolor, 100);
}

void ColorBar::setColor(int index, color_t color)
{
  // TODO remove me
}

color_t ColorBar::getColorByPosition(int x, int y)
{
  for (int i=0; i<getEntriesCount(); ++i) {
    if (getEntryBounds(i).contains(Point(x, y)))
      return getEntryColor(i);
  }

  // In foreground color
  if (getFgBounds().contains(Point(x, y)))
    return m_fgcolor;

  // In background color
  if (getBgBounds().contains(Point(x, y)))
    return m_bgcolor;

  return color_mask();
}

bool ColorBar::msg_proc(JMessage msg)
{
  switch (msg->type) {

    case JM_REQSIZE:
      if (get_config_bool("ColorBar", "CanGrow", false))
	msg->reqsize.w = 20*jguiscale() * m_columns;
      else
	msg->reqsize.w = 20*jguiscale() * MAX(1, m_columns);
      msg->reqsize.h = 20*jguiscale();
      return true;

    case JM_DRAW: {
      // Update the number of colors per column
      {
      	m_colorsPerColum = getColumnBounds(1).h / (m_entrySize*jguiscale());
      	m_colorsPerColum = MAX(1, m_colorsPerColum);
	
      	if (m_colorsPerColum*m_columns > 256) {
      	  m_colorsPerColum = 256 / m_columns;
      	  if (m_colorsPerColum*m_columns > 256)
      	    m_colorsPerColum--;
      	}

      	assert(m_colorsPerColum*m_columns <= 256);
      }

      SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);
      BITMAP *doublebuffer = create_bitmap(jrect_w(&msg->draw.rect),
					   jrect_h(&msg->draw.rect));
      int imgtype = app_get_current_image_type();
      // int bg = theme->get_panel_face_color();
      int bg = theme->get_tab_selected_face_color();

      clear_to_color(doublebuffer, bg);

      for (int i=0; i<getEntriesCount(); ++i) {
	Rect entryBounds = getEntryBounds(i);

	// The button is not even visible
	if (!entryBounds.intersects(Rect(msg->draw.rect.x1,
					 msg->draw.rect.y1,
					 jrect_w(&msg->draw.rect),
					 jrect_h(&msg->draw.rect))))
	  continue;

	entryBounds.offset(-msg->draw.rect.x1,
			   -msg->draw.rect.y1);

	int col = (i / m_colorsPerColum);
	int row = (i % m_colorsPerColum);
	color_t color = color_index(m_firstIndex + i);

	draw_color_button(doublebuffer, entryBounds,
			  row == 0 && col == 0,			 // nw
			  row == 0,				// n
			  row == 0 && col == m_columns-1,	// ne
			  col == m_columns-1,			// e
			  row == m_colorsPerColum-1 && col == m_columns-1, // se
			  row == m_colorsPerColum-1,		// s
			  row == m_colorsPerColum-1 && col == 0, // sw
			  col == 0,				 // w
			  imgtype,
			  color,
			  (i == m_hot ||
			   i == m_hot_editing),
			  (m_hot_drag == i &&
			   m_hot_drag != m_hot_drop));
	
	if (color_equals(m_bgcolor, color)) {
	  BITMAP* old_ji_screen = ji_screen; // TODO fix this ugly hack
	  ji_screen = doublebuffer;
	  theme->draw_bounds(entryBounds.x, entryBounds.y,
			     entryBounds.x+entryBounds.w-1,
			     entryBounds.y+entryBounds.h-1 - (row == m_colorsPerColum-1 ? jguiscale(): 0),
			     PART_COLORBAR_BORDER_BG_NW, -1);
	  ji_screen = old_ji_screen;
	}
	if (color_equals(m_fgcolor, color)) {
	  BITMAP* old_ji_screen = ji_screen; // TODO fix this ugly hack
	  ji_screen = doublebuffer;
	  theme->draw_bounds(entryBounds.x, entryBounds.y,
			     entryBounds.x+entryBounds.w-1,
			     entryBounds.y+entryBounds.h-1 - (row == m_colorsPerColum-1 ? jguiscale(): 0),
			     PART_COLORBAR_BORDER_FG_NW, -1);
	  ji_screen = old_ji_screen;
	}
      }

      // Draw foreground color
      Rect fgBounds = getFgBounds().offset(-msg->draw.rect.x1,
					   -msg->draw.rect.y1);
      draw_color_button(doublebuffer, fgBounds,
			true, true, true, true,
			false, false, false, true,
			imgtype, m_fgcolor,
			(m_hot         == HOTCOLOR_FGCOLOR ||
			 m_hot_editing == HOTCOLOR_FGCOLOR),
			(m_hot_drag == HOTCOLOR_FGCOLOR &&
			 m_hot_drag != m_hot_drop));

      // Draw background color
      Rect bgBounds = getBgBounds().offset(-msg->draw.rect.x1,
					   -msg->draw.rect.y1);
      draw_color_button(doublebuffer, bgBounds,
			false, false, false, true,
			true, true, true, true,
			imgtype, m_bgcolor,
			(m_hot         == HOTCOLOR_BGCOLOR ||
			 m_hot_editing == HOTCOLOR_BGCOLOR),
			(m_hot_drag == HOTCOLOR_BGCOLOR &&
			 m_hot_drag != m_hot_drop));

      blit(doublebuffer, ji_screen, 0, 0,
	   msg->draw.rect.x1,
	   msg->draw.rect.y1,
	   doublebuffer->w,
	   doublebuffer->h);
      destroy_bitmap(doublebuffer);
      return true;
    }

    case JM_BUTTONPRESSED:
      jwidget_capture_mouse(this);

      m_hot_drag = m_hot;
      m_hot_drop = m_hot;

    case JM_MOUSEENTER:
    case JM_MOTION: {
      int old_hot = m_hot;
      int hot_v1 = 0;
      int hot_v2 = 0;

      m_hot = HOTCOLOR_NONE;

      for (int i=0; i<getEntriesCount(); ++i) {
	Rect entryBounds = getEntryBounds(i);

	if (entryBounds.contains(Point(msg->mouse.x, msg->mouse.y))) {
	  if (m_hot != i) {
	    m_hot = static_cast<hotcolor_t>(i);
	    hot_v1 = entryBounds.y;
	    hot_v2 = entryBounds.y+entryBounds.h-1;
	    break;
	  }
	}
      }

      Rect fgBounds = getFgBounds();
      if (fgBounds.contains(Point(msg->mouse.x, msg->mouse.y))) {
	m_hot = HOTCOLOR_FGCOLOR;
	hot_v1 = fgBounds.y;
	hot_v2 = fgBounds.y+fgBounds.h-1;
      }

      Rect bgBounds = getBgBounds();
      if (bgBounds.contains(Point(msg->mouse.x, msg->mouse.y))) {
	m_hot = HOTCOLOR_BGCOLOR;
	hot_v1 = bgBounds.y;
	hot_v2 = bgBounds.y+bgBounds.h-1;
      }

      // Drop target
      if (m_hot_drag != HOTCOLOR_NONE)
	m_hot_drop = m_hot;

      // Redraw 'hot' color
      if (m_hot != old_hot) {
	dirty();

	// Open the new hot-color to be edited
	if ((m_hot != HOTCOLOR_NONE) &&
	    (m_hot_drag == m_hot_drop)) {
	  color_t color = getHotColor(m_hot);

	  updateStatusBar(color, 0);
	}
      }

      return true;
    }

    case JM_MOUSELEAVE:
      if (m_hot != HOTCOLOR_NONE) {
	m_hot = HOTCOLOR_NONE;
	dirty();

	statusbar_set_text(app_get_statusbar(), 0, "");
      }
      break;

    case JM_WHEEL:
      {
	int delta = jmouse_z(1) - jmouse_z(0);

	// Without Ctrl or Alt
	if (!(msg->any.shifts & (KB_ALT_FLAG |
				 KB_CTRL_FLAG))) {
	  if (msg->any.shifts & KB_SHIFT_FLAG)
	    delta *= m_colorsPerColum;

	  if (((int)m_firstIndex)+delta < 0)
	    m_firstIndex = 0;
	  else if (m_firstIndex+delta > 256-getEntriesCount())
	    m_firstIndex = 256-getEntriesCount();
	  else
	    m_firstIndex += delta;
	}

	// With Ctrl only
	if ((msg->any.shifts & (KB_ALT_FLAG |
				KB_CTRL_FLAG |
				KB_SHIFT_FLAG)) == KB_CTRL_FLAG) {
	  if (((int)m_entrySize)+delta < 12)
	    m_entrySize = 12;
	  else if (m_entrySize+delta > 256)
	    m_entrySize = 256;
	  else
	    m_entrySize += delta;
	}

	// With Alt only
	if ((msg->any.shifts & (KB_ALT_FLAG |
				KB_CTRL_FLAG |
				KB_SHIFT_FLAG)) == KB_ALT_FLAG) {
	  int old_columns = m_columns;

	  if (((int)m_columns)+delta < 1)
	    m_columns = 1;
	  else if (m_columns+delta > 4)
	    m_columns = 4;
	  else
	    m_columns += delta;

	  if (get_config_bool("ColorBar", "CanGrow", false) ||
	      (old_columns == 1 || m_columns == 1)) {
	    app_get_top_window()->remap_window();
	    app_get_top_window()->dirty();
	  }
	}

	// Redraw the whole widget
	dirty();

	// Update the status bar
	updateStatusBar(getColorByPosition(jmouse_x(0), jmouse_y(0)), 0);
      }
      break;

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture(this)) {
	/* drag and drop a color */
	if (m_hot_drag != m_hot_drop) {
	  if (m_hot_drop != HOTCOLOR_NONE) {
	    color_t color = getHotColor(m_hot_drag);
	    setHotColor(m_hot_drop, color);
	  }
	  dirty();
	}
	/* pick the color */
	else if (m_hot != HOTCOLOR_NONE) {
	  color_t color = getHotColor(m_hot);

	  if (msg->mouse.left) {
	    colorbar_set_fg_color(this, color);
	  }
	  if (msg->mouse.right) {
	    colorbar_set_bg_color(this, color);
	  }
	}

	m_hot_drag = HOTCOLOR_NONE;
	m_hot_drop = HOTCOLOR_NONE;

	jwidget_release_mouse(this);
      }
      break;

    case JM_SETCURSOR:
      if (m_hot_drag != HOTCOLOR_NONE &&
	  m_hot_drag != m_hot_drop) {
	jmouse_set_cursor(JI_CURSOR_MOVE);
	return true;
      }
      else if (m_hot >= 0) {
	jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	return true;
      }
      break;

  }

  return Widget::msg_proc(msg);
}

color_t ColorBar::getHotColor(hotcolor_t hot)
{
  switch (hot) {
    case HOTCOLOR_NONE:     return color_mask();
    case HOTCOLOR_FGCOLOR:  return m_fgcolor;
    case HOTCOLOR_BGCOLOR:  return m_bgcolor;
    default:
      assert(hot >= 0 && hot < getEntriesCount());
      return getEntryColor(hot);
  }
}

void ColorBar::setHotColor(hotcolor_t hot, color_t color)
{
  switch (hot) {
    case HOTCOLOR_NONE:
      assert(false);
      break;
    case HOTCOLOR_FGCOLOR:
      m_fgcolor = color;
      break;
    case HOTCOLOR_BGCOLOR:
      m_bgcolor = color;
      break;
    default:
      assert(hot >= 0 && hot < getEntriesCount());
#if 0
      m_color[hot] = color;

      if (hot == 0 || hot == m_ncolor-1) {
	int imgtype = app_get_current_image_type();
	color_t c1 = m_color[0];
	color_t c2 = m_color[m_ncolor-1];
	int r1 = color_get_red(imgtype, c1);
	int g1 = color_get_green(imgtype, c1);
	int b1 = color_get_blue(imgtype, c1);
	int r2 = color_get_red(imgtype, c2);
	int g2 = color_get_green(imgtype, c2);
	int b2 = color_get_blue(imgtype, c2);
	int c, r, g, b;

	for (c=1; c<m_ncolor-1; ++c) {
	  r = r1 + (r2-r1) * c / m_ncolor;
	  g = g1 + (g2-g1) * c / m_ncolor;
	  b = b1 + (b2-b1) * c / m_ncolor;
	  m_color[c] = color_rgb(r, g, b);
	}
      }
#endif
      break;
  }
}

Rect ColorBar::getColumnBounds(size_t column) const
{
  Rect rc = getBounds().shrink(jguiscale());
  Rect fgRc = getFgBounds();

  rc.w /= m_columns;
  rc.x += (rc.w * column);
  rc.h  = (fgRc.y - rc.y);

  return rc;
}

Rect ColorBar::getEntryBounds(size_t index) const
{
  size_t row = (index % m_colorsPerColum);
  size_t col = (index / m_colorsPerColum);
  Rect rc = getColumnBounds(col);

  rc.h -= 2*jguiscale();

  rc.y += row * rc.h / m_colorsPerColum;
  rc.h = ((row+1) * rc.h / m_colorsPerColum) - (row * rc.h / m_colorsPerColum);

  if (row == m_colorsPerColum-1)
    rc.h += 2*jguiscale();

  return rc;
}

Rect ColorBar::getFgBounds() const
{
  Rect rc = getBounds().shrink(jguiscale());

  return Rect(rc.x, rc.y+rc.h-BGBUTTON_SIZE-FGBUTTON_SIZE,
	      rc.w, FGBUTTON_SIZE);
}

Rect ColorBar::getBgBounds() const
{
  Rect rc = getBounds().shrink(jguiscale());

  return Rect(rc.x, rc.y+rc.h-BGBUTTON_SIZE,
	      rc.w, BGBUTTON_SIZE);
}

void ColorBar::updateStatusBar(color_t color, int msecs)
{
  statusbar_show_color(app_get_statusbar(),
		       msecs,
		       app_get_current_image_type(),
		       color);
}
