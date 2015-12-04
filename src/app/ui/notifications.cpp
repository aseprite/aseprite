// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/ui/notifications.h"

#include "app/notification_delegate.h"
#include "app/ui/skin/skin_theme.h"
#include "app/ui/skin/style.h"
#include "base/launcher.h"
#include "ui/menu.h"
#include "ui/paint_event.h"
#include "ui/size_hint_event.h"

namespace app {

using namespace ui;

class NotificationItem : public MenuItem {
public:
  NotificationItem(INotificationDelegate* del)
    : MenuItem(del->notificationText()),
      m_delegate(del) {
  }

protected:
  void onClick() override {
    MenuItem::onClick();
    m_delegate->notificationClick();
  }

private:
  INotificationDelegate* m_delegate;
};

Notifications::Notifications()
  : Button("")
  , m_flagStyle(skin::SkinTheme::instance()->styles.flag())
  , m_withNotifications(false)
{
}

void Notifications::addLink(INotificationDelegate* del)
{
  m_popup.addChild(new NotificationItem(del));
  m_withNotifications = true;
}

void Notifications::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(16, 10)*guiscale()); // TODO hard-coded flag size
}

void Notifications::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();

  skin::Style::State state;
  if (hasMouseOver()) state += skin::Style::hover();
  if (m_withNotifications) state += skin::Style::active();
  if (isSelected()) state += skin::Style::clicked();
  m_flagStyle->paint(g, clientBounds(), NULL, state);
}

void Notifications::onClick(ui::Event& ev)
{
  m_withNotifications = false;
  invalidate();

  gfx::Rect bounds = this->bounds();
  m_popup.showPopup(gfx::Point(
      bounds.x - m_popup.sizeHint().w,
      bounds.y + bounds.h));
}

} // namespace app
