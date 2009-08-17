/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2009  David Capello
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
#include "core/app.h"
#include "core/cfg.h"
#include "core/color.h"
#include "modules/editors.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palettes.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/palette.h"
#include "raster/sprite.h"
#include "raster/undo.h"
#include "widgets/colbar.h"
#include "widgets/colsel.h"
#include "widgets/paledit.h"
#include "widgets/statebar.h"

#define COLORBAR_MAX_COLORS	256

#define FGBGSIZE		(16*guiscale())

typedef enum {
  HOTCOLOR_NONE = -3,
  HOTCOLOR_FGCOLOR = -2,
  HOTCOLOR_BGCOLOR = -1,
  HOTCOLOR_GRADIENT = 0,
} hotcolor_t;

typedef struct ColorBar
{
  JWidget widget;
  JWidget tooltip_window;
  int ncolor;
  int refresh_timer_id;
  color_t color[COLORBAR_MAX_COLORS];
  color_t fgcolor;
  color_t bgcolor;
  hotcolor_t hot;
  hotcolor_t hot_editing;
  /* drag & drop colors */
  hotcolor_t hot_drag;
  hotcolor_t hot_drop;
} ColorBar;

static ColorBar *colorbar_data(JWidget colorbar);

static bool colorbar_msg_proc(JWidget widget, JMessage msg);
static color_t colorbar_get_hotcolor(JWidget widget, hotcolor_t hot);
static void colorbar_set_hotcolor(JWidget widget, hotcolor_t hot, color_t color);

static void colorbar_open_tooltip(JWidget widget, int x1, int x2, int y1, int y2,
				  color_t color, hotcolor_t hot);
static void colorbar_close_tooltip(JWidget widget);

static bool tooltip_window_msg_proc(JWidget widget, JMessage msg);

static void update_status_bar(color_t color, int msecs);
static void get_info(JWidget widget, int *beg, int *end);

JWidget colorbar_new(int align)
{
  JWidget widget = jwidget_new(colorbar_type());
  ColorBar *colorbar = jnew0(ColorBar, 1);

  colorbar->widget = widget;
  colorbar->tooltip_window = NULL;
  colorbar->ncolor = 16;
  colorbar->refresh_timer_id = jmanager_add_timer(widget, 250);
  colorbar->fgcolor = color_mask();
  colorbar->bgcolor = color_mask();
  colorbar->hot = HOTCOLOR_NONE;
  colorbar->hot_editing = HOTCOLOR_NONE;
  colorbar->hot_drag = HOTCOLOR_NONE;
  colorbar->hot_drop = HOTCOLOR_NONE;

  jwidget_add_hook(widget, colorbar_type(), colorbar_msg_proc, colorbar);
  jwidget_focusrest(widget, TRUE);
  jwidget_set_align(widget, align);

  widget->border_width.l = 2;
  widget->border_width.t = 2;
  widget->border_width.r = 2;
  widget->border_width.b = 2;

  /* return the box */
  return widget;
}

int colorbar_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

void colorbar_set_size(JWidget widget, int size)
{
  ColorBar *colorbar = colorbar_data(widget);

  colorbar->ncolor = MID(1, size, COLORBAR_MAX_COLORS);

  jwidget_dirty(widget);
}

color_t colorbar_get_fg_color(JWidget widget)
{
  ColorBar *colorbar = colorbar_data(widget);

  return colorbar->fgcolor;
}

color_t colorbar_get_bg_color(JWidget widget)
{
  ColorBar *colorbar = colorbar_data(widget);

  return colorbar->bgcolor;
}

void colorbar_set_fg_color(JWidget widget, color_t color)
{
  ColorBar *colorbar = colorbar_data(widget);

  colorbar->fgcolor = color;

  jwidget_dirty(widget);
  update_status_bar(colorbar->fgcolor, 100);
}

void colorbar_set_bg_color(JWidget widget, color_t color)
{
  ColorBar *colorbar = colorbar_data(widget);

  colorbar->bgcolor = color;

  jwidget_dirty(widget);
  update_status_bar(colorbar->bgcolor, 100);
}

void colorbar_set_color(JWidget widget, int index, color_t color)
{
  ColorBar *colorbar = colorbar_data(widget);

  assert(index >= 0 && index < COLORBAR_MAX_COLORS);

  colorbar->color[index] = color;

  jwidget_dirty(widget);
}

