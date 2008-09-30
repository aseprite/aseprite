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

#include <allegro.h>

#include "jinete/jinete.h"

#include "core/color.h"
#include "modules/gfx.h"
#include "raster/sprite.h"
#include "widgets/colbar.h"
#include "widgets/colbut.h"
#include "widgets/colsel.h"
#include "widgets/editor.h"

typedef struct ColorButton
{
  color_t color;
  int imgtype;
  JWidget tooltip_window;
} ColorButton;

static ColorButton *colorbutton_data(JWidget widget);
static bool colorbutton_msg_proc(JWidget widget, JMessage msg);
static void colorbutton_draw(JWidget widget);
static void colorbutton_open_tooltip(JWidget widget);
static void colorbutton_close_tooltip(JWidget widget);

static bool tooltip_window_msg_proc(JWidget widget, JMessage msg);

JWidget colorbutton_new(color_t color, int imgtype)
{
  JWidget widget = jbutton_new("");
  ColorButton *colorbutton = jnew(ColorButton, 1);

  colorbutton->color = color;
  colorbutton->imgtype = imgtype;
  colorbutton->tooltip_window = NULL;

  jwidget_add_hook(widget, colorbutton_type(),
		   colorbutton_msg_proc, colorbutton);
  jwidget_focusrest(widget, TRUE);

  return widget;
}

int colorbutton_type(void)
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

int colorbutton_get_imgtype(JWidget widget)
{
  ColorButton *colorbutton = colorbutton_data(widget);

  return colorbutton->imgtype;
}

color_t colorbutton_get_color(JWidget widget)
{
  ColorButton *colorbutton = colorbutton_data(widget);

  return colorbutton->color;
}

void colorbutton_set_color(JWidget widget, color_t color)
{
  ColorButton *colorbutton = colorbutton_data(widget);

  colorbutton->color = color;
  jwidget_dirty(widget);
}

static ColorButton *colorbutton_data(JWidget widget)
{
  return jwidget_get_data(widget, colorbutton_type());
}

static bool colorbutton_msg_proc(JWidget widget, JMessage msg)
{
  ColorButton *colorbutton = colorbutton_data(widget);

  switch (msg->type) {

    case JM_DESTROY:
      if (colorbutton->tooltip_window != NULL)
	jwidget_free(colorbutton->tooltip_window);

      jfree(colorbutton);
      break;

    case JM_REQSIZE: {
      struct jrect box;

      jwidget_get_texticon_info(widget, &box, NULL, NULL, 0, 0, 0);

      box.x2 = box.x1+64;

      msg->reqsize.w = jrect_w(&box) + widget->border_width.l + widget->border_width.r;
      msg->reqsize.h = jrect_h(&box) + widget->border_width.t + widget->border_width.b;
      return TRUE;
    }

    case JM_MOUSEENTER:
      colorbutton_open_tooltip(widget);
      break;

    case JM_DRAW:
      colorbutton_draw(widget);
      return TRUE;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_BUTTON_SELECT) {
	colorbutton_close_tooltip(widget);
	jwidget_hard_capture_mouse(widget);
	return TRUE;
      }
      break;

    case JM_BUTTONPRESSED:
      if (jwidget_has_capture(widget) &&
	  widget->flags & JI_HARDCAPTURE) {
	return TRUE;
      }
      break;

    case JM_MOTION:
      if (jwidget_has_capture(widget) &&
	  widget->flags & JI_HARDCAPTURE) {
	JWidget picked = jwidget_pick(ji_get_default_manager(),
				      msg->mouse.x,
				      msg->mouse.y);
	color_t color = colorbutton->color;

	if (picked) {
	  /* pick a color from another color-button */
	  if (picked->type == colorbutton_type()) {
	    color = colorbutton_data(picked)->color;
	  }
	  /* pick a color from the color-bar */
	  else if (picked->type == colorbar_type()) {
	    color = colorbar_get_color_by_position(picked,
						   msg->mouse.x,
						   msg->mouse.y);
	  }
	  /* pick a color from a editor */
	  else if (picked->type == editor_type()) {
	    Sprite *sprite = editor_get_sprite(picked);
	    int x, y, imgcolor;
	    color_t tmp;

	    if (sprite) {
	      x = msg->mouse.x;
	      y = msg->mouse.y;
	      screen_to_editor(picked, x, y, &x, &y);
	      imgcolor = sprite_getpixel(sprite, x, y);
	      tmp = color_from_image(sprite->imgtype, imgcolor);

	      if (color_type(tmp) != COLOR_TYPE_MASK)
		color = tmp;
	    }
	  }
	}

	/* does the color change? */
	if (!color_equals(color, colorbutton->color)) {
	  colorbutton_set_color(widget, color);
	  jwidget_emit_signal(widget, SIGNAL_COLORBUTTON_CHANGE);
	}

	return TRUE;
      }
      break;

    case JM_BUTTONRELEASED:
      if (jwidget_has_capture(widget) &&
	  widget->flags & JI_HARDCAPTURE) {
	jwidget_release_mouse(widget);
      }
      break;

    case JM_SETCURSOR:
      if (jwidget_has_capture(widget) &&
	  widget->flags & JI_HARDCAPTURE) {
	jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	return TRUE;
      }
      break;

  }

  return FALSE;
}

