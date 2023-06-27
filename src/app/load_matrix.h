// Aseprite
// Copyright (C) 2023  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_LOAD_MATRIX_H_INCLUDED
#define APP_LOAD_MATRIX_H_INCLUDED
#pragma once

#include <string>

namespace render {
  class DitheringMatrix;
};

namespace app {

  void load_dithering_matrix_from_sprite(
    const std::string& filename,
    render::DitheringMatrix& matrix);

} // namespace app

#endif
