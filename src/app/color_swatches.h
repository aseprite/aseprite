// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_COLOR_SWATCHES_H_INCLUDED
#define APP_COLOR_SWATCHES_H_INCLUDED
#pragma once

#include <vector>

#include "app/color.h"

namespace app {

class ColorSwatches
{
public:
  ColorSwatches(const std::string& name);

  size_t size() const { return m_colors.size(); }

  const std::string& getName() const { return m_name; }
  void setName(std::string& name) { m_name = name; }

  void addColor(const Color& color);
  void insertColor(size_t index, const Color& color);
  void removeColor(size_t index);

  const Color& operator[](size_t index) const {
    return m_colors[index];
  }

private:
  std::string m_name;
  std::vector<Color> m_colors;
};

} // namespace app

#endif
