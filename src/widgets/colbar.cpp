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

#include <assert.h>
#include <string.h>
#include <allegro.h>

#include "jinete/jinete.h"

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

#define COLORBAR_MAX_COLORS	256

#define FGBGSIZE		(16*jguiscale())

typedef enum {
  HOTCOLOR_NONE = -3,
  HOTCOLOR_FGCOLOR = -2,
  HOTCOLOR_BGCOLOR = -1,
  HOTCOLOR_GRADIENT = 0,
} hotcolor_t;

class ColorBar : public Widget
{
  Frame* m_tooltip_window;
  int m_ncolor;
  int m_refresh_timer_id;
  color_t m_color[COLORBAR_MAX_COLORS];
  color_t m_fgcolor;
  color_t m_bgcolor;
  hotcolor_t m_hot;
  hotcolor_t m_hot_editing;
  // Drag & drop colors
  hotcolor_t m_hot_drag;
  hotcolor_t m_hot_drop;

public:
  ColorBar(int align);

  void setBarSize(int size);

  color_t getFgColor() const { return m_fgcolor; }
  color_t getBgColor() const { return m_bgcolor; }
  void setFgColor(color_t color);
  void setBgColor(color_t color);

  void setColor(int index, color_t color);
  color_t getColorByPosition(int x, int y);

protected:
  virtual bool msg_proc(JMessage msg);

private:
  color_t getHotColor(hotcolor_t hot);
  void setHotColor(hotcolor_t hot, color_t color);
  void openTooltip(int x1, int x2, int y1, int y2, color_t color, hotcolor_t hot);
  void closeTooltip();
  void getRange(int& beg, int& end);
  void updateStatusBar(color_t color, int msecs);

  static bool tooltip_window_msg_proc(JWidget widget, JMessage msg);
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
  ((ColorBar*)widget)->setBarSize(size);
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
  m_tooltip_window = NULL;
  m_ncolor = 16;
  m_refresh_timer_id = jmanager_add_timer(this, 250);
  m_fgcolor = color_mask();
  m_bgcolor = color_mask();
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
}

void ColorBar::setBarSize(int size)
{
  m_ncolor = MID(1, size, COLORBAR_MAX_COLORS);
  dirty();
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
  assert(index >= 0 && index < COLORBAR_MAX_COLORS);
  m_color[index] = color;
  dirty();
}

color_t ColorBar::getColorByPosition(int x, int y)
{
  int x1, y1, x2, y2, v1, v2;
  int c, h, beg, end;

  getRange(beg, end);

  x1 = this->rc->x1;
  y1 = this->rc->y1;
  x2 = this->rc->x2-1;
  y2 = this->rc->y2-1;

  ++x1, ++y1, --x2, --y2;

  h = (y2-y1+1-(4+FGBGSIZE*2+4));

  for (c=beg; c<=end; c++) {
    v1 = y1 + h*(c-beg  )/(end-beg+1);
    v2 = y1 + h*(c-beg+1)/(end-beg+1) - 1;

    if ((y >= v1) && (y <= v2))
      return m_color[c];
  }

  /* in foreground color */
  v1 = y2-4-FGBGSIZE*2;
  v2 = y2-4-FGBGSIZE;
  if ((y >= v1) && (y <= v2)) {
    return m_fgcolor;
  }

  /* in background color */
  v1 = y2-4-FGBGSIZE+1;
  v2 = y2-4;
  if ((y >= v1) && (y <= v2)) {
    return m_bgcolor;
  }

  return color_mask();
}

