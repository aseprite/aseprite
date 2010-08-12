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

#include "jinete/jinete.h"

#include "core/color.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "widgets/colbar.h"
#include "widgets/colbut.h"
#include "widgets/colsel.h"
#include "widgets/editor.h"

typedef struct ColorButton
{
  color_t color;
  int imgtype;
  Frame* tooltip_window;
} ColorButton;

static ColorButton* colorbutton_data(JWidget widget);
static bool colorbutton_msg_proc(JWidget widget, JMessage msg);
static void colorbutton_draw(JWidget widget);
static void colorbutton_open_tooltip(JWidget widget);
static void colorbutton_close_tooltip(JWidget widget);

static bool tooltip_window_msg_proc(JWidget widget, JMessage msg);

JWidget colorbutton_new(color_t color, int imgtype)
{
  JWidget widget = jbutton_new("");
  ColorButton* colorbutton = jnew(ColorButton, 1);

  colorbutton->color = color;
  colorbutton->imgtype = imgtype;
  colorbutton->tooltip_window = NULL;

  jwidget_add_hook(widget, colorbutton_type(),
		   colorbutton_msg_proc, colorbutton);
  jwidget_focusrest(widget, true);

  return widget;
}

int colorbutton_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

int colorbutton_get_imgtype(JWidget widget)
{
  ColorButton* colorbutton = colorbutton_data(widget);

  return colorbutton->imgtype;
}

color_t colorbutton_get_color(JWidget widget)
{
  ColorButton* colorbutton = colorbutton_data(widget);

  return colorbutton->color;
}

void colorbutton_set_color(JWidget widget, color_t color)
{
  ColorButton* colorbutton = colorbutton_data(widget);

  colorbutton->color = color;
  jwidget_dirty(widget);
}

static ColorButton* colorbutton_data(JWidget widget)
{
  return reinterpret_cast<ColorButton*>
    (jwidget_get_data(widget, colorbutton_type()));
}

static bool colorbutton_msg_proc(JWidget widget, JMessage msg)
{
  ColorButton* colorbutton = colorbutton_data(widget);

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
      return true;
    }

    case JM_DRAW:
      colorbutton_draw(widget);
      return true;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_BUTTON_SELECT) {
	// If the popup window was not created or shown yet..
	if (colorbutton->tooltip_window == NULL ||
	    !colorbutton->tooltip_window->isVisible()) {
	  // Open it
	  colorbutton_open_tooltip(widget);
	}
	else {
	  // If it is visible, close it
	  colorbutton_close_tooltip(widget);
	}
	return true;
      }
      break;

    case JM_MOTION:
      if (widget->hasCapture()) {
	JWidget picked = jwidget_pick(ji_get_default_manager(),
				      msg->mouse.x,
				      msg->mouse.y);
	color_t color = colorbutton->color;

	if (picked && picked != widget) {
	  /* pick a color from another color-button */
	  if (picked->type == colorbutton_type()) {
	    color = colorbutton_data(picked)->color;
	  }
	  /* pick a color from the color-bar */
	  else if (picked->type == colorbar_type()) {
	    color = ((ColorBar*)picked)->getColorByPosition(msg->mouse.x, msg->mouse.y);
	  }
	  /* pick a color from a editor */
	  else if (picked->type == editor_type()) {
	    Editor* editor = static_cast<Editor*>(picked);
	    Sprite* sprite = editor->getSprite();
	    int x, y, imgcolor;

	    if (sprite) {
	      x = msg->mouse.x;
	      y = msg->mouse.y;
	      editor->screen_to_editor(x, y, &x, &y);
	      imgcolor = sprite->getPixel(x, y);
	      color = color_from_image(sprite->getImgType(), imgcolor);
	    }
	  }
	}

	/* does the color change? */
	if (!color_equals(color, colorbutton->color)) {
	  colorbutton_set_color(widget, color);
	  jwidget_emit_signal(widget, SIGNAL_COLORBUTTON_CHANGE);
	}
      }
      break;

    case JM_SETCURSOR:
      if (widget->hasCapture()) {
	jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	return true;
      }
      break;

  }

  return false;
}

static void colorbutton_draw(JWidget widget)
{
  ColorButton* colorbutton = colorbutton_data(widget);
  struct jrect box, text, icon;
  char buf[256];

  jwidget_get_texticon_info(widget, &box, &text, &icon, 0, 0, 0);

  jdraw_rectfill(widget->rc, ji_color_face());

  color_t color;
  // When the button is pushed, show the negative
  if (widget->isSelected()) {
    color = color_rgb(255-color_get_red(colorbutton->color),
		      255-color_get_green(colorbutton->color),
		      255-color_get_blue(colorbutton->color));
  }
  // When the button is not pressed, show the real color
  else
    color = colorbutton->color;

  draw_color_button
    (ji_screen,
     widget->getBounds(),
     true, true, true, true,
     true, true, true, true,
     colorbutton->imgtype,
     color,
     widget->hasMouseOver(), false);

  // Draw text
  color_to_formalstring(colorbutton->imgtype,
			colorbutton->color, buf, sizeof(buf), false);

  widget->setTextQuiet(buf);
  jwidget_get_texticon_info(widget, &box, &text, &icon, 0, 0, 0);
  
  int textcolor = blackandwhite_neg(color_get_red(color),
				    color_get_green(color),
				    color_get_blue(color));
  jdraw_text(ji_screen, widget->getFont(), widget->getText(), text.x1, text.y1,
	     textcolor, -1, false, jguiscale());
}

static void colorbutton_open_tooltip(JWidget widget)
{
  ColorButton* colorbutton = colorbutton_data(widget);
  Frame* window;
  int x, y;

  if (colorbutton->tooltip_window == NULL) {
    window = colorselector_new();
    window->user_data[0] = widget;
    jwidget_add_hook(window, -1, tooltip_window_msg_proc, NULL);

    colorbutton->tooltip_window = window;
  }
  else {
    window = colorbutton->tooltip_window;
  }

  colorselector_set_color(window, colorbutton->color);

  window->open_window();

  x = MID(0, widget->rc->x1, JI_SCREEN_W-jrect_w(window->rc));
  if (widget->rc->y2 <= JI_SCREEN_H-jrect_h(window->rc))
    y = MAX(0, widget->rc->y2);
  else
    y = MAX(0, widget->rc->y1-jrect_h(window->rc));

  window->position_window(x, y);

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

    static_cast<PopupWindow*>(window)->setHotRegion(rgn);
  }
}

static void colorbutton_close_tooltip(JWidget widget)
{
  ColorButton* colorbutton = colorbutton_data(widget);

  if (colorbutton->tooltip_window != NULL)
    colorbutton->tooltip_window->closeWindow(NULL);
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
  
  return false;
}
