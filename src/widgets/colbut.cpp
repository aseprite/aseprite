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
#include "Vaca/PreferredSizeEvent.h"

#include "core/color.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "widgets/colbar.h"
#include "widgets/colbut.h"
#include "widgets/colsel.h"
#include "widgets/editor.h"

static bool tooltip_window_msg_proc(JWidget widget, JMessage msg);

static int colorbutton_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

ColorButton::ColorButton(color_t color, int imgtype)
  : ButtonBase("", colorbutton_type(), JI_BUTTON, JI_BUTTON)
{
  m_color = color;
  m_imgtype = imgtype;
  m_tooltip_window = NULL;

  jwidget_focusrest(this, true);
}

ColorButton::~ColorButton()
{
  if (m_tooltip_window != NULL)
    delete m_tooltip_window;	// widget, frame
}

int ColorButton::getImgType()
{
  return m_imgtype;
}

color_t ColorButton::getColor()
{
  return m_color;
}

void ColorButton::setColor(color_t color)
{
  m_color = color;
  dirty();
}

bool ColorButton::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

    case JM_DRAW:
      draw();
      return true;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_BUTTON_SELECT) {
	// If the popup window was not created or shown yet..
	if (this->m_tooltip_window == NULL ||
	    !this->m_tooltip_window->isVisible()) {
	  // Open it
	  openSelectorDialog();
	}
	else {
	  // If it is visible, close it
	  closeSelectorDialog();
	}
	return true;
      }
      break;

    case JM_MOTION:
      if (this->hasCapture()) {
	JWidget picked = jwidget_pick(ji_get_default_manager(),
				      msg->mouse.x,
				      msg->mouse.y);
	color_t color = this->m_color;

	if (picked && picked != this) {
	  // Pick a color from another color-button
	  if (ColorButton* pickedColBut = dynamic_cast<ColorButton*>(picked)) {
	    color = pickedColBut->getColor();
	  }
	  // Pick a color from the color-bar
	  else if (picked->type == colorbar_type()) {
	    color = ((ColorBar*)picked)->getColorByPosition(msg->mouse.x, msg->mouse.y);
	  }
	  // Pick a color from a editor
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
	if (!color_equals(color, this->m_color)) {
	  this->setColor(color);
	  jwidget_emit_signal(this, SIGNAL_COLORBUTTON_CHANGE);
	}
      }
      break;

    case JM_SETCURSOR:
      if (this->hasCapture()) {
	jmouse_set_cursor(JI_CURSOR_EYEDROPPER);
	return true;
      }
      break;

  }

  return ButtonBase::onProcessMessage(msg);
}

void ColorButton::onPreferredSize(PreferredSizeEvent& ev)
{
  struct jrect box;

  jwidget_get_texticon_info(this, &box, NULL, NULL, 0, 0, 0);

  box.x2 = box.x1+64;

  ev.setPreferredSize(jrect_w(&box) + this->border_width.l + this->border_width.r,
		      jrect_h(&box) + this->border_width.t + this->border_width.b);
}

void ColorButton::draw()
{
  struct jrect box, text, icon;
  char buf[256];

  jwidget_get_texticon_info(this, &box, &text, &icon, 0, 0, 0);

  jdraw_rectfill(this->rc, ji_color_face());

  color_t color;

  // When the button is pushed, show the negative
  if (this->isSelected()) {
    color = color_rgb(255-color_get_red(this->m_color),
		      255-color_get_green(this->m_color),
		      255-color_get_blue(this->m_color));
  }
  // When the button is not pressed, show the real color
  else
    color = this->m_color;

  draw_color_button
    (ji_screen,
     this->getBounds(),
     true, true, true, true,
     true, true, true, true,
     this->m_imgtype,
     color,
     this->hasMouseOver(), false);

  // Draw text
  color_to_formalstring(this->m_imgtype,
			this->m_color, buf, sizeof(buf), false);

  this->setTextQuiet(buf);
  jwidget_get_texticon_info(this, &box, &text, &icon, 0, 0, 0);
  
  int textcolor = blackandwhite_neg(color_get_red(color),
				    color_get_green(color),
				    color_get_blue(color));
  jdraw_text(ji_screen, this->getFont(), this->getText(), text.x1, text.y1,
	     textcolor, -1, false, jguiscale());
}

void ColorButton::openSelectorDialog()
{
  Frame* window;
  int x, y;

  if (m_tooltip_window == NULL) {
    window = colorselector_new();
    window->user_data[0] = this;
    jwidget_add_hook(window, -1, tooltip_window_msg_proc, NULL);

    m_tooltip_window = window;
  }
  else {
    window = m_tooltip_window;
  }

  colorselector_set_color(window, m_color);

  window->open_window();

  x = MID(0, this->rc->x1, JI_SCREEN_W-jrect_w(window->rc));
  if (this->rc->y2 <= JI_SCREEN_H-jrect_h(window->rc))
    y = MAX(0, this->rc->y2);
  else
    y = MAX(0, this->rc->y1-jrect_h(window->rc));

  window->position_window(x, y);

  jmanager_dispatch_messages(jwidget_get_manager(window));
  jwidget_relayout(window);

  /* setup the hot-region */
  {
    JRect rc = jrect_new(MIN(this->rc->x1, window->rc->x1)-8,
			 MIN(this->rc->y1, window->rc->y1)-8,
			 MAX(this->rc->x2, window->rc->x2)+8,
			 MAX(this->rc->y2, window->rc->y2)+8);
    JRegion rgn = jregion_new(rc, 1);
    jrect_free(rc);

    static_cast<PopupWindow*>(window)->setHotRegion(rgn);
  }
}

void ColorButton::closeSelectorDialog()
{
  if (m_tooltip_window != NULL)
    m_tooltip_window->closeWindow(NULL);
}

static bool tooltip_window_msg_proc(JWidget widget, JMessage msg)
{
  switch (msg->type) {

    case JM_SIGNAL:
      if (msg->signal.num == SIGNAL_COLORSELECTOR_COLOR_CHANGED) {
	ColorButton* colorbutton_widget = (ColorButton*)widget->user_data[0];
	color_t color = colorselector_get_color(widget);

	colorbutton_widget->setColor(color);
	jwidget_emit_signal(colorbutton_widget, SIGNAL_COLORBUTTON_CHANGE);
      }
      break;

  }
  
  return false;
}
