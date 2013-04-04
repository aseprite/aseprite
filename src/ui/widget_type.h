// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_WIDGET_TYPE_H_INCLUDED
#define UI_WIDGET_TYPE_H_INCLUDED

namespace ui {

  // Widget types.
  enum WidgetType {
    // Undefined (or anonymous) widget type.
    kGenericWidget,

    // Known widgets.
    kBoxWidget,
    kButtonWidget,
    kCheckWidget,
    kComboBoxWidget,
    kEntryWidget,
    kGridWidget,
    kImageViewWidget,
    kLabelWidget,
    kListBoxWidget,
    kListItemWidget,
    kManagerWidget,
    kMenuWidget,
    kMenuBarWidget,
    kMenuBoxWidget,
    kMenuItemWidget,
    kSplitterWidget,
    kRadioWidget,
    kSeparatorWidget,
    kSliderWidget,
    kTextBoxWidget,
    kViewWidget,
    kViewScrollbarWidget,
    kViewViewportWidget,
    kWindowWidget,

    // User widgets.
    kFirstUserWidget,
    kLastUserWidget = 0x7fffffff
  };

} // namespace ui

#endif  // UI_WIDGET_TYPE_H_INCLUDED
