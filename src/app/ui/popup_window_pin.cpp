// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/popup_window_pin.h"

#include "app/modules/gfx.h"
#include "app/modules/gui.h"
#include "app/ui/skin/button_icon_impl.h"
#include "app/ui/skin/skin_theme.h"
#include "base/bind.h"
#include "gfx/border.h"
#include "gfx/size.h"
#include "ui/ui.h"

#include <vector>

namespace app {

using namespace app::skin;
using namespace ui;

PopupWindowPin::PopupWindowPin(const std::string& text, ClickBehavior clickBehavior)
  : PopupWindow(text, clickBehavior)
  , m_pin("")
{
  SkinTheme* theme = SkinTheme::instance();

  m_pin.Click.connect(&PopupWindowPin::onPinClick, this);
  m_pin.setIconInterface(
    new ButtonIconImpl(theme->parts.unpinned(),
                       theme->parts.pinned(),
                       theme->parts.unpinned(),
                       CENTER | MIDDLE));
}

void PopupWindowPin::onPinClick(Event& ev)
{
  if (m_pin.isSelected()) {
    makeFloating();
  }
  else {
    gfx::Rect rc = bounds();
    rc.enlarge(8);
    setHotRegion(gfx::Region(rc));
    makeFixed();
  }
}

bool PopupWindowPin::onProcessMessage(Message* msg)
{
  switch (msg->type()) {

    case kOpenMessage:
      m_pin.setSelected(false);
      makeFixed();
      break;

    case kCloseMessage:
      m_pin.setSelected(false);
      break;

  }

  return PopupWindow::onProcessMessage(msg);
}

void PopupWindowPin::onHitTest(HitTestEvent& ev)
{
  PopupWindow::onHitTest(ev);

  if ((m_pin.isSelected()) &&
      (ev.hit() == HitTestClient)) {
    if (ev.point().x <= bounds().x+2)
      ev.setHit(HitTestBorderW);
    else if (ev.point().x >= bounds().x2()-3)
      ev.setHit(HitTestBorderE);
    else
      ev.setHit(HitTestCaption);
  }
}

} // namespace app
