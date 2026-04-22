// Aseprite
// Copyright (C) 2020-2025  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_NOTIFICATIONS_H_INCLUDED
#define APP_UI_NOTIFICATIONS_H_INCLUDED
#pragma once

#include "app/ui/dockable.h"
#include "ui/button.h"
#include "ui/menu.h"

namespace ui {
class Style;
}

namespace app {
class INotificationDelegate;

class Notifications : public ui::Button,
                      public Dockable {
public:
  Notifications();

  void addLink(INotificationDelegate* del);
  bool hasNotifications() const { return m_popup.hasChildren(); }

  // Dockable impl
  int dockableAt() const override { return ui::TOP | ui::BOTTOM | ui::LEFT | ui::RIGHT; }

protected:
  void onInitTheme(ui::InitThemeEvent& ev) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;
  void onClick() override;

private:
  bool m_red;
  ui::Menu m_popup;
};

} // namespace app

#endif
