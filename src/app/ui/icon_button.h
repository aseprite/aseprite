// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_ICON_BUTTON_H_INCLUDED
#define APP_UI_ICON_BUTTON_H_INCLUDED
#pragma once

#include "ui/button.h"

namespace app {

  class IconButton : public ui::Button {
  public:
    IconButton(she::Surface* icon);

  protected:
    // bool onProcessMessage(ui::Message* msg) override;
    void onSizeHint(ui::SizeHintEvent& ev) override;
    void onPaint(ui::PaintEvent& ev) override;

  private:
    she::Surface* m_icon;
  };

} // namespace app

#endif
