// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_WIDGET_TYPE_MISMATCH_H_INCLUDED
#define APP_WIDGET_TYPE_MISMATCH_H_INCLUDED
#pragma once

#include <string>
#include <stdexcept>

namespace app {

  class WidgetTypeMismatch : public std::runtime_error {
  public:
    WidgetTypeMismatch(const std::string& widgetId)
      : std::runtime_error("Widget " + widgetId +
                           " of the expected type.\nPlease reinstall the program.\n\n") { }
  };

} // namespace app

#endif
