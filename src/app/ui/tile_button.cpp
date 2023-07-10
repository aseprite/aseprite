// Aseprite
// Copyright (c) 2020-2023  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/tile_button.h"

#include "app/modules/gfx.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/status_bar.h"
#include "app/ui_context.h"
#include "fmt/format.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/system.h"
#include "ui/window.h"

namespace app {

using namespace app::skin;
using namespace ui;

static WidgetType tilebutton_type()
{
  static WidgetType type = kGenericWidget;
  if (type == kGenericWidget)
    type = register_widget_type();
  return type;
}

TileButton::TileButton()
  : ButtonBase("", tilebutton_type(), kButtonWidget, kButtonWidget)
{
  setFocusStop(true);
  initTheme();

  UIContext::instance()->add_observer(this);
}

TileButton::~TileButton()
{
  UIContext::instance()->remove_observer(this);
}

doc::tile_t TileButton::getTile() const
{
  return m_tile;
}

void TileButton::setTile(doc::tile_t origTile)
{
  // Before change (this signal can modify the color)
  doc::tile_t tile = origTile;
  BeforeChange(tile);
  m_tile = tile;
  Change(tile);
  invalidate();
}

doc::tile_t TileButton::getTileByPosition(const gfx::Point& pos)
{
  // Ignore the position
  return m_tile;
}

void TileButton::onInitTheme(InitThemeEvent& ev)
{
  ButtonBase::onInitTheme(ev);
  setStyle(SkinTheme::get(this)->styles.colorButton());
}

bool TileButton::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kMouseEnterMessage:
      StatusBar::instance()->showTile(0, m_tile);
      break;

    case kMouseLeaveMessage:
      StatusBar::instance()->showDefaultText();
      break;

    case kMouseMoveMessage:
      // TODO code similar to ColorButton::onProcessMessage()
      if (hasCapture()) {
        gfx::Point mousePos = static_cast<MouseMessage*>(msg)->position();
        doc::tile_t tile = m_tile;

        // Pick a tile from a ITileSource
        Widget* picked = window()->pick(mousePos);
        if (picked == this) {
          // Do nothing
          break;
        }

        ITileSource* tileSource = dynamic_cast<ITileSource*>(picked);

        // If there is no tile source in this window, try to get the
        // tile from other display, i.e. and editor in other native
        // window.
        if (!tileSource && get_multiple_displays()) {
          os::Window* nativeWindow = display()->nativeWindow();
          gfx::Point screenPos = nativeWindow->pointToScreen(mousePos);

          picked = manager()->pickFromScreenPos(screenPos);
          tileSource = (picked != this ? dynamic_cast<ITileSource*>(picked): nullptr);
          if (tileSource) {
            nativeWindow = picked->display()->nativeWindow();
            mousePos = nativeWindow->pointFromScreen(screenPos);
          }
        }

        if (tileSource) {
          tile = tileSource->getTileByPosition(mousePos);
        }

        // Did the tile change?
        if (tile != m_tile) {
          setTile(tile);
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

void TileButton::onSizeHint(SizeHintEvent& ev)
{
  ButtonBase::onSizeHint(ev);
  gfx::Size sz(32*guiscale(),
               32*guiscale());
  ev.setSizeHint(sz);
}

void TileButton::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();
  SkinTheme* theme = static_cast<SkinTheme*>(this->theme());
  gfx::Rect rc = clientBounds();

  gfx::Color bg = bgColor();
  if (gfx::is_transparent(bg))
    bg = theme->colors.face();
  g->fillRect(bg, rc);

  Site site = UIContext::instance()->activeSite();
  draw_tile_button(g, rc,
                   site, m_tile,
                   hasMouse(), false);

  // Draw text
  if (m_tile != doc::notile) {
    int baseIndex = 1;
    if (site.tileset())
      baseIndex = site.tileset()->baseIndex();

    std::string str = fmt::format(
      "{}", int(doc::tile_geti(m_tile)) + baseIndex - 1);
    setTextQuiet(str.c_str());

    // TODO calc a proper color for the text
    gfx::Color textcolor = gfx::rgba(0, 0, 0);
    gfx::Rect textrc;
    getTextIconInfo(NULL, &textrc);
    g->drawUIText(text(), textcolor, gfx::ColorNone, textrc.origin(), 0);
  }
}

void TileButton::onActiveSiteChange(const Site& site)
{
  invalidate();
}

} // namespace app
