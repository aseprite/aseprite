// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_WIDGET_TYPE_MISMATCH_H_INCLUDED
#define APP_WIDGET_TYPE_MISMATCH_H_INCLUDED
#pragma once

#include <stdexcept>
#include <string>

namespace app {

class WidgetTypeMismatch : public std::runtime_error {
public:
  WidgetTypeMismatch(const std::string& widgetId)
    : std::runtime_error("Widget " + widgetId +
                         " of the expected type.\nPlease reinstall the program.\n\n")
  {
  }
};

} // namespace app

#endif
