/* ASEPRITE
 * Copyright (C) 2001-2012  David Capello
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

#include "app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "gui/gui.h"
#include "gui/preferred_size_event.h"
#include "modules/gfx.h"
#include "modules/gui.h"
#include "raster/sprite.h"
#include "skin/skin_theme.h"
#include "widgets/color_bar.h"
#include "widgets/color_button.h"
#include "widgets/color_selector.h"
#include "widgets/editor/editor.h"
#include "widgets/statebar.h"

static int colorbutton_type()
{
  static int type = 0;
  if (!type)
    type = ji_register_widget_type();
  return type;
}

ColorButton::ColorButton(const Color& color, PixelFormat pixelFormat)
  : ButtonBase("", colorbutton_type(), JI_BUTTON, JI_BUTTON)
  , m_color(color)
  , m_pixelFormat(pixelFormat)
  , m_frame(NULL)
{
  jwidget_focusrest(this, true);

  setFont(static_cast<SkinTheme*>(getTheme())->getMiniFont());
}

ColorButton::~ColorButton()
{
  delete m_frame;       // widget, frame
}

PixelFormat ColorButton::getPixelFormat() const
{
  return m_pixelFormat;
}

void ColorButton::setPixelFormat(PixelFormat pixelFormat)
{
  m_pixelFormat = pixelFormat;
  invalidate();
}

Color ColorButton::getColor() const
{
  return m_color;
}

void ColorButton::setColor(const Color& color)
{
  m_color = color;

  // Change the color in its related frame
  if (m_frame)
    m_frame->setColor(m_color, ColorSelector::DoNotChangeType);

  // Emit signal
  Change(color);

  invalidate();
}

bool ColorButton::onProcessMessage(Message* msg)
{
  switch (msg->type) {

    case JM_CLOSE:
      if (m_frame && m_frame->isVisible())
        m_frame->closeWindow(NULL);
      break;

    case JM_MOUSEENTER:
      app_get_statusbar()->showColor(0, "", m_color, 255);
      break;

    case JM_MOUSELEAVE:
      app_get_statusbar()->clearText();
      break;

    case JM_SIGNAL:
      if (msg->signal.num == JI_SIGNAL_BUTTON_SELECT) {
        // If the popup window was not created or shown yet..
        if (m_frame == NULL || !m_frame->isVisible()) {
          // Open it
          openSelectorDialog();
        }
        else if (!m_frame->is_moveable()) {
          // If it is visible, close it
          closeSelectorDialog();
        }
        return true;
      }
      break;

    case JM_MOTION:
      if (hasCapture()) {
        Widget* picked = ji_get_default_manager()->pick(msg->mouse.x, msg->mouse.y);
        Color color = m_color;

        if (picked && picked != this) {
          // Pick a color from another color-button
          if (ColorButton* pickedColBut = dynamic_cast<ColorButton*>(picked)) {
            color = pickedColBut->getColor();
          }
          // Pick a color from the color-bar
          else if (picked->type == palette_view_type()) {
            color = ((PaletteView*)picked)->getColorByPosition(msg->mouse.x, msg->mouse.y);
          }
          // Pick a color from a editor
          else if (picked->type == editor_type()) {
            Editor* editor = static_cast<Editor*>(picked);
            Sprite* sprite = editor->getSprite();
            int x, y, imgcolor;

            if (sprite) {
              x = msg->mouse.x;
              y = msg->mouse.y;
              editor->screenToEditor(x, y, &x, &y);
              imgcolor = sprite->getPixel(x, y);
              color = Color::fromImage(sprite->getPixelFormat(), imgcolor);
            }
          }
        }

        // Did the color change?
        if (color != m_color) {
          setColor(color);
        }
      }
      break;

    case JM_SETCURSOR:
      if (hasCapture()) {
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

  ev.setPreferredSize(jrect_w(&box) + border_width.l + border_width.r,
                      jrect_h(&box) + border_width.t + border_width.b);
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
  if (isSelected()) {
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
     m_pixelFormat,
     color,
     this->hasMouseOver(), false);

  // Draw text
  std::string str = m_color.toFormalString(m_pixelFormat, false);

  setTextQuiet(str.c_str());
  jwidget_get_texticon_info(this, &box, &text, &icon, 0, 0, 0);

  int textcolor = makecol(255, 255, 255);
  if (color.isValid())
    textcolor = color_utils::blackandwhite_neg(color.getRed(), color.getGreen(), color.getBlue());

  jdraw_text(ji_screen, getFont(), getText(), text.x1, text.y1,
             textcolor, -1, false, jguiscale());
}

void ColorButton::openSelectorDialog()
{
  int x, y;

  if (m_frame == NULL) {
    m_frame = new ColorSelector();
    m_frame->user_data[0] = this;
    m_frame->ColorChange.connect(&ColorButton::onFrameColorChange, this);
  }

  m_frame->setColor(m_color, ColorSelector::ChangeType);
  m_frame->open_window();

  x = MID(0, this->rc->x1, JI_SCREEN_W-jrect_w(m_frame->rc));
  if (this->rc->y2 <= JI_SCREEN_H-jrect_h(m_frame->rc))
    y = MAX(0, this->rc->y2);
  else
    y = MAX(0, this->rc->y1-jrect_h(m_frame->rc));

  m_frame->position_window(x, y);

  jmanager_dispatch_messages(m_frame->getManager());
  m_frame->layout();

  /* setup the hot-region */
  {
    JRect rc = jrect_new(MIN(this->rc->x1, m_frame->rc->x1)-8,
                         MIN(this->rc->y1, m_frame->rc->y1)-8,
                         MAX(this->rc->x2, m_frame->rc->x2)+8,
                         MAX(this->rc->y2, m_frame->rc->y2)+8);
    JRegion rgn = jregion_new(rc, 1);
    jrect_free(rc);

    static_cast<PopupFrame*>(m_frame)->setHotRegion(rgn);
  }
}

void ColorButton::closeSelectorDialog()
{
  if (m_frame != NULL)
    m_frame->closeWindow(NULL);
}

void ColorButton::onFrameColorChange(const Color& color)
{
  setColor(color);
}
