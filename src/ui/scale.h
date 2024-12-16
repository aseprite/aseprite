// Aseprite UI Library
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

} // namespace ui

#endif
