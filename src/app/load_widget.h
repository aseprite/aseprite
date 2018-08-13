// Aseprite
// Copyright (C) 2001-2018  David Capello
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifndef APP_LOAD_WIDGET_H_INCLUDED
#define APP_LOAD_WIDGET_H_INCLUDED
#pragma once

#include "app/widget_loader.h"

namespace app {

  template<class T>
  inline T* load_widget(const char* fileName, const char* widgetId, T* widget = NULL) {
    WidgetLoader loader;
    return loader.loadWidgetT<T>(fileName, widgetId, widget);
  }

} // namespace app

#endif
