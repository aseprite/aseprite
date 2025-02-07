// Aseprite
// Copyright (C) 2020-2022  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_USER_DATA_VIEW_H_INCLUDED
#define APP_UI_USER_DATA_VIEW_H_INCLUDED
#pragma once

#include "app/pref/preferences.h"
#include "app/ui/color_button.h"
#include "doc/user_data.h"
#include "obs/signal.h"
#include "ui/base.h"
#include "ui/entry.h"
#include "ui/grid.h"
#include "ui/label.h"

#include "user_data.xml.h"

namespace app {

class UserDataView {
public:
  UserDataView(Option<bool>& visibility);

  void configureAndSet(const doc::UserData& userData, ui::Grid* parent);
  void toggleVisibility();
  void setVisible(bool state, bool saveAsDefault = true);

  const doc::UserData& userData() const { return m_userData; }
  ColorButton* color() { return m_container.color(); }
  ui::Entry* entry() { return m_container.entry(); }
  ui::Label* colorLabel() { return m_container.colorLabel(); }
  ui::Label* entryLabel() { return m_container.entryLabel(); }

  // Called when the user data text or color are changed by the user
  // (not from the code/self-update).
  obs::signal<void()> UserDataChange;

private:
  bool isVisible() const { return m_visibility(); }
  void onEntryChange();
  void onColorChange();

  gen::UserData m_container;
  doc::UserData m_userData;
  Option<bool>& m_visibility;
  bool m_isConfigured = false;
  // True if the change of the user data is from the same object
  // (not from the user), i.e. from configureAndSet()
  bool m_selfUpdate = false;
};

} // namespace app

#endif
