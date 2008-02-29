/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2008  David A. Capello
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

#include "core/app.h"
#include "core/cfg.h"
#include "dialogs/colsel.h"
#include "dialogs/minipal.h"
#include "modules/color.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "modules/palette.h"
#include "modules/sprites.h"
#include "raster/image.h"
#include "raster/sprite.h"
#include "widgets/colbar.h"
#include "widgets/colsel2.h"
#include "widgets/statebar.h"

#define COLORBAR_MAX_COLORS   256

typedef enum {
  HOTCOLOR_NONE = -4,
  HOTCOLOR_FGCOLOR = -3,
  HOTCOLOR_BGCOLOR = -2,
  HOTCOLOR_BGSPRITE = -1,
  HOTCOLOR_GRADIENT = 0,
} hotcolor_t;

typedef struct ColorBar
{
  JWidget widget;
  int ncolor;
  color_t color[COLORBAR_MAX_COLORS];
  color_t fgcolor;
  color_t bgcolor;
  hotcolor_t hot;
  hotcolor_t hot_editing;
  JWidget tooltip_window;
} ColorBar;

static ColorBar *colorbar_data(JWidget colorbar);

static bool colorbar_msg_proc(JWidget widget, JMessage msg);
static color_t colorbar_get_hot_color(JWidget widget);
static void colorbar_open_tooltip(JWidget widget, int x, int y1, int y2,
				  JRect bounds, color_t color, hotcolor_t hot);
static void colorbar_close_tooltip(JWidget widget);

static int tooltip_window_color_changed(JWidget widget, int user_data);

static void update_status_bar(color_t color, int msecs);
static void get_info(JWidget widget, int *beg, int *end);
static void draw_colorbox(BITMAP *bmp,
			  int x1, int y1, int x2, int y2,
			  int b0, int b1, int b2, int b3,
			  int imgtype, color_t color,
			  bool hot);

