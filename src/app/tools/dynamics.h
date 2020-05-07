// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_DYNAMICS_H_INCLUDED
#define APP_TOOLS_DYNAMICS_H_INCLUDED
#pragma once

#include "render/dithering_matrix.h"

namespace app {
namespace tools {

  enum class DynamicSensor {
    Static,
    Pressure,
    Velocity,
  };

  enum class ColorFromTo {
    BgToFg,
    FgToBg,
  };

  struct DynamicsOptions {
    DynamicSensor size = DynamicSensor::Static;
    DynamicSensor angle = DynamicSensor::Static;
    DynamicSensor gradient = DynamicSensor::Static;
    int minSize = 0;
    int minAngle = 0;
    render::DitheringMatrix ditheringMatrix;
    ColorFromTo colorFromTo = ColorFromTo::BgToFg;
    float minPressureThreshold = 0.0f, maxPressureThreshold = 1.0f;
    float minVelocityThreshold = 0.0f, maxVelocityThreshold = 1.0f;

    bool isDynamic() const {
      return (size != DynamicSensor::Static ||
              angle != DynamicSensor::Static ||
              gradient != DynamicSensor::Static);
    }
  };

} // namespace tools
} // namespace app

#endif
