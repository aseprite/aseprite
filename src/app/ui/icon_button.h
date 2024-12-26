// Aseprite
// Copyright (C) 2001-2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_ICON_BUTTON_H_INCLUDED
#define APP_UI_ICON_BUTTON_H_INCLUDED
#pragma once

#include "app/ui/skin/skin_part.h"
#include "ui/button.h"

namespace app {

class IconButton : public ui::Button {
public:
  IconButton(const skin::SkinPartPtr& part);

protected:
  void onInitTheme(ui::InitThemeEvent& ev) override;
  void onSizeHint(ui::SizeHintEvent& ev) override;
  void onPaint(ui::PaintEvent& ev) override;

private:
  skin::SkinPartPtr m_part;
};

} // namespace app

#endif
