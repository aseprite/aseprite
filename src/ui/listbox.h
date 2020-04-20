// Aseprite UI Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_LISTBOX_H_INCLUDED
#define UI_LISTBOX_H_INCLUDED
#pragma once

#include "obs/signal.h"
#include "ui/widget.h"

#include <vector>

namespace ui {

  class ListItem;

  class ListBox : public Widget {
  public:
    ListBox();

    bool isMultiselect() const { return m_multiselect; }
    void setMultiselect(const bool multiselect);

    Widget* getSelectedChild();
    int getSelectedIndex();

    void selectChild(Widget* item, Message* msg = nullptr);
    void selectIndex(int index, Message* msg = nullptr);

    int getItemsCount() const;

    void makeChildVisible(Widget* item);
    void centerScroll();
    void sortItems();
    void sortItems(bool (*cmp)(Widget* a, Widget* b));

    obs::signal<void()> Change;
    obs::signal<void()> DoubleClickItem;

  protected:
    virtual bool onProcessMessage(Message* msg) override;
    virtual void onPaint(PaintEvent& ev) override;
    virtual void onResize(ResizeEvent& ev) override;
    virtual void onSizeHint(SizeHintEvent& ev) override;
    virtual void onChange();
    virtual void onDoubleClickItem();

    int getChildIndex(Widget* item);
    Widget* getChildByIndex(int index);

    int advanceIndexThroughVisibleItems(
      int startIndex, int delta, const bool loop);

    // True if this listbox accepts selecting multiple items at the
    // same time.
    bool m_multiselect;

    // Range of items selected when we click down/up. Used to specify
    // the range of selected items in a multiselect operation.
    int m_firstSelectedIndex;
    int m_lastSelectedIndex;

    // Initial state (isSelected()) of each list item when the
    // selection operation started. It's used to switch the state of
    // items in case that the user is Ctrl+clicking items several
    // items at the same time.
    std::vector<bool> m_states;
  };

} // namespace ui

#endif
