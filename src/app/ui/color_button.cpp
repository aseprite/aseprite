// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
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
#include "os/system.h"
#include "os/window.h"
#include "ui/ui.h"

#include <algorithm>

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
  , m_desktopCoords(false)
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

  auto theme = SkinTheme::get(this);
  setStyle(theme->styles.colorButton());

  if (m_window)
    m_window->initTheme();
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
      StatusBar::instance()->showColor(0, m_color);
      break;

    case kMouseLeaveMessage:
      StatusBar::instance()->showDefaultText();
      break;

    case kMouseMoveMessage:
      // TODO code similar to TileButton::onProcessMessage()
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        app::Color color = m_color;

        // Try to pick a color from a IColorSource, then get the color
        // from the display surface, and finally from the desktop. The
        // desktop must be a last resource method, because in macOS it
        // will ask for permissions to record the screen.
        os::Window* nativeWindow = msg->display()->nativeWindow();
        gfx::Point screenPos = nativeWindow->pointToScreen(mousePos);

        Widget* picked = manager()->pickFromScreenPos(screenPos);
        if (picked == this) {
          setColor(m_startDragColor);
          break;
        }
        else {
          m_mouseLeft = true;
        }

        IColorSource* colorSource = dynamic_cast<IColorSource*>(picked);
        if (colorSource) {
          nativeWindow = picked->display()->nativeWindow();
          mousePos = nativeWindow->pointFromScreen(screenPos);
        }
        else {
          gfx::Color gfxColor = gfx::ColorNone;

          // Get color from native window surface
          if (nativeWindow->contentRect().contains(screenPos)) {
            mousePos = nativeWindow->pointFromScreen(screenPos);
            if (nativeWindow->surface()->bounds().contains(mousePos))
              gfxColor = nativeWindow->surface()->getPixel(mousePos.x, mousePos.y);
          }

          // Or get the color from the screen
          if (gfxColor == gfx::ColorNone) {
            gfxColor = os::instance()->getColorFromScreen(screenPos);
          }

          color = app::Color::fromRgb(gfx::getr(gfxColor),
                                      gfx::getg(gfxColor),
                                      gfx::getb(gfxColor),
                                      gfx::geta(gfxColor));
        }

        if (colorSource) {
          color = colorSource->getColorByPosition(mousePos);
        }

        // Did the color change?
        if (color != m_color) {
          setColor(color);
        }
      }
      break;

    case kSetCursorMessage:
      if (hasCapture()) {
        auto theme = SkinTheme::get(this);
        ui::set_mouse_cursor(kCustomCursor, theme->cursors.eyedropper());
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
  auto theme = SkinTheme::get(this);
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
    auto editor = Editor::activeEditor();
    if (color.getType() == app::Color::IndexType &&
        editor &&
        editor->sprite() &&
        editor->sprite()->pixelFormat() == IMAGE_INDEXED) {
      m_dependOnLayer = true;

      if (int(editor->sprite()->transparentColor()) == color.getIndex() &&
          editor->layer() &&
          !editor->layer()->isBackground()) {
        color = app::Color::fromMask();
      }
    }
  }

  draw_color_button(g, rc,
                    color,
                    (doc::ColorMode)m_pixelFormat,
                    hasMouse(), false);

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

void ColorButton::onClick()
{
  ButtonBase::onClick();

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

void ColorButton::onStartDrag()
{
  m_startDragColor = m_color;
  m_mouseLeft = false;
}

void ColorButton::onSelectWhenDragging()
{
  if (m_mouseLeft)
    setSelected(false);
  else
    ButtonBase::onSelectWhenDragging();
}

void ColorButton::onLoadLayout(ui::LoadLayoutEvent& ev)
{
  if (canPin()) {
    bool pinned = false;

    m_desktopCoords = false;
    ev.stream() >> pinned;
    if (ev.stream() && pinned)
      ev.stream() >> m_windowDefaultBounds
                  >> m_desktopCoords;

    m_hiddenPopupBounds = m_windowDefaultBounds;
  }
}

void ColorButton::onSaveLayout(ui::SaveLayoutEvent& ev)
{
  if (canPin() && m_window && m_window->isPinned()) {
    if (m_desktopCoords)
      ev.stream() << 1 << ' ' << m_window->lastNativeFrame() << ' ' << 1;
    else
      ev.stream() << 1 << ' ' << m_window->bounds() << ' ' << 0;
  }
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
  m_window->remapWindow();

  fit_bounds(
    display(),
    m_window,
    gfx::Rect(m_window->sizeHint()),
    [this, pinned, forcePinned](const gfx::Rect& workarea,
                                gfx::Rect& winBounds,
                                std::function<gfx::Rect(Widget*)> getWidgetBounds) {
      if (!pinned || (forcePinned && m_hiddenPopupBounds.isEmpty())) {
        gfx::Rect bounds = getWidgetBounds(this);

        winBounds.x = std::clamp(bounds.x, workarea.x, workarea.x2()-winBounds.w);
        if (bounds.y2()+winBounds.h <= workarea.y2())
          winBounds.y = std::max(workarea.y, bounds.y2());
        else
          winBounds.y = std::max(workarea.y, bounds.y-winBounds.h);
      }
      else if (forcePinned) {
        winBounds = convertBounds(m_hiddenPopupBounds);
      }
      else {
        winBounds = convertBounds(m_windowDefaultBounds);
      }
    });

  m_window->openWindow();

  m_window->setPinned(pinned);

  // Add the ColorButton area to the ColorPopup hot-region
  if (!pinned) {
    gfx::Rect rc = boundsOnScreen().createUnion(m_window->boundsOnScreen());
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
  if (get_multiple_displays()) {
    m_desktopCoords = true;
    m_hiddenPopupBounds = m_window->lastNativeFrame();
  }
  else {
    m_desktopCoords = false;
    m_hiddenPopupBounds = m_window->bounds();
  }
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

gfx::Rect ColorButton::convertBounds(const gfx::Rect& bounds) const
{
  // Convert to desktop
  if (get_multiple_displays() && !m_desktopCoords) {
    auto nativeWindow = display()->nativeWindow();
    return gfx::Rect(nativeWindow->pointToScreen(bounds.origin()),
                     nativeWindow->pointToScreen(bounds.point2()));
  }
  // Convert to display
  else if (!get_multiple_displays() && m_desktopCoords) {
    auto nativeWindow = display()->nativeWindow();
    return gfx::Rect(nativeWindow->pointFromScreen(bounds.origin()),
                     nativeWindow->pointFromScreen(bounds.point2()));
  }
  // No conversion is required
  else
    return bounds;
}

} // namespace app
