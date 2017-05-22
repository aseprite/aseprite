// Aseprite
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_DITHERING_SELECTOR_H_INCLUDED
#define APP_UI_DITHERING_SELECTOR_H_INCLUDED
#pragma once

#include "render/dithering_algorithm.h"
#include "render/ordered_dither.h"
#include "ui/box.h"
#include "ui/combobox.h"

namespace app {

  class DitheringSelector : public ui::ComboBox {
  public:
    DitheringSelector();

    render::DitheringAlgorithm ditheringAlgorithm();
    render::DitheringMatrix ditheringMatrix();
  };

} // namespace app

#endif
