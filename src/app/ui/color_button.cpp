// Aseprite
// Copyright (C) 2001-2017  David Capello
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
#include "app/modules/gui.h"
#include "app/ui/color_bar.h"
#include "app/ui/color_popup.h"
#include "app/ui/editor/editor.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "doc/layer.h"
#include "doc/site.h"
#include "doc/sprite.h"
#include "gfx/rect_io.h"
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

ColorButton::ColorButton(const app::Color& color,
                         PixelFormat pixelFormat,
                         bool canPinSelector)
  : ButtonBase("", colorbutton_type(), kButtonWidget, kButtonWidget)
  , m_color(color)
  , m_pixelFormat(pixelFormat)
  , m_window(NULL)
  , m_dependOnLayer(false)
  , m_canPinSelector(canPinSelector)
{
  this->setFocusStop(true);

  setup_mini_font(this);
  setStyle(SkinTheme::instance()->newStyles.colorButton());

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

void ColorButton::setColor(const app::Color& color)
{
  m_color = color;

  // Change the color in its related window
  if (m_window)
    m_window->setColor(m_color, ColorPopup::DoNotChangeType);

  // Emit signal
  Change(color);

  invalidate();
}

app::Color ColorButton::getColorByPosition(const gfx::Point& pos)
{
  // Ignore the position
  return m_color;
}

bool ColorButton::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      if (!m_windowDefaultBounds.isEmpty() &&
          this->isVisible()) {
        openSelectorDialog();
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
      StatusBar::instance()->clearText();
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
        ui::set_mouse_cursor(kEyedropperCursor);
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
  g->drawUIText(text(), textcolor, gfx::ColorNone, textrc.origin(), 0);
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

void ColorButton::onLoadLayout(ui::LoadLayoutEvent& ev)
{
  if (m_canPinSelector) {
    bool pinned = false;
    ev.stream() >> pinned;
    if (ev.stream() && pinned)
      ev.stream() >> m_windowDefaultBounds;
  }
}

void ColorButton::onSaveLayout(ui::SaveLayoutEvent& ev)
{
  if (m_canPinSelector && m_window && m_window->isPinned())
    ev.stream() << 1 << ' ' << m_window->bounds();
  else
    ev.stream() << 0;
}

void ColorButton::openSelectorDialog()
{
  bool pinned = (!m_windowDefaultBounds.isEmpty());

  if (m_window == NULL) {
    m_window = new ColorPopup(m_canPinSelector);
    m_window->ColorChange.connect(&ColorButton::onWindowColorChange, this);
  }

  if (pinned)
    m_window->setPinned(true);

  m_window->setColor(m_color, ColorPopup::ChangeType);
  m_window->openWindow();

  gfx::Rect winBounds = m_windowDefaultBounds;
  if (!pinned) {
    winBounds = gfx::Rect(m_window->bounds().origin(),
                          m_window->sizeHint());
    winBounds.x = MID(0, bounds().x, ui::display_w()-winBounds.w);
    if (bounds().y2() <= ui::display_h()-winBounds.h)
      winBounds.y = MAX(0, bounds().y2());
    else
      winBounds.y = MAX(0, bounds().y-winBounds.h);
  }
  winBounds.x = MID(0, winBounds.x, ui::display_w()-winBounds.w);
  winBounds.y = MID(0, winBounds.y, ui::display_h()-winBounds.h);
  m_window->setBounds(winBounds);

  m_window->manager()->dispatchMessages();
  m_window->layout();

  // Setup the hot-region
  if (!pinned) {
    gfx::Rect rc = bounds().createUnion(m_window->bounds());
    rc.enlarge(8);
    gfx::Region rgn(rc);
    static_cast<PopupWindow*>(m_window)->setHotRegion(rgn);
  }

  m_windowDefaultBounds = gfx::Rect();
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

  if (m_canPinSelector) {
    // Hide window
    if (!site.document()) {
      if (m_window)
        m_window->setVisible(false);
    }
    // Show window if it's pinned
    else {
      // Check if it's pinned from the preferences (m_windowDefaultBounds)
      if (!m_window && !m_windowDefaultBounds.isEmpty())
        openSelectorDialog();
      // Or check if the window was hidden but it's pinned, so we've
      // to show it again.
      else if (m_window && m_window->isPinned())
        m_window->setVisible(true);
    }
  }
}

} // namespace app