JWidget colorbar_new(int align)
{
  JWidget widget = jwidget_new(colorbar_type());
  ColorBar *colorbar = jnew0(ColorBar, 1);

  colorbar->widget = widget;
  colorbar->ncolor = 16;
  colorbar->fgcolor = color_mask();
  colorbar->bgcolor = color_mask();
  colorbar->hot = HOTCOLOR_NONE;
  colorbar->hot_editing = HOTCOLOR_NONE;
  colorbar->tooltip_window = NULL;

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

int colorbar_type(void)
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

static ColorBar *colorbar_data(JWidget widget)
{
  return jwidget_get_data(widget, colorbar_type());
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
      colorbar->fgcolor = get_config_color("ColorBar", "FG", color_index(15));
      colorbar->bgcolor = get_config_color("ColorBar", "BG", color_index(0));
      break;
    }

    case JM_DESTROY: {
      char buf[256];
      int c;

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

      h = (y2-y1+1-(4+16+4+16+16+4));

      /* draw gradient */
      for (c=beg; c<=end; c++) {
	v1 = y1 + h*(c-beg  )/(end-beg+1);
	v2 = y1 + h*(c-beg+1)/(end-beg+1) - 1;

	draw_colorbox(doublebuffer, x1, v1, x2, v2,
		      c == beg, c == beg,
		      c == end, c == end, imgtype, colorbar->color[c],
		      (c == colorbar->hot));

	/* a selected color */
#if 0
	if ((c == colorbar->select[0]) || (c == colorbar->select[1])) {
	  color_t new_color;
	  int color;

	  new_color = color_from_image(imgtype,
				       get_color_for_image(imgtype,
							   colorbar->color[c]));

	  r = color_get_red(imgtype, new_color);
	  g = color_get_green(imgtype, new_color);
	  b = color_get_blue(imgtype, new_color);

	  jfree(new_color);

	  color = blackandwhite_neg(r, g, b);

	  if (c == colorbar->select[0])
	    rectfill(ji_screen, x1, v1, x1+1, v2, color);

	  if (c == colorbar->select[1])
	    rectfill(ji_screen, x2-1, v1, x2, v2, color);
	}
#endif
      }

      /* draw tool foreground color */
      v1 = y2-4-16-4-16-16;
      v2 = y2-4-16-4-16;
      draw_colorbox(doublebuffer, x1, v1, x2, v2, 1, 1, 0, 0,
		    imgtype, colorbar->fgcolor,
		    (colorbar->hot == HOTCOLOR_FGCOLOR));

      /* draw tool background color */
      v1 = y2-4-16-4-16+1;
      v2 = y2-4-16-4;
      draw_colorbox(doublebuffer, x1, v1, x2, v2, 0, 0, 1, 1,
		    imgtype, colorbar->bgcolor,
		    (colorbar->hot == HOTCOLOR_BGCOLOR));

      /* draw sprite background color */
      v1 = y2-4-16;
      v2 = y2-4;
      {
	color_t c =
	  current_sprite != NULL ? color_from_image(imgtype,
						    current_sprite->bgcolor):
				   color_mask();
	draw_colorbox(doublebuffer, x1, v1, x2, v2, 1, 1, 1, 1,
		      imgtype, c,
		      (colorbar->hot == HOTCOLOR_BGSPRITE));
      }

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

      h = (y2-y1+1-(4+16+4+16+16+4));

      for (c=beg; c<=end; c++) {
	v1 = y1 + h*(c-beg  )/(end-beg+1);
	v2 = y1 + h*(c-beg+1)/(end-beg+1) - 1;

	if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	  if (colorbar->hot != c) {
	    colorbar->hot = c;
	    hot_v1 = v1;
	    hot_v2 = v2;
	    break;
	  }
	}
      }

      /* in tool foreground color */
      v1 = y2-4-16-4-16-16;
      v2 = y2-4-16-4-16;
      if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	colorbar->hot = HOTCOLOR_FGCOLOR;
	hot_v1 = v1;
	hot_v2 = v2;
      }

      /* in tool background color */
      v1 = y2-4-16-4-16+1;
      v2 = y2-4-16-4;
      if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	colorbar->hot = HOTCOLOR_BGCOLOR;
	hot_v1 = v1;
	hot_v2 = v2;
      }

      /* in sprite background color */
      v1 = y2-4-16;
      v2 = y2-4;
      if ((msg->mouse.y >= v1) && (msg->mouse.y <= v2)) {
	colorbar->hot = HOTCOLOR_BGSPRITE;
	hot_v1 = v1;
	hot_v2 = v2;
      }

      /* redraw 'hot' color */
      if (colorbar->hot != old_hot) {
	jwidget_dirty(widget);

	/* close the old tooltip window to edit the 'old_hot' color slot */
	colorbar_close_tooltip(widget);

	if (colorbar->hot != HOTCOLOR_NONE) {
	  color_t color = colorbar_get_hot_color(widget);
	  JRect bounds = jwidget_get_rect(widget);

	  update_status_bar(color, 0);

	  /* open the tooltip window to edit the hot color */
	  bounds->y1 = hot_v1;
	  bounds->y2 = hot_v2;

	  colorbar_open_tooltip(widget, bounds->x2+1, hot_v1, hot_v2,
				bounds, color, colorbar->hot);

	  jrect_free(bounds);
	}
      }

      /* if the widget has the capture, we should replace the
	 selected-color with the hot-color */
      if (jwidget_has_capture(widget) &&
	  colorbar->hot != HOTCOLOR_NONE) {
	color_t color = colorbar_get_hot_color(widget);

	if (msg->mouse.left) {
	  colorbar_set_fg_color(widget, color);
	}
	if (msg->mouse.right) {
	  colorbar_set_bg_color(widget, color);
	}
/* 	else if (msg->mouse.right) { */
/* 	  /\* TODO *\/ */
/* 	  /\* colorbar->select[num = 1] = c; *\/ */
/* 	  /\* jwidget_dirty(widget); *\/ */
/* 	} */

/* 	/\* show a minipal? *\/ */
/* 	if (msg->type == JM_BUTTONPRESSED && */
/* 	    msg->any.shifts & KB_CTRL_FLAG && num >= 0) { */
/* 	  ji_minipal_new(widget, x2, v1); */
/* 	} */

	return TRUE;
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
      if (jwidget_has_capture(widget))
	jwidget_release_mouse(widget);
      break;

    case JM_DOUBLECLICK:
      if (colorbar->hot != HOTCOLOR_NONE) {
	int hot = colorbar->hot;
	color_t color = colorbar_get_hot_color(widget);

	while (jmouse_b(0))
	  jmouse_poll();

	/* change this color with the color-select dialog */
	if (ji_color_select(app_get_current_image_type(), &color)) {
	  switch (hot) {
	    case HOTCOLOR_FGCOLOR:
	      colorbar->fgcolor = color;
	      break;
	    case HOTCOLOR_BGCOLOR:
	      colorbar->bgcolor = color;
	      break;
	    case HOTCOLOR_BGSPRITE:
	      /* TODO setup sprite background color */
	      break;
	    default:
	      assert(hot >= 0 && hot < colorbar->ncolor);
	      colorbar->color[hot] = color;
	      break;
	  }
	  jwidget_dirty(widget);
	}
	return TRUE;
      }
      break;

    case JM_SETCURSOR:
      if (colorbar->hot != HOTCOLOR_FGCOLOR &&
	  colorbar->hot != HOTCOLOR_NONE) {
	jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	return TRUE;
      }
      break;
  }

  return FALSE;
}

