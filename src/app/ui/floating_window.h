// Aseprite
// Copyright (C) 2026-present  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_FLOATING_WINDOW_H_INCLUDED
#define APP_UI_FLOATING_WINDOW_H_INCLUDED
#pragma once

#include "ui/window.h"

#include <string>

namespace app {

class FloatingWindow : public ui::Window {
public:
  FloatingWindow(const std::string& title, const std::string& configSection);
  ~FloatingWindow();

  bool isEnabled() const { return m_isEnabled; }
  void setEnabled(bool state);

protected:
  bool onProcessMessage(ui::Message* msg) override;
  void onClose(ui::CloseEvent& ev) override;

private:
  std::string m_configSection;
  bool m_isEnabled;
};

} // namespace app

#endif // APP_UI_FLOATING_WINDOW_H_INCLUDED
