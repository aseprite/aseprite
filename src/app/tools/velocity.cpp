// Aseprite
// Copyright (C) 2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/tools/velocity.h"

#include "base/clamp.h"

namespace app {
namespace tools {

VelocitySensor::VelocitySensor()
{
  reset();
}

void VelocitySensor::reset()
{
  m_firstPoint = true;
  m_lastUpdate = base::current_tick();
  m_velocity = Vec2(0.0f, 0.0f);
}

void VelocitySensor::updateWithScreenPoint(const gfx::Point& screenPoint)
{
  const base::tick_t t = base::current_tick();
  const base::tick_t dt = t - m_lastUpdate;
  m_lastUpdate = t;

  // Calculate the velocity (new screen point - old screen point)
  if (m_firstPoint)
    m_firstPoint = false;
  else {
    gfx::PointF newVelocity(screenPoint - m_lastPoint);

    const float a = base::clamp(float(dt) / kFullUpdateMSecs, 0.0f, 1.0f);
    m_velocity.x = (1.0f-a)*m_velocity.x + a*newVelocity.x;
    m_velocity.y = (1.0f-a)*m_velocity.y + a*newVelocity.y;
  }

  m_lastPoint.x = screenPoint.x;
  m_lastPoint.y = screenPoint.y;
}

} // namespace tools
} // namespace app
