// Aseprite UI Library
// Copyright (C) 2019-2020  Igara Studio S.A.
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_WIDGETS_LIST_H_INCLUDED
#define UI_WIDGETS_LIST_H_INCLUDED
#pragma once

#include <vector>

#define UI_FIRST_WIDGET(list_name)                      \
  ((list_name).empty() ? nullptr: (list_name).front())

namespace ui {

  class Widget;
  typedef std::vector<Widget*> WidgetsList;

} // namespace ui

#endif
