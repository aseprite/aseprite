// Aseprite
// Copyright (C) 2001-2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/app.h"
#include "app/color.h"
#include "app/color_picker.h"
#include "app/color_utils.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_button.h"
#include "app/ui/color_selector.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "doc/layer.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "ui/size_hint_event.h"
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
  , m_dependOnLayer(false)
{
  this->setFocusStop(true);

  setup_mini_font(this);

  UIContext::instance()->addObserver(this);
}

ColorButton::~ColorButton()
{
  UIContext::instance()->removeObserver(this);

  delete m_window;       // widget, window
}

PixelFormat ColorButton::pixelFormat() const
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
      StatusBar::instance()->showColor(0, "", m_color);
      break;

    case kMouseLeaveMessage:
      StatusBar::instance()->clearText();
      break;

    case kMouseMoveMessage:
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        Widget* picked = manager()->pick(mousePos);
        app::Color color = m_color;

        if (picked && picked != this) {
          // Pick a color from another color-button
          if (ColorButton* pickedColBut = dynamic_cast<ColorButton*>(picked)) {
            color = pickedColBut->getColor();
          }
          // Pick a color from the color-bar
          else if (picked->type() == palette_view_type()) {
            color = ((PaletteView*)picked)->getColorByPosition(mousePos);
          }
          // Pick a color from a editor
          else if (picked->type() == editor_type()) {
            Editor* editor = static_cast<Editor*>(picked);
            Site site = editor->getSite();
            if (site.sprite()) {
              gfx::Point editorPos = editor->screenToEditor(mousePos);

              ColorPicker picker;
              picker.pickColor(site, editorPos, ColorPicker::FromComposition);
              color = picker.color();
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
        ui::set_mouse_cursor(kEyedropperCursor);
        return true;
      }
      break;

  }

  return ButtonBase::onProcessMessage(msg);
}

void ColorButton::onSizeHint(SizeHintEvent& ev)
{
  gfx::Rect box;
  getTextIconInfo(&box);
  box.w = 64*guiscale();

  ev.setSizeHint(box.w + border().width(),
                 box.h + border().height());
}

void ColorButton::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  gfx::Rect rc = clientBounds();

  gfx::Color bg = bgColor();
  if (gfx::is_transparent(bg))
    bg = theme->colors.face();
  g->fillRect(bg, rc);

  app::Color color;

  // When the button is pushed, show the negative
  m_dependOnLayer = false;
  if (isSelected()) {
    color = app::Color::fromRgb(255-m_color.getRed(),
                                255-m_color.getGreen(),
                                255-m_color.getBlue());
  }
  // When the button is not pressed, show the real color
  else {
    color = m_color;

    // Show transparent color in indexed sprites as mask color when we
    // are in a transparent layer.
    if (color.getType() == app::Color::IndexType &&
        current_editor &&
        current_editor->sprite() &&
        current_editor->sprite()->pixelFormat() == IMAGE_INDEXED) {
      m_dependOnLayer = true;

      if (current_editor->sprite()->transparentColor() == color.getIndex() &&
          current_editor->layer() &&
          !current_editor->layer()->isBackground()) {
        color = app::Color::fromMask();
      }
    }
  }

  draw_color_button(g, rc,
                    color,
                    (doc::ColorMode)m_pixelFormat,
                    hasMouseOver(), false);

  // Draw text
  std::string str = m_color.toHumanReadableString(m_pixelFormat,
    app::Color::ShortHumanReadableString);

  setTextQuiet(str.c_str());

  gfx::Color textcolor = gfx::rgba(255, 255, 255);
  if (color.isValid())
    textcolor = color_utils::blackandwhite_neg(
      gfx::rgba(color.getRed(), color.getGreen(), color.getBlue()));

  gfx::Rect textrc;
  getTextIconInfo(NULL, &textrc);
  g->drawUIString(text(), textcolor, gfx::ColorNone, textrc.origin());
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
    m_window->ColorChange.connect(&ColorButton::onWindowColorChange, this);
  }

  m_window->setColor(m_color, ColorSelector::ChangeType);
  m_window->openWindow();

  x = MID(0, bounds().x, ui::display_w()-m_window->bounds().w);
  if (bounds().y2() <= ui::display_h()-m_window->bounds().h)
    y = MAX(0, bounds().y2());
  else
    y = MAX(0, bounds().y-m_window->bounds().h);

  m_window->positionWindow(x, y);

  m_window->manager()->dispatchMessages();
  m_window->layout();

  // Setup the hot-region
  gfx::Rect rc = bounds().createUnion(m_window->bounds());
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

void ColorButton::onActiveSiteChange(const Site& site)
{
  if (m_dependOnLayer)
    invalidate();
}

} // namespace app
