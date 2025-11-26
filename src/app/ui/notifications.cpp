// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include "app/ui/notifications.h"

#include "app/notification_delegate.h"
#include "app/ui/skin/skin_theme.h"
#include "base/launcher.h"
#include "ui/menu.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

namespace app {

using namespace app::skin;
using namespace ui;

class NotificationItem : public MenuItem {
public:
  NotificationItem(INotificationDelegate* del) : MenuItem(del->notificationText()), m_delegate(del)
  {
  }

protected:
  void onClick() override
  {
    MenuItem::onClick();
    m_delegate->notificationClick();
  }

private:
  INotificationDelegate* m_delegate;
};

Notifications::Notifications() : Button(""), m_red(false)
{
}

void Notifications::addLink(INotificationDelegate* del)
{
  m_popup.addChild(new NotificationItem(del));
  m_red = true;
}

void Notifications::onInitTheme(InitThemeEvent& ev)
{
  Button::onInitTheme(ev);
  m_popup.initTheme();
}

void Notifications::onSizeHint(SizeHintEvent& ev)
{
  auto* theme = SkinTheme::get(this);
  auto hint = theme->calcSizeHint(this, theme->styles.flag());
  ev.setSizeHint(hint);
}

void Notifications::onPaint(PaintEvent& ev)
{
  auto* theme = SkinTheme::get(this);
  Graphics* g = ev.graphics();

  PaintWidgetPartInfo info;
  if (hasMouse())
    info.styleFlags |= ui::Style::Layer::kMouse;
  if (m_red)
    info.styleFlags |= ui::Style::Layer::kFocus;
  if (isSelected())
    info.styleFlags |= ui::Style::Layer::kSelected;

  theme->paintWidgetPart(g, theme->styles.flag(), clientBounds(), info);
}

void Notifications::onClick()
{
  m_red = false;
  invalidate();

  gfx::Rect bounds = this->bounds();
  m_popup.showPopup(gfx::Point(bounds.x - m_popup.sizeHint().w, bounds.y2()), display());
}

} // namespace app