color_t colorbar_get_color_by_position(JWidget widget, int x, int y)
{
  ColorBar *colorbar = colorbar_data(widget);
  int x1, y1, x2, y2, v1, v2;
  int c, h, beg, end;

  get_info(widget, &beg, &end);

  x1 = widget->rc->x1;
  y1 = widget->rc->y1;
  x2 = widget->rc->x2-1;
  y2 = widget->rc->y2-1;

  ++x1, ++y1, --x2, --y2;

  h = (y2-y1+1-(4+FGBGSIZE*2+4));

  for (c=beg; c<=end; c++) {
    v1 = y1 + h*(c-beg  )/(end-beg+1);
    v2 = y1 + h*(c-beg+1)/(end-beg+1) - 1;

    if ((y >= v1) && (y <= v2))
      return colorbar->color[c];
  }

  /* in foreground color */
  v1 = y2-4-FGBGSIZE*2;
  v2 = y2-4-FGBGSIZE;
  if ((y >= v1) && (y <= v2)) {
    return colorbar->fgcolor;
  }

  /* in background color */
  v1 = y2-4-FGBGSIZE+1;
  v2 = y2-4;
  if ((y >= v1) && (y <= v2)) {
    return colorbar->bgcolor;
  }

  return color_mask();
}

static ColorBar *colorbar_data(JWidget widget)
{
  return reinterpret_cast<ColorBar*>
    (jwidget_get_data(widget, colorbar_type()));
}

