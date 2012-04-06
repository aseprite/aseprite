// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_LISTBOX_H_INCLUDED
#define GUI_LISTBOX_H_INCLUDED

#include "base/compiler_specific.h"
#include "gui/widget.h"

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

protected:
  bool onProcessMessage(Message* msg) OVERRIDE;
  void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;

private:
  void layoutListBox(JRect rect);
  void dirtyChildren();
};

#endif
