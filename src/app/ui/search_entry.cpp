// Aseprite
// Copyright (c) 2020-2025  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/search_entry.h"

#include "app/ui/skin/skin_theme.h"
#include "os/surface.h"
#include "ui/graphics.h"
#include "ui/message.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"
#include "ui/timer.h"

#include <algorithm>

namespace app {

using namespace app::skin;
using namespace gfx;
using namespace ui;

SearchEntry::SearchEntry() : Entry(256, ""), m_clearOnEsc(false), m_debounceMs(0)
{
}

void SearchEntry::setDebounce(int ms)
{
  m_debounceMs = ms;
  if (!m_debounceMs && m_debounceTimer)
    m_debounceTimer.reset();
}

bool SearchEntry::onProcessMessage(ui::Message* msg)
{
  switch (msg->type()) {
    case kTimerMessage: {
      if (static_cast<TimerMessage*>(msg)->timer() == m_debounceTimer.get()) {
        Change();
        m_debounceTimer->stop();
      }
      break;
    }
    case kMouseDownMessage: {
      Rect closeBounds = getCloseIconBounds();
      Point mousePos = static_cast<MouseMessage*>(msg)->position() - bounds().origin();

      if (closeBounds.contains(mousePos)) {
        onCloseIconPressed();
        return true;
      }
      break;
    }
    case kKeyDownMessage: {
      if (m_clearOnEsc && !text().empty() && static_cast<KeyMessage*>(msg)->scancode() == kKeyEsc) {
        onCloseIconPressed();
        return true;
      }
      break;
    }
  }
  return Entry::onProcessMessage(msg);
}

void SearchEntry::onPaint(ui::PaintEvent& ev)
{
  auto theme = SkinTheme::get(this);
  theme->paintEntry(ev);

  const auto color = (isEnabled() ? theme->colors.text() : theme->colors.disabled());

  os::Surface* icon = theme->parts.iconSearch()->bitmap(0);
  Rect bounds = clientBounds();
  ev.graphics()->drawColoredRgbaSurface(icon,
                                        color,
                                        bounds.x + border().left(),
                                        bounds.y + bounds.h / 2 - icon->height() / 2);

  if (!text().empty()) {
    icon = onGetCloseIcon();
    ev.graphics()->drawColoredRgbaSurface(
      icon,
      color,
      bounds.x + bounds.w - border().right() - childSpacing() - icon->width(),
      bounds.y + bounds.h / 2 - icon->height() / 2);
  }
}

void SearchEntry::onSizeHint(SizeHintEvent& ev)
{
  Entry::onSizeHint(ev);
  Size sz = ev.sizeHint();

  auto theme = SkinTheme::get(this);
  auto icon = theme->parts.iconSearch()->bitmap(0);
  sz.h = std::max(sz.h, icon->height() + border().height());

  ev.setSizeHint(sz);
}

void SearchEntry::onChange()
{
  if (!m_debounceMs) {
    Change();
    return;
  }

  if (!m_debounceTimer)
    m_debounceTimer = std::make_unique<Timer>(m_debounceMs, this);

  m_debounceTimer->setInterval(m_debounceMs);
  m_debounceTimer->start();
}

Rect SearchEntry::onGetEntryTextBounds() const
{
  auto theme = SkinTheme::get(this);
  Rect bounds = Entry::onGetEntryTextBounds();
  auto icon1 = theme->parts.iconSearch()->bitmap(0);
  auto icon2 = onGetCloseIcon();
  bounds.x += childSpacing() + icon1->width();
  bounds.w -= 2 * childSpacing() + icon1->width() + icon2->width();
  return bounds;
}

os::Surface* SearchEntry::onGetCloseIcon() const
{
  return SkinTheme::get(this)->parts.iconClose()->bitmap(0);
}

void SearchEntry::onCloseIconPressed()
{
  // Prevents a broken caret when clearing a scrolled entry
  setCaretPos(0);
  deselectText();

  setText("");
  onChange();
}

Rect SearchEntry::getCloseIconBounds() const
{
  Rect bounds = clientBounds();
  auto icon = onGetCloseIcon();
  bounds.x += bounds.w - border().right() - childSpacing() - icon->width();
  bounds.y += bounds.h / 2 - icon->height() / 2;
  bounds.w = icon->width();
  bounds.h = icon->height();
  return bounds;
}

} // namespace app