bool ColorBar::msg_proc(JMessage msg)
{
  switch (msg->type) {

    case JM_OPEN: {
      int ncolor = get_config_int("ColorBar", "NColors", m_ncolor);
      char buf[256];
      int c, beg, end;

      m_ncolor = MID(1, ncolor, COLORBAR_MAX_COLORS);

      getRange(beg, end);

      /* fill color-bar with saved colors in the configuration file */
      for (c=0; c<COLORBAR_MAX_COLORS; c++) {
	usprintf(buf, "Color%03d", c);
	m_color[c] = get_config_color("ColorBar",
					       buf, color_index(c));
      }

      /* get selected colors */
      m_fgcolor = get_config_color("ColorBar", "FG", color_rgb(0, 0, 0));
      m_bgcolor = get_config_color("ColorBar", "BG", color_rgb(255, 255, 255));
      break;
    }

    case JM_DESTROY: {
      char buf[256];
      int c;

      jmanager_remove_timer(m_refresh_timer_id);

      if (m_tooltip_window != NULL)
	jwidget_free(m_tooltip_window);

      set_config_int("ColorBar", "NColors", m_ncolor);
      set_config_color("ColorBar", "FG", m_fgcolor);
      set_config_color("ColorBar", "BG", m_bgcolor);

      for (c=0; c<m_ncolor; c++) {
	usprintf(buf, "Color%03d", c);
	set_config_color("ColorBar", buf, m_color[c]);
      }
      break;
    }

    case JM_REQSIZE:
      msg->reqsize.w = msg->reqsize.h = 24 * jguiscale();
      return true;

    case JM_DRAW: {
      SkinneableTheme* theme = static_cast<SkinneableTheme*>(this->theme);
      BITMAP *doublebuffer = create_bitmap(jrect_w(&msg->draw.rect),
					   jrect_h(&msg->draw.rect));
      int imgtype = app_get_current_image_type();
      int x1, y1, x2, y2, v1, v2;
      int c, h, beg, end;
      int bg = theme->get_panel_face_color();

      getRange(beg, end);

      x1 = this->rc->x1 - msg->draw.rect.x1;
      y1 = this->rc->y1 - msg->draw.rect.y1;
      x2 = x1 + jrect_w(this->rc) - 1;
      y2 = y1 + jrect_h(this->rc) - 1;

      rectfill(doublebuffer, x1, y1, x2, y2, bg);
      ++x1, ++y1, --x2, --y2;

      h = (y2-y1+1-(4+FGBGSIZE*2+4));

      /* draw range */
      for (c=beg; c<=end; c++) {
	v1 = y1 + h*(c-beg  )/(end-beg+1);
	v2 = y1 + h*(c-beg+1)/(end-beg+1) - 1;

	draw_color_button(doublebuffer, x1, v1, x2, v2,
			  c == beg, c == beg,
			  c == end, c == end, imgtype, m_color[c],
			  (c == m_hot ||
			   c == m_hot_editing),
			  (m_hot_drag == c &&
			   m_hot_drag != m_hot_drop),
			  bg);
	
	if (color_equals(m_fgcolor, m_color[c])) {
	  int neg = blackandwhite_neg(color_get_red(imgtype, m_fgcolor),
				      color_get_green(imgtype, m_fgcolor),
				      color_get_blue(imgtype, m_fgcolor));

	  textout_ex(doublebuffer, this->getFont(), "FG",
		     x1+4, v1+2, neg, -1);
	}

	if (color_equals(m_bgcolor, m_color[c])) {
	  int neg = blackandwhite_neg(color_get_red(imgtype, m_bgcolor),
				      color_get_green(imgtype, m_bgcolor),
				      color_get_blue(imgtype, m_bgcolor));

	  textout_ex(doublebuffer, this->getFont(), "BG",
		     x2-3-text_length(this->getFont(), "BG"),
		     v2-jwidget_get_text_height(this), neg, -1);
	}
      }

      /* draw foreground color */
      v1 = y2-4-FGBGSIZE*2;
      v2 = y2-4-FGBGSIZE;
      draw_color_button(doublebuffer, x1, v1, x2, v2, 1, 1, 0, 0,
			imgtype, m_fgcolor,
			(m_hot         == HOTCOLOR_FGCOLOR ||
			 m_hot_editing == HOTCOLOR_FGCOLOR),
			(m_hot_drag == HOTCOLOR_FGCOLOR &&
			 m_hot_drag != m_hot_drop), bg);

      /* draw background color */
      v1 = y2-4-FGBGSIZE+1;
      v2 = y2-4;
      draw_color_button(doublebuffer, x1, v1, x2, v2, 0, 0, 1, 1,
			imgtype, m_bgcolor,
			(m_hot         == HOTCOLOR_BGCOLOR ||
			 m_hot_editing == HOTCOLOR_BGCOLOR),
			(m_hot_drag == HOTCOLOR_BGCOLOR &&
			 m_hot_drag != m_hot_drop),
			bg);

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
      int x1, y1, x2, y2, v1, v2;
      int c, h, beg, end;
      int old_hot = m_hot;
      int hot_v1 = 0;
      int hot_v2 = 0;

      m_hot = HOTCOLOR_NONE;

      getRange(beg, end);

      x1 = this->rc->x1;
      y1 = this->rc->y1;
      x2 = this->rc->x2-1;
      y2 = this->rc->y2-1;

      ++x1, ++y1, --x2, --y2;

      h = (y2-y1+1-(4+FGBGSIZE*2+4));

      for (c=beg; c<=end; c++) {
	v1 = y1 + h*(c-beg  )/(end-beg+1);
	v2 = y1 + h*(c-beg+1)/(end-beg+1) - 1;

	if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	  if (m_hot != c) {
	    m_hot = static_cast<hotcolor_t>(c);
	    hot_v1 = v1;
	    hot_v2 = v2;
	    break;
	  }
	}
      }

      /* in foreground color */
      v1 = y2-4-FGBGSIZE*2;
      v2 = y2-4-FGBGSIZE;
      if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	m_hot = HOTCOLOR_FGCOLOR;
	hot_v1 = v1;
	hot_v2 = v2;
      }

      /* in background color */
      v1 = y2-4-FGBGSIZE+1;
      v2 = y2-4;
      if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	m_hot = HOTCOLOR_BGCOLOR;
	hot_v1 = v1;
	hot_v2 = v2;
      }

      // Drop target
      if (m_hot_drag != HOTCOLOR_NONE)
	m_hot_drop = m_hot;

      // Redraw 'hot' color
      if (m_hot != old_hot) {
	dirty();

	// Close the old tooltip window to edit the 'old_hot' color slot
	closeTooltip();

	// Open the new hot-color to be edited
	if ((m_hot != HOTCOLOR_NONE) &&
	    (m_hot_drag == m_hot_drop)) {
	  color_t color = getHotColor(m_hot);

	  updateStatusBar(color, 0);

	  // Open the tooltip window to edit the hot color
	  openTooltip(this->rc->x1-1, this->rc->x2+1,
		      hot_v1, hot_v2, color, m_hot);
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
      else if (m_hot != HOTCOLOR_NONE) {
	jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	return true;
      }
      break;

    case JM_TIMER:
      /* time to refresh all the editors which have the current
	 sprite selected? */
      if (msg->timer.timer_id == m_refresh_timer_id) {
	try {
	  const CurrentSpriteReader sprite(UIContext::instance());
	  if (sprite != NULL)
	    update_editors_with_sprite(sprite);
	}
	catch (locked_sprite_exception&) {
	  // do nothing
	}

	jmanager_stop_timer(m_refresh_timer_id);
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
      assert(hot >= 0 && hot < m_ncolor);
      return m_color[hot];
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
      assert(hot >= 0 && hot < m_ncolor);
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
      break;
  }
}

void ColorBar::openTooltip(int x1, int x2, int y1, int y2,
			   color_t color, hotcolor_t hot)
{
  Frame* window;
  char buf[1024];		/* TODO warning buffer overflow */
  int x, y;

  if (m_tooltip_window == NULL) {
    window = colorselector_new(true);
    window->user_data[0] = this;
    jwidget_add_hook(window, -1, ColorBar::tooltip_window_msg_proc, NULL);

    m_tooltip_window = window;
  }
  else {
    window = m_tooltip_window;
  }

  switch (m_hot) {
    case HOTCOLOR_NONE:
      assert(false);
      break;
    case HOTCOLOR_FGCOLOR: {
      ustrcpy(buf, _("Foreground Color"));

      JAccel accel = get_accel_to_execute_command(CommandId::switch_colors, NULL);
      if (accel != NULL) {
	ustrcat(buf, _(" - "));
	jaccel_to_string(accel, buf+ustrsize(buf));
	ustrcat(buf, _(" key switches colors"));
      }
      break;
    }
    case HOTCOLOR_BGCOLOR:
      ustrcpy(buf, _("Background Color"));
      break;
    default:
      usprintf(buf, _("Gradient Entry %d"), m_hot);
      break;
  }
  window->setText(buf);

  colorselector_set_color(window, color);
  m_hot_editing = hot;

  window->open_window();

  /* window position */
  if (x2+jrect_w(window->rc) <= JI_SCREEN_W)
    x = x2;
  else
    x = x1-jrect_w(window->rc);
  y = (y1+y2)/2-jrect_h(window->rc)/2;

  x = MID(0, x, JI_SCREEN_W-jrect_w(window->rc));
  y = MID(this->rc->y1, y, this->rc->y2-jrect_h(window->rc));
  
  window->position_window(x, y);

  jmanager_dispatch_messages(jwidget_get_manager(window));
  jwidget_relayout(window);

  /* setup the hot-region */
  {
    JRect rc = jrect_new(window->rc->x1-8,
			 window->rc->y1-8,
			 window->rc->x2+8,
			 window->rc->y2+8);
/*     JRect rc2 = jrect_new(widget->rc->x1, y1, x, y2+1); */
    JRect rc2 = jrect_new(x1, y1, x2, y2+1);
    JRegion rgn = jregion_new(rc, 1);
    JRegion rgn2 = jregion_new(rc2, 1);

    jregion_union(rgn, rgn, rgn2);

    jregion_free(rgn2);
    jrect_free(rc2);
    jrect_free(rc);

    static_cast<TipWindow*>(window)->set_hotregion(rgn);
  }
}

void ColorBar::closeTooltip()
{
  if (m_tooltip_window != NULL) {
    /* close the widget */
    m_tooltip_window->closeWindow(NULL);

    /* dispatch the JM_CLOSE event to 'tooltip_window_msg_proc' */
    jmanager_dispatch_messages(jwidget_get_manager(this));
  }
}

bool ColorBar::tooltip_window_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      try {
	// change the sprite palette
	const CurrentSpriteReader sprite(UIContext::instance());
	if (sprite != NULL) {
	  Palette *pal = sprite_get_palette(sprite, sprite->frame);
	  int from, to;

	  if (palette_count_diff(pal, get_current_palette(), &from, &to) > 0) {
	    SpriteWriter sprite_writer(sprite);
	    /* TODO add undo support */
	    /* 	  if (undo_is_enabled(sprite->undo)) */
	    /* 	    undo_data(sprite->undo, (GfxObj *)sprite, pal, ); */

	    pal = get_current_palette();
	    pal->frame = sprite_writer->frame; /* TODO warning, modifing
						  the current palette in
						  this point... */

	    sprite_set_palette(sprite_writer, pal, false);
	    set_current_palette(pal, true);
	  }

	  update_editors_with_sprite(sprite);
	}
	/* change the system palette */
	else
	  set_default_palette(get_current_palette());

	/* set the 'hot_editing' to NONE */
	{
	  ColorBar* colorbar = (ColorBar*)widget->user_data[0];

	  colorbar->m_hot_editing = HOTCOLOR_NONE;
	  colorbar->dirty();
	}
      }
      catch (ase_exception& e) {
	Console console;
	console.printf("Error updating sprite palette.\n\n");
	e.show();
      }
      break;

    case JM_SIGNAL:
      if (msg->signal.num == SIGNAL_COLORSELECTOR_COLOR_CHANGED) {
	ColorBar* colorbar = (ColorBar*)widget->user_data[0];
	color_t color = colorselector_get_color(widget);
	JWidget pal = colorselector_get_paledit(widget);

	if (paledit_get_range_type(pal) != PALETTE_EDITOR_RANGE_NONE) {
	  bool array[256];
	  int i, j;

	  paledit_get_selected_entries(pal, array);

	  for (i=j=0; i<256; ++i)
	    if (array[i])
	      ++j;

	  colorbar->m_ncolor = j;
	  assert(colorbar->m_ncolor >= 1);

	  colorbar->m_hot = MIN(colorbar->m_hot, static_cast<hotcolor_t>(colorbar->m_ncolor-1));
	  colorbar->m_hot_editing = MIN(colorbar->m_hot_editing, static_cast<hotcolor_t>(colorbar->m_ncolor-1));

	  for (i=j=0; i<256; ++i)
	    if (array[i])
	      colorbar->m_color[j++] = color_index(i);
	}
	else {
	  colorbar->setHotColor(colorbar->m_hot_editing, color);
	}

	/* ONLY FOR TRUE-COLOR GRAPHICS MODE: if the palette is
	   different from the current sprite's palette, then we have
	   to start the "refresh_timer" to refresh all the editors
	   with that sprite */
	try {
	  CurrentSpriteWriter sprite(UIContext::instance());
	  if (sprite != NULL && bitmap_color_depth(screen) != 8) {
	    Palette *pal = sprite_get_palette(sprite, sprite->frame);
	  
	    if (palette_count_diff(pal, get_current_palette(), NULL, NULL) > 0) {
	      pal = get_current_palette();
	      pal->frame = sprite->frame;	/* TODO warning, modifing
						   the current palette in
						   this point... */

	      sprite_set_palette(sprite, pal, false);
	      set_current_palette(pal, true);

	      jmanager_start_timer(colorbar->m_refresh_timer_id);
	    }
	  }
	}
	catch (ase_exception& e) {
	  Console console;
	  console.printf("Error updating sprite palette.\n\n");
	  e.show();
	}

	colorbar->dirty();
      }
      break;

  }
  
  return false;
}

void ColorBar::getRange(int& beg, int& end)
{
  beg = 0;
  end = m_ncolor-1;
}

void ColorBar::updateStatusBar(color_t color, int msecs)
{
  statusbar_show_color(app_get_statusbar(),
		       msecs,
		       app_get_current_image_type(),
		       color);
}
