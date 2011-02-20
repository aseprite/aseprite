/* ASE - Allegro Sprite Editor
 * Copyright (C) 2001-2011  David Capello
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

#include "app/color.h"
#include "app/color_utils.h"
#include "gui/gui.h"
#include "gui/preferred_size_event.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "widgets/color_bar.h"
#include "widgets/color_button.h"
#include "widgets/color_selector.h"
#include "widgets/editor.h"

static bool tooltip_window_msg_proc(JWidget widget, JMessage msg);

static int colorbutton_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

ColorButton::ColorButton(const Color& color, int imgtype)
  : ButtonBase("", colorbutton_type(), JI_BUTTON, JI_BUTTON)
  , m_color(color)
  , m_imgtype(imgtype)
  , m_tooltip_window(NULL)
{
  jwidget_focusrest(this, true);
}

ColorButton::~ColorButton()
{
  if (m_tooltip_window != NULL)
    delete m_tooltip_window;	// widget, frame
}

int ColorButton::getImgType() const
{
  return m_imgtype;
}

Color ColorButton::getColor() const
{
  return m_color;
}

void ColorButton::setColor(const Color& color)
{
  m_color = color;
  invalidate();
}

bool ColorButton::onProcessMessage(JMessage msg)
{
  switch (msg->type) {

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
	Widget* picked = ji_get_default_manager()->pick(msg->mouse.x, msg->mouse.y);
	Color color = this->m_color;

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
	      color = Color::fromImage(sprite->getImgType(), imgcolor);
	    }
	  }
	}

	// Did the color change?
	if (color != this->m_color) {
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

void ColorButton::onPaint(PaintEvent& ev) // TODO use "ev.getGraphics()"
{
  struct jrect box, text, icon;
  jwidget_get_texticon_info(this, &box, &text, &icon, 0, 0, 0);

  int bg = getBgColor();
  if (bg < 0) bg = ji_color_face();
  jdraw_rectfill(this->rc, bg);

  Color color;

  // When the button is pushed, show the negative
  if (this->isSelected()) {
    color = Color::fromRgb(255-m_color.getRed(),
			   255-m_color.getGreen(),
			   255-m_color.getBlue());
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
  std::string str = m_color.toFormalString(this->m_imgtype, false);

  setTextQuiet(str.c_str());
  jwidget_get_texticon_info(this, &box, &text, &icon, 0, 0, 0);
  
  int textcolor = color_utils::blackandwhite_neg(color.getRed(),
						 color.getGreen(),
						 color.getBlue());
  jdraw_text(ji_screen, getFont(), getText(), text.x1, text.y1,
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

  jmanager_dispatch_messages(window->getManager());
  jwidget_relayout(window);

  /* setup the hot-region */
  {
    JRect rc = jrect_new(MIN(this->rc->x1, window->rc->x1)-8,
			 MIN(this->rc->y1, window->rc->y1)-8,
			 MAX(this->rc->x2, window->rc->x2)+8,
			 MAX(this->rc->y2, window->rc->y2)+8);
    JRegion rgn = jregion_new(rc, 1);
    jrect_free(rc);

    static_cast<PopupFrame*>(window)->setHotRegion(rgn);
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
	Color color = colorselector_get_color(widget);

	colorbutton_widget->setColor(color);
	jwidget_emit_signal(colorbutton_widget, SIGNAL_COLORBUTTON_CHANGE);
      }
      break;

  }
  
  return false;
}
