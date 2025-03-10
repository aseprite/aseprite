// Aseprite UI Library
// Copyright (C) 2025  Igara Studio S.A.
// Copyright (C) 2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_SCALE_H_INCLUDED
#define UI_SCALE_H_INCLUDED
#pragma once

namespace ui {

// This value is a factor to multiply every screen size/coordinate.
// Every icon/graphics/font should be scaled to this factor.
int guiscale();

// num: is a numerator with the guiscale() already applied.
inline int guiscaled_div(const int uiscaled_num, const int dem)
{
  return ((uiscaled_num / guiscale()) / dem) * guiscale();
}

inline int guiscaled_center(const int p, const int s1, const int s2)
{
  return (p / guiscale() + (s1 / guiscale()) / 2 - (s2 / guiscale()) / 2) * guiscale();
}

} // namespace ui

#endif
