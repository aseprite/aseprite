// Aseprite
// Copyright (C) 2024  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_MINI_HELP_BUTTON_H_INCLUDED
#define APP_UI_MINI_HELP_BUTTON_H_INCLUDED
#pragma once

#include "ui/button.h"

namespace app {

class MiniHelpButton : public ui::Button {
public:
  MiniHelpButton(const std::string& link);

private:
  void onInitTheme(ui::InitThemeEvent& ev) override;
  void onClick() override;
  void onSetDecorativeWidgetBounds() override;

  std::string m_link;
};

} // namespace app

#endif
