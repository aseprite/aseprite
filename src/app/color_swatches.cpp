// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/color_swatches.h"

namespace app {

ColorSwatches::ColorSwatches(const std::string& name)
  : m_name(name)
{
}

void ColorSwatches::addColor(const Color& color)
{
  m_colors.push_back(color);
}

void ColorSwatches::insertColor(size_t index, const Color& color)
{
  m_colors.insert(m_colors.begin() + index, color);
}

void ColorSwatches::removeColor(size_t index)
{
  m_colors.erase(m_colors.begin() + index);
}

} // namespace app