static color_t colorbar_get_hot_color(JWidget widget)
{
  ColorBar *colorbar = colorbar_data(widget);

  switch (colorbar->hot) {
    case HOTCOLOR_NONE:     return color_mask();
    case HOTCOLOR_FGCOLOR:  return colorbar->fgcolor;
    case HOTCOLOR_BGCOLOR:  return colorbar->bgcolor;
    case HOTCOLOR_BGSPRITE: {
      int imgtype = app_get_current_image_type();
      return
	current_sprite != NULL ? color_from_image(imgtype,
						  current_sprite->bgcolor):
				 color_mask();
    }
    default:
      assert(colorbar->hot >= 0 &&
	     colorbar->hot < colorbar->ncolor);
      return colorbar->color[colorbar->hot];
  }
}

static void colorbar_open_tooltip(JWidget widget, int x, int y1, int y2,
				  JRect bounds,
				  color_t color, hotcolor_t hot)
{
  ColorBar *colorbar = colorbar_data(widget);
  JWidget window;
  char buf[256];

  if (colorbar->tooltip_window == NULL) {
    window = colorselector_new();
    HOOK(window, SIGNAL_COLORSELECTOR_COLOR_CHANGED,
	 tooltip_window_color_changed, widget);

    colorbar->tooltip_window = window;
  }
  else
    window = colorbar->tooltip_window;

  switch (colorbar->hot) {
    case HOTCOLOR_NONE:
      assert(FALSE);
      break;
    case HOTCOLOR_FGCOLOR:
      ustrcpy(buf, _("Primary Tool Color"));
      break;
    case HOTCOLOR_BGCOLOR:
      ustrcpy(buf, _("Secondary Tool Color (to fill shapes)"));
      break;
    case HOTCOLOR_BGSPRITE:
      ustrcpy(buf, _("Sprite Background Color"));
      break;
    default:
      usprintf(buf, _("Gradient Entry %d"), colorbar->hot);
      break;
  }
  jwidget_set_text(window, buf);

  colorselector_set_color(window, color);
  colorbar->hot_editing = hot;

  jwindow_open(window);

  x = MID(0, x, JI_SCREEN_W-jrect_w(window->rc));
  jwindow_position(window,
		   x, MID(0, y1, JI_SCREEN_H-jrect_h(window->rc)));

  jmanager_dispatch_messages(jwidget_get_manager(window));
  jwidget_relayout(window);

  {
    JRect rc = jrect_new(window->rc->x1,
			 window->rc->y1-8,
			 window->rc->x2+16,
			 window->rc->y2+8);
    JRect rc2 = jrect_new(widget->rc->x1, y1, x, y2+1);
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

  if (colorbar->tooltip_window != NULL)
    jwindow_close(colorbar->tooltip_window, NULL);
}

static int tooltip_window_color_changed(JWidget widget, int user_data)
{
  JWidget colorbar_widget = (JWidget)user_data;
  ColorBar *colorbar = colorbar_data(colorbar_widget);
  color_t color = colorselector_get_color(widget);

  switch (colorbar->hot_editing) {
    case HOTCOLOR_NONE:
      assert(FALSE);
      break;
    case HOTCOLOR_FGCOLOR:
      colorbar->fgcolor = color;
      break;
    case HOTCOLOR_BGCOLOR:
      colorbar->bgcolor = color;
      break;
    case HOTCOLOR_BGSPRITE: {
/*       int imgtype = app_get_current_image_type(); */
/*       return */
/* 	current_sprite != NULL ? color_from_image(imgtype, */
/* 						  current_sprite->bgcolor): */
/* 				 color_mask(); */
      /* TODO */
      break;
    }
    default:
      assert(colorbar->hot_editing >= 0 &&
	     colorbar->hot_editing < colorbar->ncolor);
      colorbar->color[colorbar->hot_editing] = color;

      if (colorbar->hot_editing == 0 ||
	  colorbar->hot_editing == colorbar->ncolor-1) {
	int imgtype = app_get_current_image_type();
	color_t c1 = colorbar->color[0];
	color_t c2 = colorbar->color[colorbar->ncolor-1];
	int r1 = color_get_red(imgtype, c1);
	int g1 = color_get_green(imgtype, c1);
	int b1 = color_get_blue(imgtype, c1);
	int a1 = color_get_alpha(imgtype, c1);
	int r2 = color_get_red(imgtype, c2);
	int g2 = color_get_green(imgtype, c2);
	int b2 = color_get_blue(imgtype, c2);
	int a2 = color_get_alpha(imgtype, c2);
	int c, r, g, b, a;

	for (c=1; c<colorbar->ncolor-1; ++c) {
	  r = r1 + (r2-r1) * c / colorbar->ncolor;
	  g = g1 + (g2-g1) * c / colorbar->ncolor;
	  b = b1 + (b2-b1) * c / colorbar->ncolor;
	  a = a1 + (a2-a1) * c / colorbar->ncolor;
	  colorbar->color[c] = color_rgb(r, g, b, a);
	}
      }
  }

  jwidget_dirty(colorbar_widget);
  return 0;
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

static void draw_colorbox(BITMAP *bmp,
			  int x1, int y1, int x2, int y2,
			  int b0, int b1, int b2, int b3,
			  int imgtype, color_t color,
			  bool hot)
{
  int face = ji_color_face();
  int fore = ji_color_foreground();

  draw_color(bmp, x1, y1, x2, y2, imgtype, color);

  hline(bmp, x1, y1, x2, fore);
  if (b2 && b3)
    hline(bmp, x1, y2, x2, fore);
  vline(bmp, x1, y1, y2, fore);
  vline(bmp, x2, y1, y2, fore);

  if (!hot) {
    int r = color_get_red(imgtype, color);
    int g = color_get_green(imgtype, color);
    int b = color_get_blue(imgtype, color);
    int c = makecol(MIN(255, r+64),
		    MIN(255, g+64),
		    MIN(255, b+64));
    rect(bmp, x1+1, y1+1, x2-1, y2-((b2 && b3)?1:0), c);
  }
  else {
    rect(bmp, x1+1, y1+1, x2-1, y2-((b2 && b3)?1:0), fore);
    bevel_box(bmp,
	      x1+1, y1+1, x2-1, y2-((b2 && b3)?1:0),
	      ji_color_facelight(), ji_color_faceshadow(), 1);
  }

  if (b0) {
    hline(bmp, x1, y1, x1+1, face);
    putpixel(bmp, x1, y1+1, face);
    putpixel(bmp, x1+1, y1+1, fore);
  }

  if (b1) {
    hline(bmp, x2-1, y1, x2, face);
    putpixel(bmp, x2, y1+1, face);
    putpixel(bmp, x2-1, y1+1, fore);
  }

  if (b2) {
    putpixel(bmp, x1, y2-1, face);
    hline(bmp, x1, y2, x1+1, face);
    putpixel(bmp, x1+1, y2-1, fore);
  }

  if (b3) {
    putpixel(bmp, x2, y2-1, face);
    hline(bmp, x2-1, y2, x2, face);
    putpixel(bmp, x2-1, y2-1, fore);
  }
}