static void colorbutton_draw(JWidget widget)
{
  ColorButton *colorbutton = colorbutton_data(widget);
  struct jrect box, text, icon;
  char buf[256];

  jwidget_get_texticon_info(widget, &box, &text, &icon, 0, 0, 0);

  draw_color_button
    (ji_screen,
     widget->rc->x1, widget->rc->y1,
     widget->rc->x2-1, widget->rc->y2-1,
     1, 1, 1, 1,
     colorbutton->imgtype,
     colorbutton->color,
     jwidget_has_mouse(widget),
     FALSE);

  /* draw text */
  color_to_formalstring(colorbutton->imgtype,
			colorbutton->color, buf, sizeof(buf), FALSE);

  jwidget_set_text_soft(widget, buf);
  jwidget_get_texticon_info(widget, &box, &text, &icon, 0, 0, 0);

  rectfill(ji_screen, text.x1, text.y1, text.x2-1, text.y2-1, makecol(0, 0, 0));
  jdraw_text(widget->text_font, widget->text, text.x1, text.y1,
	     makecol(255, 255, 255),
	     makecol(0, 0, 0), FALSE);
}

static void colorbutton_open_tooltip(JWidget widget)
{
  ColorButton *colorbutton = colorbutton_data(widget);
  JWidget window;
  int x, y;

  if (colorbutton->tooltip_window == NULL) {
    window = colorselector_new(FALSE);
    window->user_data[0] = widget;
    jwidget_add_hook(window, -1, tooltip_window_msg_proc, NULL);
    jwidget_set_text(window, "Select color");

    colorbutton->tooltip_window = window;
  }
  else {
    window = colorbutton->tooltip_window;
  }

  colorselector_set_color(window, colorbutton->color);

  jwindow_open(window);

  x = MID(0, widget->rc->x1, JI_SCREEN_W-jrect_w(window->rc));
  if (widget->rc->y2 <= JI_SCREEN_H-jrect_h(window->rc))
    y = MAX(0, widget->rc->y2);
  else
    y = MAX(0, widget->rc->y1-jrect_h(window->rc));

  jwindow_position(window, x, y);

  jmanager_dispatch_messages(jwidget_get_manager(window));
  jwidget_relayout(window);

  /* setup the hot-region */
  {
    JRect rc = jrect_new(MIN(widget->rc->x1, window->rc->x1)-8,
			 MIN(widget->rc->y1, window->rc->y1)-8,
			 MAX(widget->rc->x2, window->rc->x2)+8,
			 MAX(widget->rc->y2, window->rc->y2)+8);
    JRegion rgn = jregion_new(rc, 1);
    jrect_free(rc);

    jtooltip_window_set_hotregion(window, rgn);
  }
}

static void colorbutton_close_tooltip(JWidget widget)
{
  ColorButton *colorbutton = colorbutton_data(widget);

  if (colorbutton->tooltip_window != NULL)
    jwindow_close(colorbutton->tooltip_window, NULL);
}

static bool tooltip_window_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == SIGNAL_COLORSELECTOR_COLOR_CHANGED) {
	JWidget colorbutton_widget = (JWidget)widget->user_data[0];
	color_t color = colorselector_get_color(widget);

	colorbutton_set_color(colorbutton_widget, color);
	jwidget_emit_signal(colorbutton_widget, SIGNAL_COLORBUTTON_CHANGE);
      }
      break;

  }
  
  return FALSE;
}
