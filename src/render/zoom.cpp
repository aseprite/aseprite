// Aseprite Render Library
// Copyright (c) 2001-2014 David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "render/zoom.h"

namespace render {

void Zoom::in()
{
  if (m_den > 1) {
    m_den--;
  }
  else if (m_num < 64) {
    m_num++;
  }
}

void Zoom::out()
{
  if (m_num > 1) {
    m_num--;
  }
  else if (m_den < 32) {
    m_den++;
  }
}

// static
Zoom Zoom::fromScale(double scale)
{
  if (scale >= 1.0) {
    return Zoom(int(scale), 1);
  }
  else {
    return Zoom(1, int(1.0 / scale));
  }
}

} // namespace render
