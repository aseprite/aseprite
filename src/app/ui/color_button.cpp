/* Aseprite
 * Copyright (C) 2001-2013  David Capello
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <allegro.h>

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_button.h"
#include "app/ui/color_selector.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "raster/sprite.h"
#include "ui/preferred_size_event.h"
#include "ui/ui.h"

namespace app {

using namespace app::skin;
using namespace ui;

static WidgetType colorbutton_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

ColorButton::ColorButton(const app::Color& color, PixelFormat pixelFormat)
  : ButtonBase("", colorbutton_type(), kButtonWidget, kButtonWidget)
  , m_color(color)
  , m_pixelFormat(pixelFormat)
  , m_window(NULL)
{
  this->setFocusStop(true);

  setFont(static_cast<SkinTheme*>(getTheme())->getMiniFont());
}

ColorButton::~ColorButton()
{
  delete m_window;       // widget, window
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

app::Color ColorButton::getColor() const
{
  return m_color;
}

void ColorButton::setColor(const app::Color& color)
{
  m_color = color;

  // Change the color in its related window
  if (m_window)
    m_window->setColor(m_color, ColorSelector::DoNotChangeType);

  // Emit signal
  Change(color);

  invalidate();
}

bool ColorButton::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kCloseMessage:
      if (m_window && m_window->isVisible())
        m_window->closeWindow(NULL);
      break;

    case kMouseEnterMessage:
      StatusBar::instance()->showColor(0, "", m_color, 255);
      break;

    case kMouseLeaveMessage:
      StatusBar::instance()->clearText();
      break;

    case kMouseMoveMessage:
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        Widget* picked = getManager()->pick(mousePos);
        app::Color color = m_color;

        if (picked && picked != this) {
          // Pick a color from another color-button
          if (ColorButton* pickedColBut = dynamic_cast<ColorButton*>(picked)) {
            color = pickedColBut->getColor();
          }
          // Pick a color from the color-bar
          else if (picked->type == palette_view_type()) {
            color = ((PaletteView*)picked)->getColorByPosition(mousePos.x, mousePos.y);
          }
          // Pick a color from a editor
          else if (picked->type == editor_type()) {
            Editor* editor = static_cast<Editor*>(picked);
            Sprite* sprite = editor->getSprite();
            int x, y, imgcolor;

            if (sprite) {
              x = mousePos.x;
              y = mousePos.y;
              editor->screenToEditor(x, y, &x, &y);
              imgcolor = sprite->getPixel(x, y, editor->getFrame());
              color = app::Color::fromImage(sprite->getPixelFormat(), imgcolor);
            }
          }
        }

        // Did the color change?
        if (color != m_color) {
          setColor(color);
        }
      }
      break;

    case kSetCursorMessage:
      if (hasCapture()) {
        jmouse_set_cursor(kEyedropperCursor);
        return true;
      }
      break;

  }

  return ButtonBase::onProcessMessage(msg);
}

void ColorButton::onPreferredSize(PreferredSizeEvent& ev)
{
  gfx::Rect box;
  getTextIconInfo(&box);
  box.w = 64*jguiscale();

  ev.setPreferredSize(box.w + border_width.l + border_width.r,
                      box.h + border_width.t + border_width.b);
}

void ColorButton::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.getGraphics();
  SkinTheme* theme = static_cast<SkinTheme*>(getTheme());
  gfx::Rect rc = getClientBounds();

  ui::Color bg = getBgColor();
  if (is_transparent(bg))
    bg = theme->getColor(ThemeColor::Face);
  g->fillRect(bg, rc);

  app::Color color;

  // When the button is pushed, show the negative
  if (isSelected()) {
    color = app::Color::fromRgb(255-m_color.getRed(),
                                255-m_color.getGreen(),
                                255-m_color.getBlue());
  }
  // When the button is not pressed, show the real color
  else
    color = m_color;

  draw_color_button(g, rc,
    color, hasMouseOver(), false);

  // Draw text
  std::string str = m_color.toHumanReadableString(m_pixelFormat,
    app::Color::ShortHumanReadableString);

  setTextQuiet(str.c_str());

  ui::Color textcolor = ui::rgba(255, 255, 255);
  if (color.isValid())
    textcolor = color_utils::blackandwhite_neg(ui::rgba(color.getRed(), color.getGreen(), color.getBlue()));

  gfx::Rect text;
  getTextIconInfo(NULL, &text);
  g->drawString(getText(), textcolor, ColorNone, false, text.getOrigin());
}

void ColorButton::onClick(Event& ev)
{
  ButtonBase::onClick(ev);

  // If the popup window was not created or shown yet..
  if (m_window == NULL || !m_window->isVisible()) {
    // Open it
    openSelectorDialog();
  }
  else if (!m_window->isMoveable()) {
    // If it is visible, close it
    closeSelectorDialog();
  }
}

void ColorButton::openSelectorDialog()
{
  int x, y;

  if (m_window == NULL) {
    m_window = new ColorSelector();
    m_window->user_data[0] = this;
    m_window->ColorChange.connect(&ColorButton::onWindowColorChange, this);
  }

  m_window->setColor(m_color, ColorSelector::ChangeType);
  m_window->openWindow();

  x = MID(0, getBounds().x, JI_SCREEN_W-m_window->getBounds().w);
  if (getBounds().y2() <= JI_SCREEN_H-m_window->getBounds().h)
    y = MAX(0, getBounds().y2());
  else
    y = MAX(0, getBounds().y-m_window->getBounds().h);

  m_window->positionWindow(x, y);

  m_window->getManager()->dispatchMessages();
  m_window->layout();

  // Setup the hot-region
  gfx::Rect rc = getBounds().createUnion(m_window->getBounds());
  rc.enlarge(8);
  gfx::Region rgn(rc);
  static_cast<PopupWindow*>(m_window)->setHotRegion(rgn);
}

void ColorButton::closeSelectorDialog()
{
  if (m_window != NULL)
    m_window->closeWindow(NULL);
}

void ColorButton::onWindowColorChange(const app::Color& color)
{
  setColor(color);
}

} // namespace app