static bool colorbar_msg_proc(JWidget widget, JMessage msg)
{
  ColorBar *colorbar = colorbar_data(widget);

  switch (msg->type) {

    case JM_OPEN: {
      int ncolor = get_config_int("ColorBar", "NColors", colorbar->ncolor);
      char buf[256];
      int c, beg, end;

      colorbar->ncolor = MID(1, ncolor, COLORBAR_MAX_COLORS);

      get_info(widget, &beg, &end);

      /* fill color-bar with saved colors in the configuration file */
      for (c=0; c<COLORBAR_MAX_COLORS; c++) {
	usprintf(buf, "Color%03d", c);
	colorbar->color[c] = get_config_color("ColorBar",
					       buf, color_index(c));
      }

      /* get selected colors */
      colorbar->fgcolor = get_config_color("ColorBar", "FG", color_rgb(0, 0, 0));
      colorbar->bgcolor = get_config_color("ColorBar", "BG", color_rgb(255, 255, 255));
      break;
    }

    case JM_DESTROY: {
      char buf[256];
      int c;

      jmanager_remove_timer(colorbar->refresh_timer_id);

      if (colorbar->tooltip_window != NULL)
	jwidget_free(colorbar->tooltip_window);

      set_config_int("ColorBar", "NColors", colorbar->ncolor);
      set_config_color("ColorBar", "FG", colorbar->fgcolor);
      set_config_color("ColorBar", "BG", colorbar->bgcolor);

      for (c=0; c<colorbar->ncolor; c++) {
	usprintf(buf, "Color%03d", c);
	set_config_color("ColorBar", buf, colorbar->color[c]);
      }

      jfree(colorbar);
      break;
    }

    case JM_REQSIZE:
      msg->reqsize.w = msg->reqsize.h = 24;
      return TRUE;

    case JM_DRAW: {
      BITMAP *doublebuffer = create_bitmap(jrect_w(&msg->draw.rect),
					   jrect_h(&msg->draw.rect));
      int imgtype = app_get_current_image_type();
      int x1, y1, x2, y2, v1, v2;
      int c, h, beg, end;

      get_info(widget, &beg, &end);

      x1 = widget->rc->x1 - msg->draw.rect.x1;
      y1 = widget->rc->y1 - msg->draw.rect.y1;
      x2 = x1 + jrect_w(widget->rc) - 1;
      y2 = y1 + jrect_h(widget->rc) - 1;

      rectfill(doublebuffer, x1, y1, x2, y2, ji_color_face());
      ++x1, ++y1, --x2, --y2;

      h = (y2-y1+1-(4+FGBGSIZE*2+4));

      /* draw gradient */
      for (c=beg; c<=end; c++) {
	v1 = y1 + h*(c-beg  )/(end-beg+1);
	v2 = y1 + h*(c-beg+1)/(end-beg+1) - 1;

	draw_color_button(doublebuffer, x1, v1, x2, v2,
			  c == beg, c == beg,
			  c == end, c == end, imgtype, colorbar->color[c],
			  (c == colorbar->hot ||
			   c == colorbar->hot_editing),
			  (colorbar->hot_drag == c &&
			   colorbar->hot_drag != colorbar->hot_drop));
      }

      /* draw foreground color */
      v1 = y2-4-FGBGSIZE*2;
      v2 = y2-4-FGBGSIZE;
      draw_color_button(doublebuffer, x1, v1, x2, v2, 1, 1, 0, 0,
			imgtype, colorbar->fgcolor,
			(colorbar->hot         == HOTCOLOR_FGCOLOR ||
			 colorbar->hot_editing == HOTCOLOR_FGCOLOR),
			(colorbar->hot_drag == HOTCOLOR_FGCOLOR &&
			 colorbar->hot_drag != colorbar->hot_drop));

      /* draw background color */
      v1 = y2-4-FGBGSIZE+1;
      v2 = y2-4;
      draw_color_button(doublebuffer, x1, v1, x2, v2, 0, 0, 1, 1,
			imgtype, colorbar->bgcolor,
			(colorbar->hot         == HOTCOLOR_BGCOLOR ||
			 colorbar->hot_editing == HOTCOLOR_BGCOLOR),
			(colorbar->hot_drag == HOTCOLOR_BGCOLOR &&
			 colorbar->hot_drag != colorbar->hot_drop));

      blit(doublebuffer, ji_screen, 0, 0,
	   msg->draw.rect.x1,
	   msg->draw.rect.y1,
	   doublebuffer->w,
	   doublebuffer->h);
      destroy_bitmap(doublebuffer);
      return TRUE;
    }

    case JM_BUTTONPRESSED:
      jwidget_capture_mouse(widget);

      colorbar->hot_drag = colorbar->hot;
      colorbar->hot_drop = colorbar->hot;

    case JM_MOUSEENTER:
    case JM_MOTION: {
      int x1, y1, x2, y2, v1, v2;
      int c, h, beg, end;
      int old_hot = colorbar->hot;
      int hot_v1 = 0;
      int hot_v2 = 0;

      colorbar->hot = HOTCOLOR_NONE;

      get_info(widget, &beg, &end);

      x1 = widget->rc->x1;
      y1 = widget->rc->y1;
      x2 = widget->rc->x2-1;
      y2 = widget->rc->y2-1;

      ++x1, ++y1, --x2, --y2;

      h = (y2-y1+1-(4+FGBGSIZE*2+4));

      for (c=beg; c<=end; c++) {
	v1 = y1 + h*(c-beg  )/(end-beg+1);
	v2 = y1 + h*(c-beg+1)/(end-beg+1) - 1;

	if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	  if (colorbar->hot != c) {
	    colorbar->hot = static_cast<hotcolor_t>(c);
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
	colorbar->hot = HOTCOLOR_FGCOLOR;
	hot_v1 = v1;
	hot_v2 = v2;
      }

      /* in background color */
      v1 = y2-4-FGBGSIZE+1;
      v2 = y2-4;
      if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	colorbar->hot = HOTCOLOR_BGCOLOR;
	hot_v1 = v1;
	hot_v2 = v2;
      }

      /* drop target */
      if (colorbar->hot_drag != HOTCOLOR_NONE)
	colorbar->hot_drop = colorbar->hot;

      /* redraw 'hot' color */
      if (colorbar->hot != old_hot) {
	jwidget_dirty(widget);

	/* close the old tooltip window to edit the 'old_hot' color slot */
	colorbar_close_tooltip(widget);

	/* open the new hot-color to be edited */
	if ((colorbar->hot != HOTCOLOR_NONE) &&
	    (colorbar->hot_drag == colorbar->hot_drop)) {
	  color_t color = colorbar_get_hotcolor(widget, colorbar->hot);

	  update_status_bar(color, 0);

	  /* open the tooltip window to edit the hot color */
	  colorbar_open_tooltip(widget, widget->rc->x1-1, widget->rc->x2+1,
				hot_v1, hot_v2, color, colorbar->hot);
	}
      }

      return TRUE;
    }

    case JM_MOUSELEAVE:
      if (colorbar->hot != HOTCOLOR_NONE) {
	colorbar->hot = HOTCOLOR_NONE;
	jwidget_dirty(widget);
      }
      break;

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture(widget)) {
	/* drag and drop a color */
	if (colorbar->hot_drag != colorbar->hot_drop) {
	  if (colorbar->hot_drop != HOTCOLOR_NONE) {
	    color_t color = colorbar_get_hotcolor(widget, colorbar->hot_drag);
	    colorbar_set_hotcolor(widget, colorbar->hot_drop, color);
	  }
	  jwidget_dirty(widget);
	}
	/* pick the color */
	else if (colorbar->hot != HOTCOLOR_NONE) {
	  color_t color = colorbar_get_hotcolor(widget, colorbar->hot);

	  if (msg->mouse.left) {
	    colorbar_set_fg_color(widget, color);
	  }
	  if (msg->mouse.right) {
	    colorbar_set_bg_color(widget, color);
	  }
	}

	colorbar->hot_drag = HOTCOLOR_NONE;
	colorbar->hot_drop = HOTCOLOR_NONE;

	jwidget_release_mouse(widget);
      }
      break;

    case JM_SETCURSOR:
      if (colorbar->hot_drag != HOTCOLOR_NONE &&
	  colorbar->hot_drag != colorbar->hot_drop) {
	jmouse_set_cursor(JI_CURSOR_MOVE);
	return TRUE;
      }
      else if (colorbar->hot != HOTCOLOR_NONE) {
	jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	return TRUE;
      }
      break;

    case JM_TIMER:
      /* time to refresh all the editors which have the current
	 sprite selected? */
      if (msg->timer.timer_id == colorbar->refresh_timer_id) {
	try {
	  const CurrentSpriteReader sprite;
	  if (sprite != NULL)
	    update_editors_with_sprite(sprite);
	}
	catch (locked_sprite_exception&) {
	  // do nothing
	}

	jmanager_stop_timer(colorbar->refresh_timer_id);
      }
      break;

  }

  return FALSE;
}

