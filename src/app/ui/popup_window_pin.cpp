// Aseprite
// Copyright (C) 2020-2021  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/popup_window_pin.h"

#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/skin/skin_theme.h"
#include "gfx/border.h"
#include "gfx/size.h"
#include "ui/ui.h"

#include <vector>

namespace app {

using namespace app::skin;
using namespace ui;

PopupWindowPin::PopupWindowPin(const std::string& text,
                               const ClickBehavior clickBehavior,
                               const bool canPin)
  : PopupWindow(text, clickBehavior, EnterBehavior::CloseOnEnter, canPin)
  , m_pinned(false)
{
}

void PopupWindowPin::setPinned(const bool pinned)
{
  m_pinned = pinned;
  if (m_pinned)
    makeFloating();
  else {
    gfx::Rect rc = boundsOnScreen();
    rc.enlarge(8 * guiscale());
    setHotRegion(gfx::Region(rc));
    makeFixed();
  }
}

bool PopupWindowPin::onProcessMessage(Message* msg)
{
  switch (msg->type()) {
    case kOpenMessage: {
      if (!m_pinned)
        setPinned(false);
      break;
    }

    case kCloseMessage: {
      // If the closer() wasn't the hot region or the window, it might
      // be because the user pressed the close button.
      if (closer() && closer() != this)
        m_pinned = false;
      break;
    }
  }

  return PopupWindow::onProcessMessage(msg);
}

void PopupWindowPin::onWindowMovement()
{
  PopupWindow::onWindowMovement();

  // If the window isn't pinned and we move it, we can automatically
  // pin it.
  if (!m_pinned)
    setPinned(true);
}

} // namespace app
