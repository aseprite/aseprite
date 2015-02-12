// Aseprite
// Copyright (C) 2001-2015  David Capello
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation.

#ifndef APP_LOAD_WIDGET_H_INCLUDED
#define APP_LOAD_WIDGET_H_INCLUDED
#pragma once

#include "app/widget_loader.h"
#include "base/unique_ptr.h"

namespace app {

  template<class T>
  inline T* load_widget(const char* fileName, const char* widgetId, T* widget = NULL) {
    WidgetLoader loader;
    return loader.loadWidgetT<T>(fileName, widgetId, widget);
  }

} // namespace app

#endif
