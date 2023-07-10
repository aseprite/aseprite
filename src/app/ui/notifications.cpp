// Aseprite
// Copyright (C) 2020-2023  Igara Studio S.A.
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
  , m_flagStyle(skin::SkinTheme::get(this)->styles.flag())
  , m_red(false)
{
}

void Notifications::addLink(INotificationDelegate* del)
{
  m_popup.addChild(new NotificationItem(del));
  m_red = true;
}

void Notifications::onSizeHint(SizeHintEvent& ev)
{
  ev.setSizeHint(gfx::Size(16, 10)*guiscale()); // TODO hard-coded flag size
}

void Notifications::onPaint(PaintEvent& ev)
{
  Graphics* g = ev.graphics();

  PaintWidgetPartInfo info;
  if (hasMouse()) info.styleFlags |= ui::Style::Layer::kMouse;
  if (m_red) info.styleFlags |= ui::Style::Layer::kFocus;
  if (isSelected()) info.styleFlags |= ui::Style::Layer::kSelected;

  theme()->paintWidgetPart(
    g, m_flagStyle, clientBounds(), info);
}

void Notifications::onClick()
{
  m_red = false;
  invalidate();

  gfx::Rect bounds = this->bounds();
  m_popup.showPopup(
    gfx::Point(
      bounds.x - m_popup.sizeHint().w,
      bounds.y2()),
    display());
}

} // namespace app
