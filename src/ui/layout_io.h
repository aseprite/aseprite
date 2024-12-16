// Aseprite UI Library
// Copyright (C) 2001-2013  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LAYOUT_IO_H_INCLUDED
#define UI_LAYOUT_IO_H_INCLUDED
#pragma once

#include <string>

namespace ui {

class Widget;

class LayoutIO {
public:
  virtual ~LayoutIO() {}
  virtual std::string loadLayout(Widget* widget) = 0;
  virtual void saveLayout(Widget* widget, const std::string& str) = 0;
};

} // namespace ui

#endif
