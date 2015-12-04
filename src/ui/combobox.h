// Aseprite UI Library
// Copyright (C) 2001-2013, 2015  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_COMBOBOX_H_INCLUDED
#define UI_COMBOBOX_H_INCLUDED
#pragma once

#include "base/signal.h"
#include "ui/widget.h"

#include <string>
#include <vector>

namespace ui {

  class Button;
  class Entry;
  class ListBox;
  class ListItem;
  class Window;

  class ComboBoxEntry;
  class ComboBoxListBox;

  class ComboBox : public Widget {
    friend class ComboBoxEntry;
    friend class ComboBoxListBox;

  public:
    typedef std::vector<ListItem*> ListItems;

    ComboBox();
    ~ComboBox();

    ListItems::iterator begin() { return m_items.begin(); }
    ListItems::iterator end() { return m_items.end(); }

    void setEditable(bool state);
    void setClickOpen(bool state);
    void setCaseSensitive(bool state);

    bool isEditable() const { return m_editable; }
    bool isClickOpen() const { return m_clickopen; }
    bool isCaseSensitive() const { return m_casesensitive; }

    int addItem(ListItem* item);
    int addItem(const std::string& text);
    void insertItem(int itemIndex, ListItem* item);
    void insertItem(int itemIndex, const std::string& text);

    // Removes the given item (you must delete it).
    void removeItem(ListItem* item);

    // Removes and deletes the given item.
    void removeItem(int itemIndex);

    void removeAllItems();

    int getItemCount() const;

    ListItem* getItem(int itemIndex);
    const std::string& getItemText(int itemIndex) const;
    void setItemText(int itemIndex, const std::string& text);
    int findItemIndex(const std::string& text) const;
    int findItemIndexByValue(const std::string& value) const;

    ListItem* getSelectedItem() const;
    void setSelectedItem(ListItem* item);

    int getSelectedItemIndex() const;
    void setSelectedItemIndex(int itemIndex);

    std::string getValue() const;
    void setValue(const std::string& value);

    Entry* getEntryWidget();
    Button* getButtonWidget();

    void openListBox();
    void closeListBox();
    void switchListBox();
    gfx::Rect getListBoxPos() const;

    // Signals
    base::Signal0<void> Change;
    base::Signal0<void> OpenListBox;
    base::Signal0<void> CloseListBox;

  protected:
    bool onProcessMessage(Message* msg) override;
    void onResize(ResizeEvent& ev) override;
    void onSizeHint(SizeHintEvent& ev) override;
    virtual void onChange();
    virtual void onOpenListBox();
    virtual void onCloseListBox();

  private:
    void onButtonClick(Event& ev);

    ComboBoxEntry* m_entry;
    Button* m_button;
    Window* m_window;
    ComboBoxListBox* m_listbox;
    ListItems m_items;
    int m_selected;
    bool m_editable;
    bool m_clickopen;
    bool m_casesensitive;
  };

} // namespace ui

#endif
