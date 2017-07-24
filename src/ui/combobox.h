// Aseprite UI Library
// Copyright (C) 2001-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef UI_COMBOBOX_H_INCLUDED
#define UI_COMBOBOX_H_INCLUDED
#pragma once

#include "obs/signal.h"
#include "ui/widget.h"

#include <string>
#include <vector>

namespace ui {

  class Button;
  class Entry;
  class Event;
  class ListBox;
  class Window;

  class ComboBoxEntry;
  class ComboBoxListBox;

  class ComboBox : public Widget {
    friend class ComboBoxEntry;
    friend class ComboBoxListBox;

  public:
    typedef std::vector<Widget*> Items;

    ComboBox();
    ~ComboBox();

    Items::iterator begin() { return m_items.begin(); }
    Items::iterator end() { return m_items.end(); }

    void setEditable(bool state);
    void setClickOpen(bool state);
    void setCaseSensitive(bool state);
    void setUseCustomWidget(bool state);

    bool isEditable() const { return m_editable; }
    bool isClickOpen() const { return m_clickopen; }
    bool isCaseSensitive() const { return m_casesensitive; }
    bool useCustomWidget() const { return m_useCustomWidget; }

    int addItem(Widget* item);
    int addItem(const std::string& text);
    void insertItem(int itemIndex, Widget* item);
    void insertItem(int itemIndex, const std::string& text);

    // Removes the given item (you must delete it).
    void removeItem(Widget* item);

    // Removes and deletes the given item.
    void removeItem(int itemIndex);

    void removeAllItems();

    int getItemCount() const;

    Widget* getItem(int itemIndex);
    const std::string& getItemText(int itemIndex) const;
    void setItemText(int itemIndex, const std::string& text);
    int findItemIndex(const std::string& text) const;
    int findItemIndexByValue(const std::string& value) const;

    Widget* getSelectedItem() const;
    void setSelectedItem(Widget* item);

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
    obs::signal<void()> Change;
    obs::signal<void()> OpenListBox;
    obs::signal<void()> CloseListBox;

  protected:
    bool onProcessMessage(Message* msg) override;
    void onResize(ResizeEvent& ev) override;
    void onSizeHint(SizeHintEvent& ev) override;
    virtual void onChange();
    virtual void onOpenListBox();
    virtual void onCloseListBox();

  private:
    void onButtonClick(Event& ev);
    void filterMessages();
    void removeMessageFilters();
    void putSelectedItemAsCustomWidget();

    ComboBoxEntry* m_entry;
    Button* m_button;
    Window* m_window;
    ComboBoxListBox* m_listbox;
    Items m_items;
    int m_selected;
    bool m_editable;
    bool m_clickopen;
    bool m_casesensitive;
    bool m_filtering;
    bool m_useCustomWidget;
  };

} // namespace ui

#endif
