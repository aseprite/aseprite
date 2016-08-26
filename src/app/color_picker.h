// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COLOR_PICKER_H_INCLUDED
#define APP_COLOR_PICKER_H_INCLUDED
#pragma once

#include "app/color.h"
#include "doc/layer.h"
#include "gfx/point.h"

namespace doc {
  class Site;
}

namespace app {

  class ColorPicker {
  public:
    enum Mode { FromComposition, FromActiveLayer };

    ColorPicker();

    void pickColor(const doc::Site& site,
      const gfx::Point& pos, Mode mode);

    app::Color color() const { return m_color; }
    int alpha() const { return m_alpha; }
    doc::Layer* layer() const { return m_layer; }

  private:
    app::Color m_color;
    int m_alpha;
    doc::Layer* m_layer;
  };

} // namespace app

#endif
