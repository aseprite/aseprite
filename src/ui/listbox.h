// ASEPRITE gui library
// Copyright (C) 2001-2013  David Capello
//
// This source file is distributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_LISTBOX_H_INCLUDED
#define UI_LISTBOX_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "ui/widget.h"

namespace ui {

  class ListItem;

  class ListBox : public Widget
  {
  public:
    ListBox();

    ListItem* getSelectedChild();
    int getSelectedIndex();

    void selectChild(ListItem* item);
    void selectIndex(int index);

    size_t getItemsCount() const;

    void centerScroll();

    Signal0<void> ChangeSelectedItem;
    Signal0<void> DoubleClickItem;

  protected:
    virtual bool onProcessMessage(Message* msg) OVERRIDE;
    virtual void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    virtual void onChangeSelectedItem();
    virtual void onDoubleClickItem();

  private:
    void layoutListBox(JRect rect);
  };

} // namespace ui

#endif
