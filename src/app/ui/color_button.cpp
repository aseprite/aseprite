// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/color_button.h"

#include "app/app.h"
#include "app/color.h"
#include "app/color_utils.h"
#include "app/modules/editors.h"
#include "app/modules/gfx.h"
#include "app/site.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_popup.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "doc/layer.h"
#include "doc/sprite.h"
#include "gfx/rect_io.h"
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

ColorButton::ColorButton(const app::Color& color,
                         const PixelFormat pixelFormat,
                         const ColorButtonOptions& options)
  : ButtonBase("", colorbutton_type(), kButtonWidget, kButtonWidget)
  , m_color(color)
  , m_pixelFormat(pixelFormat)
  , m_window(nullptr)
  , m_dependOnLayer(false)
  , m_options(options)
{
  setFocusStop(true);
  initTheme();

  UIContext::instance()->add_observer(this);
}

ColorButton::~ColorButton()
{
  UIContext::instance()->remove_observer(this);

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

void ColorButton::setColor(const app::Color& origColor)
{
  // Before change (this signal can modify the color)
  app::Color color = origColor;
  BeforeChange(color);

  m_color = color;

  // Change the color in its related window
  if (m_window) {
    // In the window we show the original color. In case
    // BeforeChange() has changed the color type (e.g. to index), we
    // don't care, in the window we prefer to keep the original
    // HSV/HSL values.
    m_window->setColor(origColor, ColorPopup::DontChangeType);
  }

  // Emit signal
  Change(color);

  invalidate();
}

app::Color ColorButton::getColorByPosition(const gfx::Point& pos)
{
  // Ignore the position
  return m_color;
}

void ColorButton::onInitTheme(InitThemeEvent& ev)
{
  ButtonBase::onInitTheme(ev);
  setStyle(SkinTheme::instance()->styles.colorButton());
}

bool ColorButton::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      if (!m_windowDefaultBounds.isEmpty() &&
          this->isVisible()) {
        openPopup(false);
      }
      break;

    case kCloseMessage:
      if (m_window && m_window->isVisible())
        m_window->closeWindow(NULL);
      break;

    case kMouseEnterMessage:
      StatusBar::instance()->showColor(0, "", m_color);
      break;

    case kMouseLeaveMessage:
      StatusBar::instance()->showDefaultText();
      break;

    case kMouseMoveMessage:
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        Widget* picked = manager()->pick(mousePos);
        app::Color color = m_color;

        if (picked && picked != this) {
          // Pick a color from a IColorSource
          if (IColorSource* colorSource = dynamic_cast<IColorSource*>(picked)) {
            color = colorSource->getColorByPosition(mousePos);
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
        ui::set_mouse_cursor(kCustomCursor, SkinTheme::instance()->cursors.eyedropper());
        return true;
      }
      break;

  }

  return ButtonBase::onProcessMessage(msg);
}

void ColorButton::onSizeHint(SizeHintEvent& ev)
{
  ButtonBase::onSizeHint(ev);

  gfx::Rect box;
  getTextIconInfo(&box);
  box.w = 64*guiscale();

  gfx::Size sz = ev.sizeHint();
  sz.w = std::max(sz.w, box.w);
  ev.setSizeHint(sz);
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

      if (int(current_editor->sprite()->transparentColor()) == color.getIndex() &&
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
  g->drawUIText(text(), textcolor, gfx::ColorNone, textrc.origin(), 0);
}

void ColorButton::onClick(Event& ev)
{
  ButtonBase::onClick(ev);

  // If the popup window was not created or shown yet..
  if (!m_window || !m_window->isVisible()) {
    // Open it
    openPopup(false);
  }
  else if (!m_window->isMoveable()) {
    // If it is visible, close it
    closePopup();
  }
}

void ColorButton::onLoadLayout(ui::LoadLayoutEvent& ev)
{
  if (canPin()) {
    bool pinned = false;
    ev.stream() >> pinned;
    if (ev.stream() && pinned)
      ev.stream() >> m_windowDefaultBounds;

    m_hiddenPopupBounds = m_windowDefaultBounds;
  }
}

void ColorButton::onSaveLayout(ui::SaveLayoutEvent& ev)
{
  if (canPin() && m_window && m_window->isPinned())
    ev.stream() << 1 << ' ' << m_window->bounds();
  else
    ev.stream() << 0;
}

bool ColorButton::isPopupVisible()
{
  return (m_window && m_window->isVisible());
}

void ColorButton::openPopup(const bool forcePinned)
{
  const bool pinned = forcePinned ||
    (!m_windowDefaultBounds.isEmpty());

  if (m_window == NULL) {
    m_window = new ColorPopup(m_options);
    m_window->Close.connect(&ColorButton::onWindowClose, this);
    m_window->ColorChange.connect(&ColorButton::onWindowColorChange, this);
  }

  m_window->setColor(m_color, ColorPopup::ChangeType);
  m_window->openWindow();

  gfx::Rect winBounds;
  if (!pinned || (forcePinned && m_hiddenPopupBounds.isEmpty())) {
    winBounds = gfx::Rect(m_window->bounds().origin(),
                          m_window->sizeHint());
    winBounds.x = MID(0, bounds().x, ui::display_w()-winBounds.w);
    if (bounds().y2() <= ui::display_h()-winBounds.h)
      winBounds.y = MAX(0, bounds().y2());
    else
      winBounds.y = MAX(0, bounds().y-winBounds.h);
  }
  else if (forcePinned) {
    winBounds = m_hiddenPopupBounds;
  }
  else {
    winBounds = m_windowDefaultBounds;
  }
  winBounds.x = MID(0, winBounds.x, ui::display_w()-winBounds.w);
  winBounds.y = MID(0, winBounds.y, ui::display_h()-winBounds.h);
  m_window->setBounds(winBounds);

  m_window->manager()->dispatchMessages();
  m_window->layout();

  m_window->setPinned(pinned);

  // Add the ColorButton area to the ColorPopup hot-region
  if (!pinned) {
    gfx::Rect rc = bounds().createUnion(m_window->bounds());
    rc.enlarge(8);
    gfx::Region rgn(rc);
    static_cast<PopupWindow*>(m_window)->setHotRegion(rgn);
  }

  m_windowDefaultBounds = gfx::Rect();
}

void ColorButton::closePopup()
{
  if (m_window)
    m_window->closeWindow(nullptr);
}

void ColorButton::onWindowClose(ui::CloseEvent& ev)
{
  m_hiddenPopupBounds = m_window->bounds();
}

void ColorButton::onWindowColorChange(const app::Color& color)
{
  setColor(color);
}

void ColorButton::onActiveSiteChange(const Site& site)
{
  if (m_dependOnLayer)
    invalidate();

  if (canPin()) {
    // Hide window
    if (!site.document()) {
      if (m_window)
        m_window->setVisible(false);
    }
    // Show window if it's pinned
    else {
      // Check if it's pinned from the preferences (m_windowDefaultBounds)
      if (!m_window && !m_windowDefaultBounds.isEmpty())
        openPopup(false);
      // Or check if the window was hidden but it's pinned, so we've
      // to show it again.
      else if (m_window && m_window->isPinned())
        m_window->setVisible(true);
    }
  }
}

} // namespace app
