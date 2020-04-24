// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_TOOLS_VELOCITY_H_INCLUDED
#define APP_TOOLS_VELOCITY_H_INCLUDED
#pragma once

#include "app/tools/pointer.h"
#include "base/time.h"
#include "base/vector2d.h"
#include "gfx/point.h"

namespace app {
namespace tools {

// Mouse velocity sensor
class VelocitySensor {
public:
  // Milliseconds between two updates to change the velocity vector
  // with the new value (shorter periods will mix the old velocity
  // vector with the new one).
  static constexpr float kFullUpdateMSecs = 50.0f;

  // Maximum length of the velocity vector (number of screen pixels
  // traveled between two updates) to create a max sensor output.
  static constexpr float kScreenPixelsForFullVelocity = 32.0f; // TODO 32 should be configurable

  VelocitySensor();

  void reset();

  const Vec2& velocity() const { return m_velocity; }

  void updateWithScreenPoint(const gfx::Point& screenPoint);

private:
  bool m_firstPoint;
  Vec2 m_velocity;
  gfx::Point m_lastPoint;
  base::tick_t m_lastUpdate;
};

} // namespace tools
} // namespace app

#endif
