// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_COLOR_PICKER_H_INCLUDED
#define APP_COLOR_PICKER_H_INCLUDED
#pragma once

#include "app/color.h"
#include "doc/layer.h"
#include "gfx/point.h"

namespace render {
  class Projection;
}

namespace app {
  class Site;

  class ColorPicker {
  public:
    enum Mode {
      FromComposition,
      FromActiveLayer,
      FromFirstReferenceLayer
    };

    ColorPicker();

    void pickColor(const Site& site,
                   const gfx::PointF& pos,
                   const render::Projection& proj,
                   const Mode mode);

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
