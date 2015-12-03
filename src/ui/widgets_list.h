// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_WIDGETS_LIST_H_INCLUDED
#define UI_WIDGETS_LIST_H_INCLUDED
#pragma once

#include <vector>

#define UI_FOREACH_WIDGET_BACKWARD(list_name, iterator_name)    \
  for (WidgetsList::const_reverse_iterator                      \
         iterator_name = (list_name).rbegin(),                  \
         __end=(list_name).rend();                              \
       iterator_name != __end;                                  \
       ++iterator_name)

#define UI_FOREACH_WIDGET_WITH_END(list_name, iterator_name, end_name)  \
  for (WidgetsList::const_iterator                                      \
         iterator_name = (list_name).begin(),                           \
         end_name = (list_name).end();                                  \
       iterator_name != end_name;                                       \
       ++iterator_name)

#define UI_FIRST_WIDGET(list_name)                      \
  ((list_name).empty() ? NULL: (list_name).front())

namespace ui {

  class Widget;
  typedef std::vector<Widget*> WidgetsList;

} // namespace ui

#endif