static color_t colorbar_get_hotcolor(JWidget widget, hotcolor_t hot)
{
  ColorBar *colorbar = colorbar_data(widget);

  switch (hot) {
    case HOTCOLOR_NONE:     return color_mask();
    case HOTCOLOR_FGCOLOR:  return colorbar->fgcolor;
    case HOTCOLOR_BGCOLOR:  return colorbar->bgcolor;
    default:
      assert(hot >= 0 && hot < colorbar->ncolor);
      return colorbar->color[hot];
  }
}

static void colorbar_set_hotcolor(JWidget widget, hotcolor_t hot, color_t color)
{
  ColorBar *colorbar = colorbar_data(widget);

  switch (hot) {
    case HOTCOLOR_NONE:
      assert(FALSE);
      break;
    case HOTCOLOR_FGCOLOR:
      colorbar->fgcolor = color;
      break;
    case HOTCOLOR_BGCOLOR:
      colorbar->bgcolor = color;
      break;
    default:
      assert(hot >= 0 && hot < colorbar->ncolor);
      colorbar->color[hot] = color;

      if (hot == 0 || hot == colorbar->ncolor-1) {
	int imgtype = app_get_current_image_type();
	color_t c1 = colorbar->color[0];
	color_t c2 = colorbar->color[colorbar->ncolor-1];
	int r1 = color_get_red(imgtype, c1);
	int g1 = color_get_green(imgtype, c1);
	int b1 = color_get_blue(imgtype, c1);
	int r2 = color_get_red(imgtype, c2);
	int g2 = color_get_green(imgtype, c2);
	int b2 = color_get_blue(imgtype, c2);
	int c, r, g, b;

	for (c=1; c<colorbar->ncolor-1; ++c) {
	  r = r1 + (r2-r1) * c / colorbar->ncolor;
	  g = g1 + (g2-g1) * c / colorbar->ncolor;
	  b = b1 + (b2-b1) * c / colorbar->ncolor;
	  colorbar->color[c] = color_rgb(r, g, b);
	}
      }
      break;
  }
}

static void colorbar_open_tooltip(JWidget widget, int x1, int x2, int y1, int y2,
				  color_t color, hotcolor_t hot)
{
  ColorBar *colorbar = colorbar_data(widget);
  JWidget window;
  char buf[1024];		/* TODO warning buffer overflow */
  int x, y;

  if (colorbar->tooltip_window == NULL) {
    window = colorselector_new(TRUE);
    window->user_data[0] = widget;
    jwidget_add_hook(window, -1, tooltip_window_msg_proc, NULL);

    colorbar->tooltip_window = window;
  }
  else {
    window = colorbar->tooltip_window;
  }

  switch (colorbar->hot) {
    case HOTCOLOR_NONE:
      assert(FALSE);
      break;
    case HOTCOLOR_FGCOLOR: {
      Command *cmd;

      ustrcpy(buf, _("Foreground Color"));

      cmd = command_get_by_name(CMD_SWITCH_COLORS);
      assert(cmd != NULL);

      JAccel accel = get_accel_to_execute_command(cmd, NULL);
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
      usprintf(buf, _("Gradient Entry %d"), colorbar->hot);
      break;
  }
  jwidget_set_text(window, buf);

  colorselector_set_color(window, color);
  colorbar->hot_editing = hot;

  jwindow_open(window);

  /* window position */
  if (x2+jrect_w(window->rc) <= JI_SCREEN_W)
    x = x2;
  else
    x = x1-jrect_w(window->rc);
  y = (y1+y2)/2-jrect_h(window->rc)/2;

  x = MID(0, x, JI_SCREEN_W-jrect_w(window->rc));
  y = MID(widget->rc->y1, y, widget->rc->y2-jrect_h(window->rc));
  
  jwindow_position(window, x, y);

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

    jtooltip_window_set_hotregion(window, rgn);
  }
}

