// Aseprite UI Library
// Copyright (C) 2001-2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LISTBOX_H_INCLUDED
#define UI_LISTBOX_H_INCLUDED
#pragma once

#include "base/signal.h"
#include "ui/widget.h"

namespace ui {

  class ListItem;

  class ListBox : public Widget {
  public:
    ListBox();

    Widget* getSelectedChild();
    int getSelectedIndex();

    void selectChild(Widget* item);
    void selectIndex(int index);

    std::size_t getItemsCount() const;

    void makeChildVisible(Widget* item);
    void centerScroll();
    void sortItems();

    Signal0<void> Change;
    Signal0<void> DoubleClickItem;

  protected:
    virtual bool onProcessMessage(Message* msg) override;
    virtual void onPaint(PaintEvent& ev) override;
    virtual void onResize(ResizeEvent& ev) override;
    virtual void onPreferredSize(PreferredSizeEvent& ev) override;
    virtual void onChange();
    virtual void onDoubleClickItem();
  };

} // namespace ui

#endif
