// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_UI_MAP_ALGORITHM_SELECTOR_H_INCLUDED
#define APP_UI_MAP_ALGORITHM_SELECTOR_H_INCLUDED
#pragma once

#include "doc/rgbmap_algorithm.h"
#include "ui/combobox.h"

namespace app {

class RgbMapAlgorithmSelector : public ui::ComboBox {
public:
  RgbMapAlgorithmSelector();

  doc::RgbMapAlgorithm algorithm();
  void algorithm(doc::RgbMapAlgorithm mapAlgo);
};

} // namespace app

#endif
