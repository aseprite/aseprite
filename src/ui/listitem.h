// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_LISTITEM_H_INCLUDED
#define UI_LISTITEM_H_INCLUDED

#include "base/compiler_specific.h"
#include "ui/widget.h"

namespace ui {

  class ListItem : public Widget
  {
  public:
    ListItem(const char* text);

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onResize(ResizeEvent& ev) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
  };

} // namespace ui

#endif
