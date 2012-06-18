// ASEPRITE gui library
// Copyright (C) 2001-2012  David Capello
//
// This source file is ditributed under a BSD-like license, please
// read LICENSE.txt for more information.

#ifndef GUI_COMBOBOX_H_INCLUDED
#define GUI_COMBOBOX_H_INCLUDED

#include "base/compiler_specific.h"
#include "base/signal.h"
#include "gui/widget.h"

#include <string>
#include <vector>

namespace ui {

  class Button;
  class Entry;
  class Frame;
  class ListBox;

  class ComboBox : public Widget
  {
  public:
    ComboBox();
    ~ComboBox();

    void setEditable(bool state);
    void setClickOpen(bool state);
    void setCaseSensitive(bool state);

    bool isEditable();
    bool isClickOpen();
    bool isCaseSensitive();

    int addItem(const std::string& text);
    void insertItem(int itemIndex, const std::string& text);
    void removeItem(int itemIndex);
    void removeAllItems();

    int getItemCount();

    std::string getItemText(int itemIndex);
    void setItemText(int itemIndex, const std::string& text);
    int findItemIndex(const std::string& text);

    int getSelectedItem();
    void setSelectedItem(int itemIndex);

    void* getItemData(int itemIndex);
    void setItemData(int itemIndex, void* data);

    Entry* getEntryWidget();
    Button* getButtonWidget();

    void openListBox();
    void closeListBox();
    void switchListBox();
    JRect getListBoxPos();

    // Signals
    Signal0<void> Change;

  protected:
    bool onProcessMessage(Message* msg) OVERRIDE;
    void onPreferredSize(PreferredSizeEvent& ev) OVERRIDE;

  private:
    void onButtonClick(Event& ev);

    struct Item;

    Entry* m_entry;
    Button* m_button;
    Frame* m_window;
    ListBox* m_listbox;
    std::vector<Item*> m_items;
    int m_selected;
    bool m_editable : 1;
    bool m_clickopen : 1;
    bool m_casesensitive : 1;
  };

} // namespace ui

#endif
