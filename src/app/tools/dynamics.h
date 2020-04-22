// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_DYNAMICS_H_INCLUDED
#define APP_TOOLS_DYNAMICS_H_INCLUDED
#pragma once

#include "render/dithering_algorithm.h"
#include "render/dithering_matrix.h"

namespace app {
namespace tools {

  enum class DynamicSensor {
    Static,
    Pressure,
    Velocity,
  };

  struct DynamicsOptions {
    DynamicSensor size = DynamicSensor::Static;
    DynamicSensor angle = DynamicSensor::Static;
    DynamicSensor gradient = DynamicSensor::Static;
    int maxSize = 0;
    int maxAngle = 0;
    render::DitheringAlgorithm ditheringAlgorithm = render::DitheringAlgorithm::None;
    render::DitheringMatrix ditheringMatrix;

    bool isDynamic() const {
      return (size != DynamicSensor::Static ||
              angle != DynamicSensor::Static ||
              gradient != DynamicSensor::Static);
    }
  };

} // namespace tools
} // namespace app

#endif
