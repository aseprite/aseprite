// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef UI_LISTBOX_H_INCLUDED
#define UI_LISTBOX_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "ui/widget.h"

namespace ui {

  class ListBox : public Widget
  {
  public:
    class Item : public Widget
    {
    public:
      Item(const char* text);

    protected:
      bool onProcessMessage(Message* msg) OVERRIDE;
      void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;
    };

    ListBox();

    Item* getSelectedChild();
    int getSelectedIndex();

    void selectChild(Item* item);
    void selectIndex(int index);

    int getItemsCount();

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
    void dirtyChildren();
  };

} // namespace ui

#endif
