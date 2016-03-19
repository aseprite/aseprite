// Aseprite
// Copyright (C) 2016  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_UI_COLOR_SELECTOR_H_INCLUDED
#define APP_UI_COLOR_SELECTOR_H_INCLUDED
#pragma once

#include "app/color.h"
#include "app/ui/color_source.h"
#include "base/signal.h"
#include "ui/mouse_buttons.h"
#include "ui/widget.h"

namespace app {

  class ColorSelector : public ui::Widget
                      , public IColorSource {
  public:
    ColorSelector();

    void selectColor(const app::Color& color);

    // Signals
    base::Signal2<void, const app::Color&, ui::MouseButtons> ColorChange;

  protected:
    void onSizeHint(ui::SizeHintEvent& ev) override;
    bool onProcessMessage(ui::Message* msg) override;

    app::Color m_color;

    // Internal flag used to lock the modification of m_color.
    // E.g. When the user picks a color harmony, we don't want to
    // change the main color.
    bool m_lockColor;
  };

} // namespace app

#endif
