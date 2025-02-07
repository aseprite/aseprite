// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LABEL_H_INCLUDED
#define UI_LABEL_H_INCLUDED
#pragma once

#include "gfx/color.h"
#include "ui/widget.h"

namespace ui {

class Label : public Widget {
public:
  Label(const std::string& text);
};

} // namespace ui

#endif
