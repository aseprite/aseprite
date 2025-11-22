// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LABEL_H_INCLUDED
#define UI_LABEL_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "ui/widget.h"

namespace ui {

class Label : public Widget {
public:
  Label(const std::string& text);

  Widget* buddy();
  void setBuddy(Widget* buddy);
  void setBuddy(const std::string& buddyId);

  // Keeps the label's enabled state synchronized with the buddy
  void setBuddySyncEnabled(bool sync)
  {
    if (m_buddySync != sync) {
      m_buddySync = sync;
      refreshBuddySync();
    }
  }
  bool buddySyncEnabled() const { return m_buddySync; }

protected:
  bool onProcessMessage(Message* msg) override;
  void onEnable(bool enabled) override;

private:
  void refreshBuddySync();

  Widget* m_buddy;
  bool m_buddySync;
  std::string m_buddyId;
  obs::scoped_connection m_buddyEnabledConn;
};

} // namespace ui

#endif