static void colorbar_close_tooltip(JWidget widget)
{
  ColorBar *colorbar = colorbar_data(widget);

  if (colorbar->tooltip_window != NULL) {
    /* close the widget */
    jwindow_close(colorbar->tooltip_window, NULL);

    /* dispatch the JM_CLOSE event to 'tooltip_window_msg_proc' */
    jmanager_dispatch_messages(jwidget_get_manager(widget));
  }
}

static bool tooltip_window_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      try {
	// change the sprite palette
	const CurrentSpriteReader sprite;
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

	    sprite_set_palette(sprite_writer, pal, FALSE);
	    set_current_palette(pal, TRUE);
	  }

	  update_editors_with_sprite(sprite);
	}
	/* change the system palette */
	else
	  set_default_palette(get_current_palette());

	/* set the 'hot_editing' to NONE */
	{
	  JWidget colorbar_widget = (JWidget)widget->user_data[0];
	  ColorBar *colorbar = colorbar_data(colorbar_widget);

	  colorbar->hot_editing = HOTCOLOR_NONE;
	  jwidget_dirty(colorbar_widget);
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
	JWidget colorbar_widget = (JWidget)widget->user_data[0];
	ColorBar *colorbar = colorbar_data(colorbar_widget);
	color_t color = colorselector_get_color(widget);
	JWidget pal = colorselector_get_paledit(widget);

	if (paledit_get_range_type(pal) != PALETTE_EDITOR_RANGE_NONE) {
	  bool array[256];
	  int i, j;

	  paledit_get_selected_entries(pal, array);

	  for (i=j=0; i<256; ++i)
	    if (array[i])
	      ++j;

	  colorbar->ncolor = j;
	  assert(colorbar->ncolor >= 1);

	  colorbar->hot = MIN(colorbar->hot, static_cast<hotcolor_t>(colorbar->ncolor-1));
	  colorbar->hot_editing = MIN(colorbar->hot_editing, static_cast<hotcolor_t>(colorbar->ncolor-1));

	  for (i=j=0; i<256; ++i)
	    if (array[i])
	      colorbar->color[j++] = color_index(i);
	}
	else {
	  colorbar_set_hotcolor(colorbar_widget, colorbar->hot_editing, color);
	}

	/* ONLY FOR TRUE-COLOR GRAPHICS MODE: if the palette is
	   different from the current sprite's palette, then we have
	   to start the "refresh_timer" to refresh all the editors
	   with that sprite */
	try {
	  CurrentSpriteWriter sprite;
	  if (sprite != NULL && bitmap_color_depth(screen) != 8) {
	    Palette *pal = sprite_get_palette(sprite, sprite->frame);
	  
	    if (palette_count_diff(pal, get_current_palette(), NULL, NULL) > 0) {
	      pal = get_current_palette();
	      pal->frame = sprite->frame;	/* TODO warning, modifing
						   the current palette in
						   this point... */

	      sprite_set_palette(sprite, pal, FALSE);
	      set_current_palette(pal, TRUE);

	      jmanager_start_timer(colorbar->refresh_timer_id);
	    }
	  }
	}
	catch (ase_exception& e) {
	  Console console;
	  console.printf("Error updating sprite palette.\n\n");
	  e.show();
	}

	jwidget_dirty(colorbar_widget);
      }
      break;

  }
  
  return FALSE;
}

static void update_status_bar(color_t color, int msecs)
{
  statusbar_show_color(app_get_statusbar(),
		       msecs,
		       app_get_current_image_type(),
		       color);
}

static void get_info(JWidget widget, int *_beg, int *_end)
{
  ColorBar *colorbar = colorbar_data(widget);
  int beg, end;

  beg = 0;
  end = colorbar->ncolor-1;

  if (_beg) *_beg = beg;
  if (_end) *_end = end;
}
